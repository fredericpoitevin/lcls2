#include "PV134Ctrls.hh"
#include "Module134.hh"
#include "FexCfg.hh"
#include "Fmc134Cpld.hh"
#include "Fmc134Ctrl.hh"
#include "I2c134.hh"
#include "ChipAdcCore.hh"
#include "ChipAdcReg.hh"
#include "Pgp.hh"
#include "OptFmc.hh"
#include "Jesd204b.hh"

#include "psdaq/epicstools/EpicsPVA.hh"

#include <algorithm>
#include <sstream>
#include <cctype>
#include <stdio.h>
#include <unistd.h>

using Pds_Epics::EpicsPVA;

using Pds::HSD::Jesd204b;
using Pds::HSD::Jesd204bStatus;

namespace Pds {
  namespace HSD {

    PV134Ctrls::PV134Ctrls(Module134& m, Pds::Task& t) :
      PVCtrlsBase(t),
      _m(m)
    {}

    void PV134Ctrls::_allocate()
    {
      _m.i2c_lock(I2cSwitch::PrimaryFmc);
      _m.jesdctl().default_init(_m.i2c().fmc_cpld, _testpattern=0);
      _m.jesdctl().dump();
      _m.i2c_unlock();

      _m.optfmc().qsfp = 0x89;
    }

    void PV134Ctrls::configure(unsigned fmc) {

#define PVGET(name) pv.getScalarAs<unsigned>(#name)
#define PVGETF(name) pv.getScalarAs<float>(#name)

      Pds_Epics::EpicsPVA& pv = *_pv[fmc];

      ChipAdcReg& reg = _m.chip(fmc).reg;
      reg.stop();

      _m.i2c_lock(I2cSwitch::PrimaryFmc);

      unsigned testp = PVGET(test_pattern);
      if (testp!=_testpattern) {
        _m.jesdctl().default_init(_m.i2c().fmc_cpld, 
                                  _testpattern=testp);
        _m.jesdctl().dump();
      }

      _m.i2c().fmc_cpld.adc_range(fmc,PVGET(fs_range_vpp));
      _m.i2c_unlock();

      FexCfg& fex = _m.chip(fmc).fex;

      if (PVGET(enable)==1) {

        reg.init();
        reg.resetCounts();

        { std::vector<Pgp*> pgp = _m.pgp();
          for(unsigned i=4*fmc; i<4*(fmc+1); i++)
            pgp[i]->resetCounts(); }

        unsigned group = PVGET(readoutGroup);
        reg.setupDaq(group);

        _configure_fex(fmc,fex);

        printf("Configure done for chip %d\n",fmc);

        reg.setChannels(1);
        reg.start();
      }

      _ready[fmc]->putFrom<unsigned>(1);
    }

    void PV134Ctrls::reset() {
      for(unsigned i=0; i<2; i++) {
        ChipAdcReg& reg = _m.chip(i).reg;
        reg.resetFbPLL();
        usleep(1000000);
        reg.resetFb ();
        reg.resetDma();
        usleep(1000000);
      }
    }

    void PV134Ctrls::loopback(bool v) {
      std::vector<Pgp*> pgp = _m.pgp();
      for(unsigned i=0; i<pgp.size(); i++)
        pgp[i]->loopback(v);
    }
  };
};
