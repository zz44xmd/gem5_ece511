#ifndef __FEDEX_CENTRAL_HH__
#define __FEDEX_CENTRAL_HH__

#include "arch/generic/mmu.hh"
#include "cpu/translation.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/request.hh"
#include "params/FedexCentral.hh"
#include "sim/clocked_object.hh"



#include <queue>

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

    void tick();
    void sendToReadTranslate();
    void sendToWriteTranslate(PacketPtr readPkt);
    bool updateWriteBuffer(PacketPtr pkt);
    void sendWriteBuffer();

    void retryFailedRead();
    void retryFailedWrite();

    EventFunctionWrapper tickEvent;

private:
    //** General Control
    bool valid;
    Tick lastMemReqTick;

    //** Fedex Command Buffer
    Addr pc;
    Addr srcAddr;
    Addr dstAddr;
    uint64_t sizeByte;

    Addr curSrcAddr;
    Addr curDstAddr;
    uint64_t remainSizeByte;
    uint64_t remainSizeByte_write;

    //** Fedex Read Buffer
    std::queue<RequestPtr> readBuffer;
    std::queue<RequestPtr> writeBuffer;
    std::queue<PacketPtr> retryReadBuffer;
    std::queue<PacketPtr> retryWriteBuffer;

    unsigned readBufferEntriesCount;
    unsigned writeBufferEntriesCount;

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

    BaseCPU* cpu;
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
                fc->updateWriteBuffer(pkt);
                return true;
            }
            virtual void recvReqRetry() override {}
    };

public:
    void sendToMemory(const RequestPtr&, uint8_t*, uint64_t*, bool);
    void finishTranslation(WholeTranslationState *);
    bool isSquashed();

protected:
    /** Reads this CPU's unique data requestor ID */
    RequestorID dataRequestorId() const { return _dataRequestorId; }


private:
    BaseMMU *mmu; 
    FedexDataPort dataPort;
    unsigned int cacheLineSize;
    RequestorID _dataRequestorId;
};

} // namespace gem5

#endif // __FEDEX_CENTRAL_HH__
