#ifndef TRANSPORT_RX
#define TRANSPORT_RX

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

//Canal de transporte de informacion de para Nodos Rx
//Debe ser capaz de enviar informacion sobre su buffer a transportTx

class TransportRx: public cSimpleModule {
    private:
        cQueue buffer; //El buffer mismo
        cQueue bufferStatus; //Informacion del buffer

        bool fullBuffer;

        cMessage *endServiceEvent;
        cMessage *sendStatusEvent; //Evento que informa a la entidad que envie una respuesta

        int packetsDropped;
        cOutVector bufferSizeVector;
        cOutVector bufferDropVector;

        simtime_t serviceTime;

        //Funcion protocolo
        void protocol1(cMessage *msg);
    public:
        TransportRx();
        virtual ~TransportRx();
    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportRx);

//Definicion metodos protected

TransportRx::TransportRx(){
    endServiceEvent = NULL;
    sendStatusEvent = NULL;
}

TransportRx::~TransportRx(){
    cancelAndDelete(endServiceEvent);
    cancelAndDelete(sendStatusEvent);
}

void TransportRx::initialize(){
    buffer.setName("Receptor buffer");
    bufferStatus.setName("Receptor status buffer");

    packetsDropped = 0;
    endServiceEvent = new cMessage("endService");
    sendStatusEvent = new cMessage("sendStatus");

    bufferSizeVector.setName("buffer size");
    bufferDropVector.setName("packetDrop");

    bufferDropVector.record(0);
    fullBuffer = false;


}

void TransportRx::finish(){
}

void TransportRx::protocol1(cMessage *msg){
    if (msg->getKind() == 2 || msg->getKind() == 3){
        bufferStatus.insert(msg);
        if(!sendStatusEvent->isScheduled()){
            scheduleAt(simTime()+0, sendStatusEvent);
        }
    } else {
        const int bufferMaxSize = par("bufferSize").intValue();
        int umbral = 0.8 * bufferMaxSize;

        //Si el buffer se encuentra mas alla de su capacidad, borramos el mensaje y aumentamos la cantidad de paquetes dropeados
            //Si el buffer supera el umbral, crea un mensaje de estatus
        if (buffer.getLength() >= umbral && !fullBuffer){
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(2); //Setea su tipo en 2
            statusMsg->setByteLength(20); //Asigna su tamaño de 1 byte
            buffer.insertBefore(buffer.front(), statusMsg);
            fullBuffer = true;

        } else if (buffer.getLength() >= umbral && fullBuffer){
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(3); //Setea su tipo en 2
            statusMsg->setByteLength(20); //Asigna su tamaño de 1 byte
            fullBuffer = false;
            buffer.insertBefore(buffer.front(), statusMsg);
        }
            //Insertamos mensaje al buffer
        buffer.insert(msg);
        bufferSizeVector.record(buffer.getLength());

        if(!endServiceEvent->isScheduled()){
            scheduleAt(simTime() + 0, endServiceEvent);
        }
    }
}
void TransportRx::handleMessage(cMessage *msg){
        // if msg is signaling an endServiceEvent
        if (msg == endServiceEvent) {
            // if packet in buffer, send next one
            if (!buffer.isEmpty()) {
                // dequeue packet
                cPacket *pkt = (cPacket*) buffer.pop();
                // send packet
                send(pkt, "toApp");
                serviceTime = pkt->getDuration();
                scheduleAt(simTime() + serviceTime, endServiceEvent);
            }
        } else if (msg == sendStatusEvent) {
            if (!bufferStatus.isEmpty()){
                // Si nuestra queue de estado no esta vacia, desencolamos y enviamos como con EndService
                cPacket *statuspkt = (cPacket*) bufferStatus.pop();
                //Mandamos el estado
                send(statuspkt,"toOut$o");

                serviceTime = statuspkt->getDuration();
                scheduleAt(simTime() + serviceTime, sendStatusEvent);
            }
        } else { // if msg is a data packet
            if (buffer.getLength() >= par("bufferSize").intValue()){
                delete msg;
                this->bubble("packet dropped");
                bufferDropVector.record(1);
            } else {
                protocol1(msg);
            }
        }
        // enqueue the packet
}

#endif
