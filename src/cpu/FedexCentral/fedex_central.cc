#include "cpu/FedexCentral/fedex_central.hh"

#include <iostream>
#include <algorithm>
#include "sim/system.hh"
#include "cpu/o3/cpu.hh"

#define BLOCK_CEILING(x) ((x & (~0x3F)) + 0x40)

namespace gem5
{

/*************************************************************************/
/************************* Fedex Central General *************************/
/*************************************************************************/

FedexCentral::FedexCentral(const FedexCentralParams &params) :
    ClockedObject(params),
    tickEvent([this]{ tick(); }, "FedexCentral tick", false),
    valid(false),
    lastMemReqTick(clockEdge()),
    readBufferEntriesCount(params.readBufferEntries),
    writeBufferEntriesCount(params.writeBufferEntries),
    cpu(params.baseCPU),
    o3RequestPort(this),
    mmu(params.mmu),
    dataPort(this),
    cacheLineSize(params.system->cacheLineSize()),
    _dataRequestorId(params.system->getRequestorId(this, "data"))
{
    std::cout << "[FedexC]Init FedexCentral Station xD" << std::endl;
    std::cout << "[FedexC]Data Requestor ID: "  << dataRequestorId() << std::endl;

    if (!mmu){
        std::cout << "[FedexC]Fedex has to have a MMU" << std::endl;
        assert(false);
    }

    if (!cpu){
        std::cout << "[FedexC]Fedex has to have a related CPU, for Version 1" << std::endl;
        assert(false);        
    }

}


Port& FedexCentral::getPort(const std::string &if_name, PortID idx)
{
    // Get the right port based on name. This applies to all the
    // subclasses of the base CPU and relies on their implementation
    // of getDataPort and getInstPort.
    if (if_name == "o3RequestPort")
        return o3RequestPort;
    else if (if_name == "dataPort")
        return dataPort;
    else
        return ClockedObject::getPort(if_name, idx);
}



/*************************************************************************/
/******************** Fedex Central Command Processing *******************/
/*************************************************************************/
bool FedexCentral::processCmd(PacketPtr pkt){
    std::cout << "Processing Command xD" << std::endl;

    if (valid){
        panic("Still processing previous Fedex Command"); 
    }

    //** Testing Beta Part *******************************
    uint64_t* data = pkt->getPtr<uint64_t>();
    srcAddr = data[0];
    dstAddr = data[1];
    sizeByte = data[2];

    std::cout << "FedexCentral Received addrSrc: " << std::hex << srcAddr << std::endl;
    std::cout << "FedexCentral Received addrDest: " << std::hex << dstAddr << std::endl;
    std::cout << "FedexCentral Received sizeByte: " << std::dec << sizeByte << std::endl;

    valid = true;
    //****************************************************

    //** Add to Fedex Command Buffer
    //!TOOD

    curSrcAddr = srcAddr;
    curDstAddr = dstAddr;
    remainSizeByte = sizeByte;

    if (!tickEvent.scheduled()) {
        schedule(tickEvent, clockEdge(Cycles(1)));
    }
    return true;
}

void FedexCentral::tick(){
    if (!valid) return;
    // std::cout << "Tick xD" << std::endl;

    //** Retry the failed operations
    retryFailedRead();
    retryFailedWrite();

    //** Add to Read Buffer and Send to Memory
    sendToReadTranslate();

    //** Transfer Finished Read to Write. Handle MisAligned Case
    // updateWriteBuffer();

    //** Add to Write Buffer and Send to Memory
    sendWriteBuffer();


    if (!tickEvent.scheduled()) {
        schedule(tickEvent, clockEdge(Cycles(1)));
    }
}


void FedexCentral::sendToReadTranslate(){

    //** We don't pressure read buffer if it's full
    if (readBuffer.size() >= readBufferEntriesCount)
        return;
    if (remainSizeByte <= 0)
        return;

    //** Find Request Size
    uint64_t src2AlignSize = BLOCK_CEILING(curSrcAddr) - curSrcAddr;
    uint64_t req_size = std::min(remainSizeByte, std::min(src2AlignSize, static_cast<uint64_t>(64)));

    //** Construct Request
    //!Don't Care about CONTEXT
    Request::FlagsType flags = 0;
    BaseMMU::Mode mode = BaseMMU::Read;
    RequestPtr req = std::make_shared<Request>(
        curSrcAddr, req_size, flags, dataRequestorId(), pc, 0); 

    //** Send to MMU
    WholeTranslationState *state = 
        new WholeTranslationState(req, new uint8_t[req_size], NULL, mode);
    DataTranslation<FedexCentral*> *translation
        = new DataTranslation<FedexCentral*>(this, state);

    std::cout << "Translation request sent " << std::hex << curSrcAddr << " " << std::dec << req_size << std::endl;
    mmu->translateTiming(req, cpu->threadContexts[0], translation, mode);

    //** Update FedexCentral State
    readBuffer.push(req);
    remainSizeByte -= req_size;
    curSrcAddr += req_size;
}


void FedexCentral::retryFailedRead(){

    if (retryReadBuffer.empty())
        return;
    else if (clockEdge() == lastMemReqTick)
        return;

    std::cout << "Retry Failed Read xD" << std::endl;
    PacketPtr pkt = retryReadBuffer.front();
    if (dataPort.sendTimingReq(pkt)){
        retryReadBuffer.pop();
        lastMemReqTick = clockEdge();
    }
}

void FedexCentral::retryFailedWrite(){

    if (retryWriteBuffer.empty())
        return;
    else if (clockEdge() == lastMemReqTick)
        return;

    std::cout << "Retry Failed Write xD" << std::endl;
    PacketPtr pkt = retryWriteBuffer.front();
    if (dataPort.sendTimingReq(pkt)){
        retryWriteBuffer.pop();
        lastMemReqTick = clockEdge();
    }
}


void FedexCentral::sendToWriteTranslate(){

    //** We don't pressure write buffer if it's full
    if (writeBuffer.size() >= writeBufferEntriesCount)
        return;

}

bool FedexCentral::updateWriteBuffer(PacketPtr pkt){
    //!TODO
    std::cout << "Received packet from memory:" << std::endl;
    std::cout << "Command: " << pkt->cmdString() << std::endl;
    std::cout << "Address: 0x" << std::hex << pkt->getAddr() << std::dec << std::endl;
    std::cout << "Size: " << pkt->getSize() << " bytes" << std::endl;

    uint8_t* data = pkt->getPtr<uint8_t>();
    std::cout << "Data: ";
    for (unsigned i = 0; i < pkt->getSize(); i++) {
        std::cout << std::hex << static_cast<uint32_t>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    return true;
}

void FedexCentral::sendWriteBuffer(){
    //!TODO
}

/*************************************************************************/
/************************* Connection With Memory ************************/
/*************************************************************************/
void FedexCentral::sendToMemory(const RequestPtr &req, uint8_t *data, uint64_t *res,
                          bool read){

    std::cout << "Send to Memory xD" << std::endl;

    PacketPtr pkt = read ? Packet::createRead(req) : Packet::createWrite(req);

    pkt->dataDynamic<uint8_t>(data);

    //** Already has a request out this cycle
    if (clockEdge() == lastMemReqTick){
        retryReadBuffer.push(pkt);
        return;
    }

    if (dataPort.sendTimingReq(pkt)){
        lastMemReqTick = clockEdge();
    } else if (read){
        retryReadBuffer.push(pkt);
    } else {
        retryWriteBuffer.push(pkt);
    }
}


void FedexCentral::finishTranslation(WholeTranslationState* state){
    std::cout << "Finish Translation xD" << std::endl;
    assert(valid);

    if (state->getFault() != NoFault){
        std::cout << "Whaaaaaat>>>>??? Translation Fault xD -- " << state->getFault() << std::endl;
        delete [] state->data;
        state->deleteReqs();
        assert(false);
    } else {
        sendToMemory(state->mainReq, state->data, state->res, state->mode == BaseMMU::Read);
    }
}

bool FedexCentral::isSquashed(){
    std::cout << "Is Squashed xD" << std::endl;
    //!Don't Care
    return false;
}


bool FedexCentral::processReadBack(PacketPtr pkt){

    std::cout << "Processing Read Back xD" << std::endl;

    //!TODO



    return true;
}


bool FedexCentral::processWriteBack(PacketPtr pkt){
    std::cout << "Processing Write Back xD" << std::endl;
    return true;
}

    //!TODO

} // namespace gem5
