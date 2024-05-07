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

    // if (valid){
    //     panic("Still processing previous Fedex Command"); 
    // }

    //** Testing Beta Part *******************************
    uint64_t* data = pkt->getPtr<uint64_t>();
    srcAddr = data[0];
    dstAddr = data[1];
    sizeByte = data[2];
    numRead = 0;
    numWriteDone = 0;
    tried = false;
    writesent = false;


    std::cout << "FedexCentral Received addrSrc: " << std::hex << srcAddr << std::endl;
    std::cout << "FedexCentral Received addrDest: " << std::hex << dstAddr << std::endl;
    std::cout << "FedexCentral Received sizeByte: " << std::dec << sizeByte << std::endl;
    std::cout << "[!!] begin clockedge is " << clockEdge() << std::endl;
    std::cout << std::endl;
    valid = true;
    //****************************************************

    //** Add to Fedex Command Buffer
    //!TOOD

    curSrcAddr = srcAddr;
    curDstAddr = dstAddr;
    remainSizeByte = sizeByte;
    remainSizeByte_write = sizeByte;

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
    // sendToWriteTranslate();

    //** Add to Write Buffer and Send to Memory
    sendWriteBuffer();
    // oneCycleDelayforWrite();

    processCmdDone();


    if (!tickEvent.scheduled()) {
        if (writesent == true){
            schedule(tickEvent, clockEdge(Cycles(2)));
            writesent = false;
        }
        else 
        {
           schedule(tickEvent, clockEdge(Cycles(1))); 
        }
    }
}


void FedexCentral::sendToReadTranslate(){

    //** We don't pressure read buffer if it's full
    if (retryReadBuffer.size() >= readBufferEntriesCount)
        return;
    if (remainSizeByte <= 0)
        return;

    //** Find Request Size
    uint64_t src2AlignSize = BLOCK_CEILING(curSrcAddr) - curSrcAddr;
    uint64_t req_size = std::min(remainSizeByte, std::min(src2AlignSize, static_cast<uint64_t>(64)));
    std::cout << "remainSizeByte " << std::dec << remainSizeByte << std::endl;
    // std::cout << "src2AlignSize " << std::dec << src2AlignSize << std::endl;
    // std::cout << "req_size " << std::dec << req_size << std::endl;

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

    std::cout << "[-] Read Translation request sent " << std::hex << curSrcAddr << " " << std::dec << req_size << std::endl;
    std::cout << "[-] clockEdge is " << clockEdge() << std::endl;
    std::cout << std::endl;
    mmu->translateTiming(req, cpu->threadContexts[0], translation, mode);

    //** Update FedexCentral State
    // readBuffer.push(req);
    numRead += 1;
    tried = true;
    remainSizeByte -= req_size;
    curSrcAddr += req_size;
}


void FedexCentral::retryFailedRead(){

    if (retryReadBuffer.empty())
        return;
    else if (clockEdge() == lastMemReqTick)
        return;
    std::cout << "[-] Retry Failed Read xD" << std::endl;
    std::cout << "[-] clockEdge is " << clockEdge() << std::endl;
    PacketPtr pkt = retryReadBuffer.front();
    if (dataPort.sendTimingReq(pkt)){
        std::cout << "[-] Read Request sent from retryReadBuffer" << std::endl;
        std::cout << std::endl;
        retryReadBuffer.pop();
        lastMemReqTick = clockEdge();
    }
    else {
        std::cout << "[-] Failed to send request from retryReadBuffer" << std::endl;
        std::cout << std::endl;
    }
}

void FedexCentral::retryFailedWrite(){

    if (retryWriteBuffer.empty())
        return;
    else if (clockEdge() == lastMemReqTick)
        return;

    std::cout << "[+] Retry Failed Write xD" << std::endl;
    std::cout << "[+] clockEdge is " << clockEdge() << std::endl;
    std::cout << std::endl;
    PacketPtr pkt = retryWriteBuffer.front();
    if (dataPort.sendTimingReq(pkt)){
        std::cout << "[+] Write Request sent from retryWriteBuffer" << std::endl;
        std::cout << std::endl;
        retryWriteBuffer.pop();
        lastMemReqTick = clockEdge();
    }
    else{
        std::cout << "[+] Failed to send request from retryWriteBuffer" << std::endl;
        std::cout << std::endl;
    }
}


