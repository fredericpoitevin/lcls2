from PyQt5 import QtCore, QtGui, QtWidgets
from p4p.client.thread import Context
import logging

try:
    QString = unicode
except NameError:
    # Python 3
    QString = str

try:
    QChar = unichr
except NameError:
    # Python 3
    QChar = chr

logger = logging.getLogger(__name__)

NBeamSeq = 16

interval   = 14./13.
dstsel     = ['Include','DontCare']
evtselSc   = ['Fixed Rate','AC Rate','Sequence']
evtselCu   = ['Fixed Rate','AC Rate','EventCodes']
fixedRates  = ['929kHz','71.4kHz','10.2kHz','1.02kHz','102Hz','10.2Hz','1.02Hz']
acRates     = ['60Hz','30Hz','10Hz','5Hz','1Hz']
acTS        = ['TS%u'%(i+1) for i in range(6)]
seqBits     = ['b%u'%i for i in range(16)]
# Sequence 16 is programmed for rates stepping at 10kHz
seqIdxs     = ['s%u'%i for i in range(18)]
seqBursts   = ['%u x %.2fus'%(2<<(i%4),float(int(i/4+1))*interval) for i in range(16)]
seqRates    = ['%u0kHz'%(i+1) for i in range(16)]
seqLocal    = ['%u0kHz'%(4*i+4) for i in range(16)]
seqGlobal   = ['GLT %d'%(i) for i in range(17)]

frLMH       = { 'L':0, 'H':1, 'M':2, 'm':3 }
toLMH       = { 0:'L', 1:'H', 2:'M', 3:'m' }
pvactx      = Context('pva')

nogui       = False
xtpg        = False

def setCuMode(v):
    global xtpg
    print('CuMode',v)
    xtpg = v

def getCuMode():
    return xtpg

class Pv:
    def __init__(self, pvname, callback=None, isStruct=False):
        self.pvname = pvname
        self.__value__ = None
        self.isStruct = isStruct
        if callback:
            logger.info("Monitoring PV %s", self.pvname)
            def monitor_cb(newval):
                if self.isStruct:
                    self.__value__ = newval
                else:
                    self.__value__ = newval.raw.value
                logger.info("Received monitor event for PV %s, received %s", self.pvname, self.__value__)
#                try:
                callback(err=None)
#                except Exception as e:
#                    logger.error("Exception in callback for %s"%self.pvname)
            try:
                self.subscription = pvactx.monitor(self.pvname, monitor_cb)
                self.__value__ = None
            except TimeoutError as e:
                logger.error("Timeout expection connecting to PV %s", pvname)
        else:
            self.__value__ = None
            logger.debug("PV %s created without a callback", self.pvname) # Call get explictly for an sync get or use for put

    def get(self, useCached=True):
        if self.isStruct:
            self.__value__ = pvactx.get(self.pvname)
        else:
            self.__value__ = pvactx.get(self.pvname).raw.value
        logger.info("Current value of PV %s Value %s", self.pvname, self.__value__)
        return self.__value__

    def put(self, newval, wait=None):
        logger.info("Putting to PV %s current value %s new value %s", self.pvname, self.__value__, newval)
        ret =  pvactx.put(self.pvname, newval, wait=wait)
        self.__value__ = newval
        return ret

    def monitor(self, callback):
        if callback:
            logger.info("Monitoring PV %s", self.pvname)
            def monitor_cb(newval):
                if self.isStruct:
                    self.__value__ = newval
                else:
                    self.__value__ = newval.raw.value
                logger.info("Received monitor event for PV %s, received %s", self.pvname, self.__value__)
                callback(err=None)
            self.subscription = pvactx.monitor(self.pvname, monitor_cb)


def initPvMon(mon, pvname, isStruct=False):
    logger.info("Monitoring PV %s", pvname)
    mon.pv = Pv(pvname, mon.update, isStruct=isStruct)

class PvDisplay(QtWidgets.QLabel):

    valueSet = QtCore.pyqtSignal('QString',name='valueSet')

    def __init__(self):
        QtWidgets.QLabel.__init__(self, "-")
        self.setMinimumWidth(100)

    def connect_signal(self):
        self.valueSet.connect(self.setValue)

    def setValue(self,value):
        self.setText(value)

