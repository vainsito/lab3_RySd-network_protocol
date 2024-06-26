#ifndef QUEUE
#define QUEUE

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Queue: public cSimpleModule {
private:
    cOutVector bufferSizeVector; 
    cOutVector packetDropVector;
    cQueue buffer;
    cMessage *endServiceEvent; 
    simtime_t serviceTime;

    bool statusSentQueue;

    //funciones
    void protocol2(cMessage *msg); //Protocolo de encolamiento
    void sendPacket(); //Envia el paquete

public:
    Queue();
    virtual ~Queue();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Queue);

Queue::Queue() {
    endServiceEvent = NULL;
}

Queue::~Queue() {
    cancelAndDelete(endServiceEvent);
}

void Queue::initialize() {
    buffer.setName("buffer");
    bufferSizeVector.setName("bufferSizeQueue");
    packetDropVector.setName("packets drop");

    packetDropVector.record(0);
    endServiceEvent = new cMessage("endService");
    statusSentQueue = false;

}

void Queue::finish() {
}


void Queue::protocol2(cMessage *msg){
    //Si el buffer se encuentra mas alla de su capacidad, borramos el mensaje
    if (buffer.getLength() >= par("bufferSize").intValue()){
        delete msg;

        this->bubble("packet dropped");
        packetDropVector.record(1);
    } else {
        float umbral = 0.80 * par("bufferSize").intValue();
        float umbral_min = 0.25 * par("bufferSize").intValue();

        
        if (buffer.getLength() >= umbral && !statusSentQueue){ //Si el buffer se encuentra en un estado critico, enviamos un mensaje de estado (tipo 2)
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(2); 
            statusMsg->setByteLength(20);
            statusSentQueue = true;
            buffer.insertBefore(buffer.front(), statusMsg);
        
        } else if (buffer.getLength() < umbral_min && statusSentQueue){ //Si el buffer se encuentra con un tamaño menor al umbral minimo, enviamos un mensaje de estado (tipo 3)
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(3); //Setea su tipo en 3
            statusMsg->setByteLength(20);
            statusSentQueue = false;
            buffer.insertBefore(buffer.front(), statusMsg);
        }

        //Si el buffer no se encuentra en ninguna de las situaciones anteriores, encolamos el mensaje
        buffer.insert(msg);
        bufferSizeVector.record(buffer.getLength());

        if(!endServiceEvent->isScheduled()){
            scheduleAt(simTime() + 0, endServiceEvent);
        }
    }
}

//Funcion que envia el paquete
void Queue::sendPacket(){
    if (!buffer.isEmpty()) {
        // dequeue packet
        cPacket *pkt = (cPacket*) buffer.pop();
        //send packet
        send(pkt, "out");
        serviceTime = pkt->getDuration();
        scheduleAt(simTime() + serviceTime, endServiceEvent);
    }
}

void Queue::handleMessage(cMessage *msg) {

    bufferSizeVector.record(buffer.getLength());
    // if msg is signaling an endServiceEvent
    if (msg == endServiceEvent) {
        sendPacket();
    } else { // if msg is a data packet
        protocol2(msg);
        // enqueue the packet
    }
}

#endif /* QUEUE */