void FedexCentral::sendToWriteTranslate(PacketPtr readPkt){

    //** We don't pressure write buffer if it's full
    if (retryWriteBuffer.size() >= writeBufferEntriesCount)
        return;
    if (remainSizeByte_write <= 0)
        return;
   
    uint64_t dst2AlignSize = BLOCK_CEILING(curDstAddr) - curDstAddr;
    uint64_t write_size = std::min(remainSizeByte_write, std::min(dst2AlignSize, static_cast<uint64_t>(64)));
    // if (write_size == readPkt->getSize()){       
    // }
    std::cout << "[++] dst2AlignSize is " << std::dec << dst2AlignSize << std::endl;
    std::cout << "static_cast<uint64_t>(64) " << std::dec << static_cast<uint64_t>(64) << std::endl;
    std::cout << "remainSizeByte_write is " << std::dec << remainSizeByte_write << std::endl;
    write_size = readPkt->getSize();
    std::cout << "readPkt size is " << std::dec << readPkt->getSize() << std::endl;

    // std::cout << "remainSizeByte_write " << std::dec << remainSizeByte_write << std::endl;
    // std::cout << "dst2AlignSize " << std::dec << dst2AlignSize << std::endl;
    // std::cout << "static_cast<uint64_t>(64) " << std::dec << static_cast<uint64_t>(64) << std::endl;
    // std::cout << "write_size " << std::dec << write_size << std::endl;


    Request::FlagsType flags = 0;
    BaseMMU::Mode mode = BaseMMU::Write;
    uint8_t* newData = new uint8_t[write_size];
    uint8_t* readData = readPkt->getPtr<uint8_t>();
    if (readData == NULL) {
        assert(flags & Request::STORE_NO_DATA);
        // This must be a cache block cleaning request
        memset(newData, 0, write_size);
    } else {
        memcpy(newData, readData, write_size);
    }
    for (unsigned int i = 0; i < write_size; i++) {
        std::cout << std::hex << static_cast<uint32_t>(newData[i]) << " ";
    }
    std::cout << std::endl;
        
    RequestPtr writeReq = std::make_shared<Request>(
        curDstAddr, write_size, flags, dataRequestorId(), pc, 0);

    WholeTranslationState* state =
        new WholeTranslationState(writeReq, newData, NULL, mode);
    DataTranslation<FedexCentral*>* translation =
        new DataTranslation<FedexCentral*>(this, state);
    std::cout << "[+] Write Translation request sent " << std::hex << curDstAddr << " " << std::dec << write_size << std::endl;
    std::cout << "[+] clockEdge is " << clockEdge() << std::endl;
    std::cout << std::endl;
    mmu->translateTiming(writeReq, cpu->threadContexts[0], translation, mode);

    // writeBuffer.push(writeReq);
    remainSizeByte_write -= write_size;
    curDstAddr += write_size;
}