class PvLabel(QtWidgets.QWidget):
    def __init__(self, owner, parent, pvbase, name, dName=None, isInt=False, isTime=False, scale=None, units=None):
        super(PvLabel,self).__init__()
        layout = QtWidgets.QHBoxLayout()
        layout.setContentsMargins(0,0,0,0)
        label  = QtWidgets.QLabel(name)
        label.setMinimumWidth(100)
        layout.addWidget(label)
        #layout.addStretch()
        self.__display = PvDisplay()
        self.__display.connect_signal()
        layout.addWidget(self.__display)
        self.setLayout(layout)
        parent.addWidget(self)

        pvname = pvbase+name
        print(pvname)
        if dName is not None:
            dPvName = pvbase+dName
            self.dPv = Pv(dPvName)
            self.dPv.monitor(self.update)
        else:
            self.dPv = None
        self.isInt = isInt
        self.isTime = isTime
        self.scale = scale
        self.units = units
        initPvMon(self,pvname)

        owner._pvlabels.append(self)

    def update(self, err):
        q = self.pv.__value__
        if self.dPv is not None:
            dq = self.dpv.__value__
        else:
            dq = None
        if err is None:
            s = QString('fail')
            try:
                if self.isTime:
                    dat = QtCore.QDateTime(QtCore.QDate(1990,1,1),QtCore.QTime(0,0,0),QtCore.Qt.UTC).addSecs(int(q)).toLocalTime()
                    s = QString(dat.toString("yyyy-MMM-dd HH:mm:ss"))
                elif self.isInt:
                    s = QString("%s (0x%s)") % ((QString('{:,}'.format(int(q)))),QString(format(int(q)&0xffffffff, 'x')))
                    if dq is not None:
                        s = s + QString(" [%s (0x%s)]") % ((QString(int(dq))),(format(int(dq)&0xffffffff, 'x')))
                else:
                    if self.scale is None:
                        s = QString('{:,}'.format(q))
                        if dq is not None:
                            s = s + QString(" [%s]") % (QString(dq))
                    else:
                        s = '{0:.4f}'.format(q*self.scale)
                        if dq is not None:
                            s = s + ' [{0:.4f}]'.format(dq*self.scale)
                    if self.units is not None:
                        s = s + self.units
            except:
#                logger.error('Exception in pvLable')
                v = ''
                for i in range(len(q)):
                    #v = v + ' %f'%q[i]
#                    v = v + ' ' + QString(q[i]*self.scale)
                    v = v + " %5.2f" % (q[i]*self.scale)
                    if dq is not None:
#                        v = v + QString(" [%s]") % (QString(dq[i]*self.scale))
                        v = v + " %5.2f" % (dq[i]*self.scale)
                        #v = v + ' [' + '%f'%dq[i] + ']'
                    if ((i%8)==7):
                        v = v + '\n'
                if self.units is not None:
                    v = v + self.units
                s = QString(v)

            if nogui:
                print(self.pv.pvname,str(s))
            else:
                self.__display.valueSet.emit(s)
        else:
            print(err)

class PvPushButton(QtWidgets.QPushButton):

    valueSet = QtCore.pyqtSignal('QString',name='valueSet')

    def __init__(self, pvname, label):
        super(PvPushButton, self).__init__(label)
        sz = len(label)*8
        if sz < 25:
            sz = 25
        self.setMaximumWidth(sz) # Revisit

        self.clicked.connect(self.buttonClicked)

        self.pv = Pv(pvname)

    def buttonClicked(self):
        self.pv.put(1)
        self.pv.put(0)

class CheckBox(QtWidgets.QCheckBox):

    valueSet = QtCore.pyqtSignal(int, name='valueSet')

    def __init__(self, label):
        super(CheckBox, self).__init__(label)

    def connect_signal(self):
        self.valueSet.connect(self.boxClicked)

    def boxClicked(self, state):
        #print "CheckBox.clicked: state:", state
        self.setChecked(state)

class PvCheckBox(CheckBox):

    def __init__(self, pvname, label):
        super(PvCheckBox, self).__init__(label)
        self.connect_signal()
        self.clicked.connect(self.pvClicked)
        initPvMon(self,pvname)

    def pvClicked(self):
        q = self.isChecked()
        self.pv.put(1 if q else 0)
        #print "PvCheckBox.clicked: pv %s q %x" % (self.pv.name, q)

    def update(self, err):
        #print ("PvCheckBox.update:  pv %s, i %s, v %x, err %s" % (self.pv.name, self.text(), self.pv.get(), err))
        q = self.pv.__value__ != 0
        if err is None:
            if nogui:
                print(self.pv.pvname,q)
            else:
                if q != self.isChecked():  self.valueSet.emit(q)
        else:
            print(err)

