#ifndef __FEDEX_CENTRAL_HH__
#define __FEDEX_CENTRAL_HH__

#include "arch/generic/mmu.hh"
#include "cpu/translation.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/request.hh"
#include "params/FedexCentral.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class FedexCentral : public ClockedObject
{
public:
    FedexCentral(const FedexCentralParams &p);


    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

protected:
    bool processCmd(PacketPtr pkt);
    bool processReadBack(PacketPtr pkt);
    bool processWriteBack(PacketPtr pkt);

// private:


/****************************************************************************/
/************************** Connection With O3 CPU **************************/
/****************************************************************************/
protected:


    class FedexCentralCmdRecv : public ResponsePort
    {
      public:

        FedexCentralCmdRecv(FedexCentral* _fc):
            ResponsePort(_fc->name() + ".o3RequestPort", _fc),
            fc(_fc)
        { }

      protected:

        FedexCentral* fc;
        virtual bool recvTimingReq(PacketPtr pkt) override {
            return fc->processCmd(pkt);
        }
        virtual Tick recvAtomic(PacketPtr pkt) override {return Tick();}
        virtual void recvRespRetry() override {}
        virtual void recvFunctional(PacketPtr pkt) override {}
        virtual AddrRangeList getAddrRanges() const override {return AddrRangeList();}
    };

    FedexCentralCmdRecv o3RequestPort;

    /**************************************************************************/
    /************************* Connection With Memory *************************/
    /**************************************************************************/



    class FedexDataPort : public RequestPort
    {
        public:
            FedexDataPort(FedexCentral* _fc) :
                RequestPort(_fc->name() + ".data_port", _fc), fc(_fc)
            { }

        protected:
            FedexCentral* fc;
            virtual bool recvTimingResp(PacketPtr pkt) override {

                std::cout << "Received pkt from Memory" << std::endl;

                // if (pkt->isRead())
                //     return fc->processReadBack(pkt);
                // else if (pkt->isWrite())
                //     return fc->processWriteBack(pkt);
                // else{
                //     std::cout << "Received a packet that is not a read or write" << std::endl;
                //     return true;
                // }
                return true;
            }
            virtual void recvReqRetry() override {}

    };

    FedexDataPort dataPort;
};

} // namespace gem5

#endif // __FEDEX_CENTRAL_HH__
