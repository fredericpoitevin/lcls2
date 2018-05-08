"""
Control - Control server

Author: Chris Ford <caf@slac.stanford.edu>
"""

import zmq
from ControlTransition import ControlTransition as Transition
from CMMsg import CMMsg as ControlMsg
from ControlState import ControlState, StateMachine
from psp import PV
import logging
import argparse
import time

class ControlStateMachine(StateMachine):

    # Define states
    state_unconfigured = ControlState(b'UNCONFIGURED')
    state_configured = ControlState(b'CONFIGURED')
    state_running = ControlState(b'RUNNING')
    state_enabled = ControlState(b'ENABLED')

    def __init__(self, pv_base):

        # initialize PVs

        self.pvMsgEnable = PV(pv_base+':MsgEnable')
        self.pvMsgDisable = PV(pv_base+':MsgDisable')
        self.pvRun = PV(pv_base+':Run')

        # register callbacks for each valid state+transition combination

        unconfigured_dict = {
            Transition.configure: (self.configfunc, self.state_configured)
        }

        configured_dict = {
            Transition.unconfigure: (self.unconfigfunc,
                                          self.state_unconfigured),
            Transition.beginrun: (self.beginrunfunc, self.state_running)
        }

        running_dict = {
            Transition.endrun: (self.endrunfunc, self.state_configured),
            Transition.enable: (self.enablefunc, self.state_enabled)
        }

        enabled_dict = {
            Transition.disable: (self.disablefunc, self.state_running)
        }

        self.state_unconfigured.register(unconfigured_dict)
        self.state_configured.register(configured_dict)
        self.state_running.register(running_dict)
        self.state_enabled.register(enabled_dict)

        # Start with a default state.
        self._state = self.state_unconfigured

    def configfunc(self):
        logging.debug("configfunc()")
        logging.debug("PV Run = 0")
        self.pvRun.put(0)
        return True

    def unconfigfunc(self):
        logging.debug("unconfigfunc()")
        logging.debug("PV Run = 0")
        self.pvRun.put(0)
        return True

    def beginrunfunc(self):
        logging.debug("beginrunfunc()")
        logging.debug("PV Run = 1")
        self.pvRun.put(1)
        return True

    def endrunfunc(self):
        logging.debug("endrunfunc()")
        logging.debug("PV Run = 0")
        self.pvRun.put(0)
        return True

    def enablefunc(self):
        logging.debug("enablefunc()")
        logging.debug("PV MsgEnable = 0->1->0")
        self.pvMsgEnable.put(0)
        self.pvMsgEnable.put(1)
        self.pvMsgEnable.put(0)
        logging.debug("PV Run = 1")
        self.pvRun.put(1)
        return True

    def disablefunc(self):
        logging.debug("disablefunc()")
        logging.debug("PV MsgDisable = 0->1->0")
        self.pvMsgDisable.put(0)
        self.pvMsgDisable.put(1)
        self.pvMsgDisable.put(0)
        logging.debug("PV Run = 1")
        self.pvRun.put(1)
        return True


def test_ControlStateMachine():

    # ControlStateMachine tests

    yy = ControlStateMachine('DAQ:LAB2:PART:2')
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_unconfigured)

    yy.on_transition(Transition.configure)
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_configured)

    yy.on_transition(Transition.beginrun)
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_running)

    yy.on_transition(Transition.enable)
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_enabled)

    yy.on_transition(Transition.disable)
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_running)

    yy.on_transition(Transition.endrun)
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_configured)

    yy.on_transition(Transition.unconfigure)
    print("ControlStateMachine state:", yy.state())
    assert(yy.state() == yy.state_unconfigured)

    print("ControlStateMachine OK")

    return


def main():

    # Process arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('pvbase', help='EPICS PV base (e.g. DAQ:LAB2:PART:2)')
    parser.add_argument('-p', type=int, choices=range(0, 8), default=0, help='platform (default 0)')
    parser.add_argument('-v', action='store_true', help='be verbose')
    args = parser.parse_args()

    if args.v:
        logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
    else:
        logging.basicConfig(level=logging.WARNING, format='%(asctime)s - %(levelname)s - %(message)s')

    logging.info('control server starting')

    # CM state
    yy = ControlStateMachine(args.pvbase)
    logging.debug("Initial ControlStateMachine state: %s" % yy.state())

    # context and sockets
    ctx = zmq.Context()
    cmd = ctx.socket(zmq.ROUTER)
    cmd.bind("tcp://*:%d" % ControlMsg.router_port(args.p))

    sequence = 0

    poller = zmq.Poller()
    poller.register(cmd, zmq.POLLIN)
    try:
        while True:
            items = dict(poller.poll(1000))

            # Execute state cmd request
            if cmd in items:
                msg = cmd.recv_multipart()
                identity = msg[0]
                request = msg[1]
                logging.debug('Received <%s> from cmd' % request.decode())

                if request == ControlMsg.PING:
                    # Send reply to client
                    logging.debug("Sending <PONG> reply")
                    cmd.send(identity, zmq.SNDMORE)
                    cmmsg = ControlMsg(key=ControlMsg.PONG)
                    cmmsg.send(cmd)
                    continue

                if request == ControlMsg.PONG:
                    continue

                if request in [ Transition.configure, Transition.beginrun,
                                Transition.enable, Transition.disable,
                                Transition.endrun, Transition.unconfigure,
                                ControlMsg.GETSTATE]:

                    if request != ControlMsg.GETSTATE:
                        # Do transition
                        yy.on_transition(request)

                    # Send reply to client
                    cmd.send(identity, zmq.SNDMORE)
                    cmmsg = ControlMsg(key=yy.state().key())
                    cmmsg.send(cmd)
                    continue

                else:
                    logging.warning("Unknown msg <%s>" % request.decode())
                    # Send reply to client
                    logging.debug("Sending <HUH?> reply")
                    cmd.send(identity, zmq.SNDMORE)
                    cmmsg = ControlMsg(key=ControlMsg.HUH)
                    cmmsg.send(cmd)
                    continue

    except KeyboardInterrupt:
        logging.debug("Interrupt received")

    # Clean up
    logging.debug("Clean up")

    time.sleep(.25)

    # close zmq sockets
    cmd.close()

    # terminate zmq context
    ctx.term()

    logging.info('control server exiting')

if __name__ == '__main__':
    main()