class PvTextDisplay(QtWidgets.QLineEdit):

    valueSet = QtCore.pyqtSignal('QString',name='valueSet')

    def __init__(self, label):
        super(PvTextDisplay, self).__init__("-")
        #self.setMinimumWidth(60)

    def connect_signal(self):
        self.valueSet.connect(self.setValue)

    def setValue(self,value):
        self.setText(value)

class PvComboDisplay(QtWidgets.QComboBox):

    #valueSet = QtCore.pyqtSignal('QString',name='valueSet')
    valueSet = QtCore.pyqtSignal(int ,name='valueSet')

    def __init__(self, choices):
        super(PvComboDisplay, self).__init__()
        self.addItems(choices)

    def connect_signal(self):
        self.valueSet.connect(self.setValue)

    def setValue(self,value):
        self.setCurrentIndex(value)

class PvEditTxt(PvTextDisplay):

    def __init__(self, pv, label):
        super(PvEditTxt, self).__init__(label)
        self.connect_signal()
        self.editingFinished.connect(self.setPv)
        initPvMon(self,pv)

class PvEditInt(PvEditTxt):

    def __init__(self, pv, label):
        super(PvEditInt, self).__init__(pv, label)

    def setPv(self):
        try:
            value = int(self.text())
        except ValueError:
            # ignore input that fails to convert to int
            pass
        else:
            self.pv.put(value)

    def update(self, err):
#        print 'Update '+pv  #  This print is evil.
        q = self.pv.__value__
        if err is None:
            s = QString('fail')
            try:
                s = QString("%s") % (QString(int(q)))
            except:
                v = ''
                for i in range(len(q)):
                    v = v + ' %f'%q[i]
                s = QString(v)

            if nogui:
                print(self.pv.pvname,str(s))
            else:
                self.valueSet.emit(s)
        else:
            print(err)


class PvInt(PvEditInt):

    def __init__(self,pv):
        super(PvInt, self).__init__(pv, '')
#        self.setEnabled(False)

    def setPv(self):
        pass

class PvIntArray:

    def __init__(self, pv, widgets):
        self.widgets = widgets
        initPvMon(self,pv)

    def update(self, err):
        q = self.pv.__value__
        if err is None:
            for i in range(len(q)):
#                self.widgets[i].valueSet.emit(QString(format(q[i], 'd')))
                self.widgets[i].setText(QString(format(q[i], 'd')))
        else:
            print(err)

class PvEditHML(PvEditTxt):

    def __init__(self, pv, label):
        super(PvEditHML, self).__init__(pv, label)

    def setPv(self):
        value = self.text()
        try:
            q = 0
            for i in range(len(value)):
                q |= frLMH[str(value[i])] << (2 * (len(value) - 1 - i))
            self.pv.put(q)
        except KeyError:
            print("Invalid character in string:", value)

    def update(self, err):
        q = self.pv.__value__
        if err is None:
            v = toLMH[q & 0x3]
            q >>= 2
            while q:
                v = toLMH[q & 0x3] + v
                q >>= 2
            s = QString(v)

            if nogui:
                print(self.pv.pvname,str(s))
            else:
                self.valueSet.emit(s)
        else:
            print(err)

class PvHML(PvEditHML):

    def __init__(self, pv, label):
        super(PvHML, self).__init__(pv, label)
        self.setEnabled(False)

class PvEditDbl(PvEditTxt):

    def __init__(self, pv, label):
        super(PvEditDbl, self).__init__(pv, label)

    def setPv(self):
        value = self.text().toDouble()
        self.pv.put(value)

    def update(self, err):
        q = self.pv.__value__
        if err is None:
            s = QString('fail')
            try:
                s = QString('{:.4f}'.format(q))
            except:
                v = ''
                for i in range(len(q)):
                    v = v + ' {:4f}'.format(q[i])
                s = QString(v)

            if nogui:
                print(self.pv.pvname,str(s))
            else:
                self.valueSet.emit(s)
        else:
            print(err)

class PvDbl(PvEditDbl):

    def __init__(self,pv):
        super(PvDbl, self).__init__(pv, '')
        self.setEnabled(False)

class PvDblArrayW(QtWidgets.QLabel):

    valueSet = QtCore.pyqtSignal('QString',name='valueSet')

    def __init__(self):
        super(PvDblArrayW, self).__init__('-')
        self.connect_signal()

    def connect_signal(self):
        self.valueSet.connect(self.setValue)

    def setValue(self,value):
        self.setText(value)

