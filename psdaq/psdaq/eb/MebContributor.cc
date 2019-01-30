#include "psdaq/eb/MebContributor.hh"

#include "psdaq/eb/Endpoint.hh"
#include "psdaq/eb/EbLfClient.hh"

#include "psdaq/eb/utilities.hh"

#include "xtcdata/xtc/Dgram.hh"

#include <string.h>
#include <cstdint>
#include <string>

using namespace XtcData;
using namespace Pds::Eb;


MebContributor::MebContributor(const MebCtrbParams& prms) :
  _maxEvSize (roundUpSize(prms.maxEvSize)),
  _maxTrSize (prms.maxTrSize),
  _trSize    (roundUpSize(TransitionId::NumberOf * _maxTrSize)),
  _transport (new EbLfClient(prms.verbose)),
  _links     (),
  _id        (prms.id),
  _verbose   (prms.verbose),
  _eventCount(0)
{
  size_t regionSize = prms.maxEvents * _maxEvSize;

  _initialize(__func__, prms.addrs, prms.ports, prms.id, regionSize);
}

MebContributor::~MebContributor()
{
  if (_transport)
  {
    for (auto it = _links.begin(); it != _links.end(); ++it)
    {
      _transport->shutdown(it->second);
    }
    _links.clear();
    delete _transport;
  }
}

void MebContributor::_initialize(const char*                     who,
                                 const std::vector<std::string>& addrs,
                                 const std::vector<std::string>& ports,
                                 unsigned                        id,
                                 size_t                          rmtRegSize)
{
  for (unsigned i = 0; i < addrs.size(); ++i)
  {
    const char*    addr = addrs[i].c_str();
    const char*    port = ports[i].c_str();
    EbLfLink*      link;
    const unsigned tmo(120000);         // Milliseconds
    if (_transport->connect(addr, port, tmo, &link))
    {
      fprintf(stderr, "%s: Error connecting to EbLfServer at %s:%s\n",
              who, addr, port);
      abort();
    }
    if (link->preparePoster(id, rmtRegSize))
    {
      fprintf(stderr, "%s: Failed to prepare link to %s:%s\n",
              who, addr, port);
      abort();
    }
    _links[link->id()] = link;

    printf("%s: EbLfServer ID %d connected\n", who, link->id());
  }
}

int MebContributor::post(const Dgram* ddg, uint32_t destination)
{
  unsigned  dst    = ImmData::src(destination);
  uint32_t  idx    = ImmData::idx(destination);
  size_t    sz     = sizeof(*ddg) + ddg->xtc.sizeofPayload();
  unsigned  offset = _trSize + idx * _maxEvSize;
  EbLfLink* link   = _links[dst];
  uint32_t  data   = ImmData::value(ImmData::Buffer, _id, idx);

  if (sz > _maxEvSize)
  {
    fprintf(stderr, "%s:\n  L1Accept of size %zd is too big for target buffer of size %zd\n",
            __PRETTY_FUNCTION__, sz, _maxEvSize);
    return -1;
  }

  if (_verbose)
  {
    uint64_t pid    = ddg->seq.pulseId().value();
    unsigned ctl    = ddg->seq.pulseId().control();
    void*    rmtAdx = (void*)link->rmtAdx(offset);
    printf("MebCtrb posts %6ld       monEvt [%4d]  @ "
           "%16p, ctl, %02d, pid %014lx, sz %4zd, MEB %2d @ %16p, data %08x\n",
           _eventCount, idx, ddg, ctl, pid, sz, link->id(), rmtAdx, data);
  }

  if (int rc = link->post(ddg, sz, offset, data) < 0)  return rc;

  ++_eventCount;

  return 0;
}

int MebContributor::post(const Dgram* ddg)
{
  size_t              sz  = sizeof(*ddg) + ddg->xtc.sizeofPayload();
  TransitionId::Value tr  = ddg->seq.service();
  uint64_t            ofs = tr * _maxTrSize;

  if (sz > _maxTrSize)
  {
    fprintf(stderr, "%s:\n  %s transition of size %zd is too big for target buffer of size %zd\n",
            __PRETTY_FUNCTION__, TransitionId::name(tr), sz, _maxTrSize);
    return -1;
  }

  for (auto it = _links.begin(); it != _links.end(); ++it)
  {
    EbLfLink* link = it->second;
    uint32_t  data = ImmData::value(ImmData::Transition, _id, tr);

    if (_verbose)
    {
      uint64_t pid    = ddg->seq.pulseId().value();
      unsigned ctl    = ddg->seq.pulseId().control();
      void*    rmtAdx = (void*)link->rmtAdx(ofs);
      printf("MebCtrb posts %6ld         trId [%4d]  @ "
             "%16p, ctl %02x, pid %014lx, sz %4zd, MEB %2d @ %16p, data %08x\n",
             _eventCount, tr, ddg, ctl, pid, sz, link->id(), rmtAdx, data);
    }

    if (int rc = link->post(ddg, sz, ofs, data) < 0)  return rc;
  }

  return 0;
}
