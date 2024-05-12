#ifndef TRANSPORTRX
#define TRANSPORTRX

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

//Canal de transporte de informacion de para Nodos Rx
//Debe ser capaz de enviar informacion sobre su buffer a transportTx

class TransportRx: public cSimpleModule {
    private:
        cQueue buffer; //El buffer mismo
        cQueue bufferStatus; //Informacion del buffer

        bool statusSend; Podriamos modularizar;

        cMessage *endServiceEvent;
        cMessage *sendStatusEvent; //Evento que informa a la entidad que envie una respuesta

        int packetsDropped;
        cOutVector bufferSizeVector;
        cOutVector packetDropVector;

        //Funcion protocolo
        void protocol(cMessage *msg);
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

    endServiceEvent = new cMessage("endService");
    sendStatusEvent = new cMessage("sendStatus");
}

void TransportRx::finish(){
    recordScalar("Dropped packets", packetDropVector.getCount()); //No se si esto esta bien
}

void TransportRx::protocol(cMessage *msg){
    const int bufferMaxSize = par("buffersize").intValue();
    int umbral = 0.5 * bufferMaxSize;

    //Si el buffer se encuentra mas alla de su capacidad, borramos el mensaje y aumentamos la cantidad de paquetes dropeados
    if (buffer.getLenth() >= bufferMaxSize){
        delete msg;

        this->bubble("packet dropped");

        packetsDropped++;
        packetDropVector.record(packetsDropped);
    } else {
        //Si el buffer supera el umbral, crea un mensaje de estatus
        if (buffer.getLength() >= umbral){
            cPacket *statusMsg = new cPacket();
            statusMsg->setKind(2); //Setea su tipo en 2
            statusMsg->setByteLength(1); //Asigna su tamaño de 1 byte
            statusMsg->setFullBuffer(true);

            bufferStatus.inster(msg);
            if(!sendStatusEvent->isScheduled()){
                scheduleAt(simTime()+0, sendStatusEvent);
            }
        }
        //Insertamos mensaje al buffer
        buffer.insert(msg);

        if(!endServiceEvent->isScheduled()){
            sheduleAt(simTime() + 0, endServiceEvent);
        }
    }
}
void TransportRx::HandleMessage(cMessage *msg){
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
    } else if (msg == sendStatusEvent){
        if (!bufferStatus.isEmpty()){
            // Si nuestra queue de estado no esta vacia, desencolamos y enviamos como con EndService
            cPacket *statuspkt = (cPacket*) bufferStatus.pop();
            //Mandamos el estado
            send(statuspkt,"toOut$o");

            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, sendStatusEvent);
        }
    } else { // if msg is a data packet
        if (msg->getKind() == 2){
            //Encola los mensajes de tipo Status en el buffer de status
            bufferStatus.inster(msg);
            if(!sendStatusEvent->isScheduled()){
                scheduleAt(simTime()+0, sendStatusEvent);
            }
        } else {
            protocol(msg);
        }
        // enqueue the packet
    }
}

#endif