class PvDblArray:

    def __init__(self, pv, widgets):
        self.widgets = widgets
        initPvMon(self,pv)

    def update(self, err):
        q = self.pv.__value__
        if err is None:
            for i in range(len(q)):
                if nogui==False:
                    self.widgets[i].valueSet.emit(QString(format(q[i], '.4f')))
            if nogui:
                print(self.pv.pvname,q)
        else:
            print(err)


class PvEditCmb(PvComboDisplay):

    def __init__(self, pvname, choices, cb=None):
        super(PvEditCmb, self).__init__(choices)
        self.cb = cb
        self.connect_signal()
        self.currentIndexChanged.connect(self.setValue)
        initPvMon(self,pvname)

    def setValue(self):
        value = self.currentIndex()
        if self.pv.__value__ != value:
            self.pv.put(value)
        else:
            logger.debug("Skipping updating PV for edit combobox as the value of the pv %s is the same as the current value", self.pv.pvname)

    def update(self, err):
        q = self.pv.__value__
        if err is None:
            if nogui:
                print(self.pv.pvname,q)
            else:
                self.setCurrentIndex(q)
                self.valueSet.emit(q)
            if self.cb != None:
                self.cb()
        else:
            print(err)


class PvCmb(PvEditCmb):

    def __init__(self, pvname, choices):
        super(PvCmb, self).__init__(pvname, choices)
        self.setEnabled(False)

class PvIntRow(object):
    def __init__(self, layout, name, pvname, row, length):
        layout.addWidget( QtWidgets.QLabel(name), row, 0 )
        self.cells = []
        for j in range(length):
            lbl = QtWidgets.QLabel()
            layout.addWidget( lbl, row, j+1 )
            self.cells.append(lbl)
        initPvMon(self,pvname)

    def update(self, err):
        q = self.pv.__value__
        for i in range(len(q)):
            if type(q[i]) == int:
                self.cells[i].setText('%d'%q[i])
            else:
                self.cells[i].setText('{0:.4f}'.format(q[i]))

class PvIntTable(QtWidgets.QGroupBox):
    def __init__(self, title, pvbase, pvlist, names, length):
        super(PvIntTable, self).__init__(title)

        lo = QtWidgets.QGridLayout()
        for i,name in enumerate(names):
            PvIntRow( lo, name, pvbase+pvlist[i], i, length )
        self.setLayout(lo)


class PvCString(QtWidgets.QWidget):
    def __init__(self, parent, pvbase, name, dName=None):
        super(PvCString,self).__init__()
        layout = QtWidgets.QHBoxLayout()
        layout.setContentsMargins(0,0,0,0)
        label  = QtWidgets.QLabel(name)
        label.setMinimumWidth(100)
        layout.addWidget(label)
        #layout.addStretch()
        self.__display = PvDisplay()
        self.__display.setWordWrap(True)
        self.__display.connect_signal()
        layout.addWidget(self.__display)
        self.setLayout(layout)
        parent.addWidget(self)

        pvname = pvbase+name
        initPvMon(self,pvname)

    def update(self, err):
        q = self.pv.get()
        if err is None:
            s = QString()
            slen = len(q)
#            if slen > 64:
#                slen = 64
            for i in range(slen):
                if q[i]==0:
                    break
                s += QChar(ord(q[i]))
            self.__display.valueSet.emit(s)
        else:
            print(err)


class PvMask(object):

    def __init__(self, parent, pvname, bits):
        super(PvMask,self).__init__()

        self.chkBox = []
        for i in range(bits):
            chkB = QtWidgets.QCheckBox()
            parent.addWidget( chkB )
            self.chkBox.append(chkB)
            chkB.setEnabled(False)
        initPvMon(self, pvname)

    def update(self, err):
        v = self.pv.__value__
        for i in range(len(self.chkBox)):
            if v & (1<<i):
                self.chkBox[i].setChecked(True)
            else:
                self.chkBox[i].setChecked(False)

class PvMaskTab(QtWidgets.QWidget):

    def __init__(self, pvname, names):
        super(PvMaskTab,self).__init__()

        print('Pv '+pvname)
        self.pv = Pv(pvname)

        self.chkBox = []
        layout = QtWidgets.QGridLayout()
        rows = (len(names)+3)/4
        cols = (len(names)+rows-1)/rows
        for i in range(len(names)):
            layout.addWidget( QtWidgets.QLabel(names[i]), i/cols, 2*(i%cols) )
            chkB = QtWidgets.QCheckBox()
            layout.addWidget( chkB, i/cols, 2*(i%cols)+1 )
            chkB.clicked.connect(self.update)
            self.chkBox.append(chkB)
        self.setLayout(layout)

    def update(self):
        v = 0
        for i in range(len(self.chkBox)):
            if self.chkBox[i].isChecked():
                v |= (1<<i)
        self.pv.put(v)

    #  Reassert PV when window is shown
    def showEvent(self,QShowEvent):