bool FedexCentral::updateWriteBuffer(PacketPtr pkt){
    //!TODO
    if (remainSizeByte_write <= 0){
        valid = false;
        std::cout << "[!!] Program finish " << std::endl;
        std::cout << "[!!] Clockedge is " << clockEdge() << std::endl;
        std::cout << std::endl;
        return true;
    }
    if (pkt->isRead()) {
        std::cout << "[-] Received read response from memory:" << std::endl;
        std::cout << "[-] clockEdge is " << clockEdge() << std::endl;
        // std::cout << std::endl;
        std::cout << "Address: 0x" << std::hex << pkt->getAddr() << std::dec << std::endl;
        // std::cout << "Size: " << pkt->getSize() << " bytes" << std::endl;

        // uint8_t* data = pkt->getPtr<uint8_t>();
        // std::cout << "Data: ";
        // for (unsigned i = 0; i < pkt->getSize(); i++) {
        //     std::cout << std::hex << static_cast<uint32_t>(data[i]) << " ";
        // }
        std::cout << std::dec << std::endl;
        std::cout << std::endl;
        sendToWriteTranslate(pkt);
        std::cout << "sendToWriteTranslate call" << std::endl;
        std::cout << "[+] clockEdge is " << clockEdge() << std::endl;

    } else {
        std::cout << "[+] Received write response from memory" << std::endl;
        std::cout << "[+] clockEdge is " << clockEdge() << std::endl;
        numWriteDone += 1;
    }

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
    if (!retryReadBuffer.empty() && read){
        retryReadBuffer.push(pkt);
        std::cout << std::endl;
        std::cout << "[-] move to retryReadBuffer beacuase of retryReadBuffer not empty" << std::endl;
        std::cout << "[-] clockEdge is " << clockEdge() << std::endl;
        std::cout << std::endl;
        return;
    }
    if (!retryWriteBuffer.empty() &&!read){
        retryWriteBuffer.push(pkt);
        std::cout << "[+] move to retryWriteBuffer beacuase of retryWriteBuffer not empty" << std::endl;
        std::cout << "[+] clockEdge is " << clockEdge() << std::endl;
        std::cout << std::endl;
        return;
    }
    //** Already has a request out this cycle
    if (clockEdge() == lastMemReqTick){
        if (read){
            retryReadBuffer.push(pkt);
            std::cout << "[-] move to retryReadBuffer Already has a request out this cycle" << std::endl;
            std::cout << "[-] clockEdge is " << clockEdge() << std::endl;
            std::cout << std::endl;
        } else {
            retryWriteBuffer.push(pkt);
            std::cout << "[+] move to retryWriteBuffer Already has a request out this cycle" << std::endl;
            std::cout << "[+] clockEdge is " << clockEdge() << std::endl;
            std::cout << std::endl;
        }
        return;
    }
    std::cout << "clockEdge != lastMemReqTick" << std::endl;
    std::cout << "lastMemReqTick " << std::dec << lastMemReqTick << std::endl;
    std::cout << "clockEdge is " << std::dec << clockEdge() << std::endl;
    if (dataPort.sendTimingReq(pkt)){
        lastMemReqTick = clockEdge();
        if (read){
            std::cout << "[-] Read Request sent" << std::endl;
            std::cout << std::endl;
        }
        else {
            std::cout << "[+] Write Request sent" << std::endl;
            std::cout << std::endl;
            writesent = true;
        }
    } else if (read){
        retryReadBuffer.push(pkt);
        std::cout << "[-] sendTimingReq fails, RetryRead Request sent" << std::endl;
        std::cout << "[-] clockEdge is " << clockEdge() << std::endl;
        std::cout << std::endl;
    } else {
        retryWriteBuffer.push(pkt);
        std::cout << "[+] sendTimingReq fails, RetryWrite Request sent" << std::endl;
        std::cout << "[+] clockEdge is " << clockEdge() << std::endl;
        std::cout << std::endl;
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
        // Addr physAddr = state->mainReq->getPaddr();
        // uint64_t alignedAddr = BLOCK_CEILING(physAddr);
        // uint64_t alignSize = alignedAddr - physAddr;
        // std::cout << "[!!]alignedAddr " << std::hex << alignedAddr << std::dec << std::endl;
        // std::cout << "alignSize " << std::hex << alignSize << std::dec << std::endl;
        // std::cout << std::endl;
        // uint64_t reqSize = std::min(static_cast<uint64_t>(state->mainReq->getSize()), std::min(alignSize, static_cast<uint64_t>(64)));
        // uint64_t write_size = std::min(remainSizeByte_write, std::min(dst2AlignSize, static_cast<uint64_t>(64)));
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


void FedexCentral::processCmdDone(){
    if (tried && (numRead == numWriteDone)){
        valid = false;
        std::cout << "FedexCentral Command Done xD" << std::endl;
    }
}

} // namespace gem5