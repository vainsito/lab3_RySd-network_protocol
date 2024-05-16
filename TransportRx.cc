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
        cQueue bufferStatus; //Mensajes de estados del buffer

        bool statusSent;

        cMessage *endServiceEvent;
        cMessage *sendStatusEvent; //Evento que informa a la entidad que envie una respuesta

        cOutVector bufferSizeVector;
        cOutVector bufferDropVector;

        simtime_t serviceTime;

        //Funciones
        void protocol1(cMessage *msg);
        void sendPacket();
        void sendStatus();

    public:
        TransportRx();
        virtual ~TransportRx();
    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportRx);

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

    bufferSizeVector.setName("buffer size");
    bufferDropVector.setName("packetDrop");

    bufferDropVector.record(0);
    statusSent = false;

}

void TransportRx::finish(){
}

//La unica diferencia entre el protocolo 1 y 2 es que en el 1 se encuentra el condicional que verifica si msg es mensaje de estado del buffer
void TransportRx::protocol1(cMessage *msg){
    //Si el mensaje es de tipo 2 o 3, lo insertamos en el buffer de estados
    if (msg->getKind() == 2 || msg->getKind() == 3){
        bufferStatus.insert(msg);
        if(!sendStatusEvent->isScheduled()){
            scheduleAt(simTime()+0, sendStatusEvent);
        }
    } else { //Si no, lo insertamos en el buffer de mensajes haciendo el chequeo de si se debe enviar un mensaje de estado
        float umbral = 0.80 * par("bufferSize").intValue();
        float umbral_min = 0.25 * par("bufferSize").intValue();

        if (buffer.getLength() >= umbral && !statusSent){
            //Enviamos mensaje de tipo 2 (Reducir tasa de transmision)
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(2); //Setea su tipo en 2
            statusMsg->setByteLength(20);
            send(statusMsg,"toOut$o");
            statusSent = true;

        } else if (buffer.getLength() < umbral_min && statusSent){
            //Enviamos mensaje de tipo 3 (Aumentar tasa de transmision)
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(3); //Setea su tipo en 3
            statusMsg->setByteLength(20);
            send(statusMsg,"toOut$o"); 
            statusSent = false;
        }
        //Si el buffer no se encuentra en ninguna de las situaciones anteriores, encolamos el mensaje
        buffer.insert(msg);
        bufferSizeVector.record(buffer.getLength());

        if(!endServiceEvent->isScheduled()){
            scheduleAt(simTime() + 0, endServiceEvent);
        }
    }
}

//Funcion que envia paquetes
void TransportRx::sendPacket(){
    if (!buffer.isEmpty()) {
        // dequeue packet
        cPacket *pkt = (cPacket*) buffer.pop();
        //send packet
        send(pkt, "toApp");
        serviceTime = pkt->getDuration();
        scheduleAt(simTime() + serviceTime, endServiceEvent);
    }
}

//Funcion que envia los mensajes de estatus
void TransportRx::sendStatus(){
    if (!bufferStatus.isEmpty()){
        // Si nuestra queue de estado no esta vacia, desencolamos y enviamos como con EndService
        cPacket *statuspkt = (cPacket*) bufferStatus.pop();
        //Mandamos el estado
        send(statuspkt,"toOut$o");

        serviceTime = statuspkt->getDuration();
        scheduleAt(simTime() + serviceTime, sendStatusEvent);
    }
}

void TransportRx::handleMessage(cMessage *msg){
        // if msg is signaling an endServiceEvent
        if (msg == endServiceEvent) {
            // if packet in buffer, send next one
            sendPacket();
        } else if (msg == sendStatusEvent) {
            sendStatus();
        } else { // if msg is a data packet
            if (buffer.getLength() >= par("bufferSize").intValue()){
                delete msg;
                this->bubble("packet dropped");
                bufferDropVector.record(1);
            } else {
                protocol1(msg);
            }
        }
}

#endif /* TRANSPORTRX */