#        self.QWidget.showEvent()
        self.update()

class PvDefSeq(QtWidgets.QWidget):
    valueSet = QtCore.pyqtSignal(int,name='valueSet')

    def __init__(self, pvname):
        super(PvDefSeq,self).__init__()

        lo = QtWidgets.QVBoxLayout()
        self.seqsel = QtWidgets.QComboBox()
        self.seqsel.addItems(['Global_%d'%i for i in range(17)]+['Local'])
        self.seqsel.currentIndexChanged.connect(self.setValue)
        lo.addWidget(self.seqsel)

        seqstack = QtWidgets.QStackedWidget()
        for i in range(17):
            seqstack.addWidget(PvEditCmb(pvname+'_SeqBit', seqGlobal))
#        seqstack.addWidget(PvEditCmb(pvname+'_SeqBit'  ,seqBursts))
#        seqstack.addWidget(PvEditCmb(pvname+'_SeqBit'  ,seqRates))
        seqstack.addWidget(PvEditCmb(pvname+'_SeqBit'  ,seqLocal))
        self.seqsel.currentIndexChanged.connect(seqstack.setCurrentIndex)
        lo.addWidget(seqstack)

        self.setLayout(lo)

        initPvMon(self,pvname+'_Sequence')

    def setValue(self):
        value = self.seqsel.currentIndex()
#        self.pv.put(value+15)  # Defined sequences start at 15
        self.pv.put(value)

    def update(self,err):
        q = self.pv.__value__
        if err is None:
            self.seqsel.setCurrentIndex(q)
            self.valueSet.emit(q)
        else:
            print(err)

class PvDefCuSeq(QtWidgets.QWidget):
    valueSet = QtCore.pyqtSignal(int,name='valueSet')

    def __init__(self, pvname):
        super(PvDefCuSeq,self).__init__()

        lo = QtWidgets.QHBoxLayout()
        lo.addWidget(QtWidgets.QLabel('EventCode'))
        self.ecsel = QtWidgets.QLineEdit('-')
        self.ecsel.editingFinished.connect(self.setValue)
        lo.addWidget(self.ecsel)
        self.setLayout(lo)

        self.pvseq = Pv(pvname+'_Sequence')
        self.pvbit = Pv(pvname+'_SeqBit')

    def setValue(self):
        try:
            value = int(self.ecsel.text())
            self.pvseq.put(value/16)
            self.pvbit.put(value%16)
        except:
            pass

class PvEvtTab(QtWidgets.QStackedWidget):

    def __init__(self, pvname, evtcmb):
        super(PvEvtTab,self).__init__()

        self.addWidget(PvEditCmb(pvname+'_FixedRate',fixedRates))

        acw = QtWidgets.QWidget()
        acl = QtWidgets.QVBoxLayout()
        acl.addWidget(PvEditCmb(pvname+'_ACRate',acRates))
        acl.addWidget(PvMaskTab(pvname+'_ACTimeslot',acTS))
        acw.setLayout(acl)
        self.addWidget(acw)

#        sqw = QtGui.QWidget()
#        sql = QtGui.QVBoxLayout()
##        sql.addWidget(PvEditCmb(pvname+'_Sequence',seqIdxs))
##        sql.addWidget(PvEditCmb(pvname+'_SeqBit',seqBits))
#        sql.addWidget(PvEditCmb(pvname+'_SeqBit'  ,seqRates))
#        sqw.setLayout(sql)
        sqw = PvDefSeq(pvname) if xtpg==False else PvDefCuSeq(pvname)
        self.addWidget(sqw)

        self.setCurrentIndex(evtcmb.currentIndex())
        evtcmb.currentIndexChanged.connect(self.setCurrentIndex)

class PvEditEvt(QtWidgets.QWidget):

    def __init__(self, pvname, idx):
        super(PvEditEvt, self).__init__()
        vbox = QtWidgets.QVBoxLayout()
        print('xtpg',xtpg)
        evtcmb = PvEditCmb(pvname,evtselSc if xtpg==False else evtselCu)
        vbox.addWidget(evtcmb)
        vbox.addWidget(PvEvtTab(pvname,evtcmb))
        self.setLayout(vbox)

