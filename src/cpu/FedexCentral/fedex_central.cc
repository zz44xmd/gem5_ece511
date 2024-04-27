#include "cpu/FedexCentral/fedex_central.hh"

#include <iostream>

namespace gem5
{

/*************************************************************************/
/************************* Fedex Central General *************************/
/*************************************************************************/

FedexCentral::FedexCentral(const FedexCentralParams &params) :
    ClockedObject(params), o3RequestPort(this), dataPort(this)
{
    std::cout << "Init FedexCentral Station xD" << std::endl;
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

    // o3RequestPort.sendTimingResp(pkt);
    dataPort.sendTimingReq(pkt);

    return true;
}

/*************************************************************************/
/************************* Connection With Memory ************************/
/*************************************************************************/


bool FedexCentral::processReadBack(PacketPtr pkt){
    std::cout << "Processing Read Back xD" << std::endl;
    return true;
}


bool FedexCentral::processWriteBack(PacketPtr pkt){
    std::cout << "Processing Write Back xD" << std::endl;
    return true;
}

} // namespace gem5