class PvDstTab(QtWidgets.QWidget):

    def __init__(self, pvname, cb=None):
        super(PvDstTab,self).__init__()

        self.cb = cb
        initPvMon(self,pvname)

        self.chkBox = []
        layout = QtWidgets.QGridLayout()
        for i in range(NBeamSeq):
            layout.addWidget( QtWidgets.QLabel('D%d'%i), i/4, 2*(i%4) )
            chkB = QtWidgets.QCheckBox()
            layout.addWidget( chkB, i/4, 2*(i%4)+1 )
            chkB.clicked.connect(self.setValue)
            self.chkBox.append(chkB)
        self.setLayout(layout)

    def setValue(self):
        v = 0
        for i in range(NBeamSeq):
            if self.chkBox[i].isChecked():
                v |= (1<<i)
        self.pv.put(v)

    def update(self, err):
        q = self.pv.__value__
        if err is None:
            if nogui:
                print(self.pv.pvname,q)
            else:
                for i in range(NBeamSeq):
                    self.chkBox[i].setChecked(q&(1<<i))
            if self.cb != None:
                self.cb()
        else:
            print(err)

class PvEditDst(QtWidgets.QWidget):

    def __init__(self, pvname, idx):
        super(PvEditDst, self).__init__()

        self.ok_palette = QtGui.QPalette()
        self.errpalette = QtGui.QPalette()
        self.errpalette.setColor(QtGui.QPalette.Window, QtGui.QColor.fromRgb(255,0,0))

        vbox = QtWidgets.QVBoxLayout()
        self.selcmb = PvEditCmb(pvname,dstsel,self.validate)
        vbox.addWidget(self.selcmb)
        self.selmask = PvDstTab(pvname+'_Mask',self.validate)
        vbox.addWidget(self.selmask)
        self.setLayout(vbox)

    def validate(self):
        if self.selcmb.pv.__value__==0 and self.selmask.pv.__value__==0:
            self.selcmb .setPalette(self.errpalette)
            self.selmask.setPalette(self.errpalette)
        else:
            self.selcmb .setPalette(self.ok_palette)
            self.selmask.setPalette(self.ok_palette)

class PvEditTS(PvEditCmb):

    def __init__(self, pvname, idx):
        super(PvEditTS, self).__init__(pvname, ['%u'%i for i in range(16)])

def PvInput(widget, parent, pvbase, name, count=1, start=0, istart=0, enable=True, horiz=True):
    pvname = pvbase+name
    print(pvname)

    if horiz:
        layout = QtWidgets.QHBoxLayout()
    else:
        layout = QtWidgets.QVBoxLayout()
    label  = QtWidgets.QLabel(name)
    label.setMinimumWidth(100)
    layout.addWidget(label)
    #layout.addStretch
    if count == 1:
        w = widget(pvname, '')
        w.setEnabled(enable)
        layout.addWidget(w)
    else:
        for i in range(count):
            w = widget(pvname+'%d'%(i+start), QString(i+istart))
            w.setEnabled(enable)
            layout.addWidget(w)
    #layout.addStretch
    parent.addLayout(layout)

def LblPushButton(parent, pvbase, name, count=1):
    return PvInput(PvPushButton, parent, pvbase, name, count)

def LblCheckBox(parent, pvbase, name, count=1, start=0, istart=0, enable=True, horiz=True):
    return PvInput(PvCheckBox, parent, pvbase, name, count, start, istart, enable, horiz=horiz)

def LblEditInt(parent, pvbase, name, count=1, horiz=True):
    return PvInput(PvEditInt, parent, pvbase, name, count, horiz=horiz)

def LblEditHML(parent, pvbase, name, count=1):
    return PvInput(PvEditHML, parent, pvbase, name, count)

def LblEditTS(parent, pvbase, name, count=1):
    return PvInput(PvEditTS, parent, pvbase, name, count)

def LblEditEvt(parent, pvbase, name, count=1):
    return PvInput(PvEditEvt, parent, pvbase, name, count)

def LblEditDst(parent, pvbase, name, count=1):
    return PvInput(PvEditDst, parent, pvbase, name, count)

def LblMask(pvbase, label, bits=1):
    hbox = QtWidgets.QHBoxLayout()
    hbox.addWidget( QtWidgets.QLabel(label) )
    PvMask(hbox, pvbase+label, bits)
    hbox.addStretch(1)
    return hbox
