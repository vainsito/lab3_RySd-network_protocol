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

    int packetsDropped;

    bool fullBufferQueue;

    void protocol2(cMessage *msg);
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
    fullBufferQueue = false;

}


void Queue::finish() {
}

void Queue::protocol2(cMessage *msg){
    const int bufferMaxSize = par("bufferSize").intValue();
    int umbral = 0.8 * bufferMaxSize;

    //Si el buffer se encuentra mas alla de su capacidad, borramos el mensaje y aumentamos la cantidad de paquetes dropeados
    if (buffer.getLength() >= bufferMaxSize){
        delete msg;

        this->bubble("packet dropped");
        packetDropVector.record(1);

    } else {
        //Si el buffer supera el umbral, crea un mensaje de estatus
        if (buffer.getLength() >= umbral && !fullBufferQueue){
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(2); //Setea su tipo en 2
            statusMsg->setByteLength(1); //Asigna su tamaño de 1 byte
            fullBufferQueue = true;
            buffer.insertBefore(buffer.front(), statusMsg);

        } else if (buffer.getLength() >= umbral && fullBufferQueue){
            cPacket *statusMsg = new cPacket("statusMsg");
            statusMsg->setKind(3); //Setea su tipo en 2
            statusMsg->setByteLength(1); //Asigna su tamaño de 1 byte
            fullBufferQueue = false;
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

void Queue::handleMessage(cMessage *msg) {

    bufferSizeVector.record(buffer.getLength());
    // if msg is signaling an endServiceEvent
    if (msg == endServiceEvent) {
        // if packet in buffer, send next one
        if (!buffer.isEmpty()) {
            // dequeue packet
            cPacket *pkt = (cPacket*) buffer.pop();
            // send packet
            send(pkt, "out");
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, endServiceEvent);
        }
    } else { // if msg is a data packet
        protocol2(msg);
        // enqueue the packet
    }
}

#endif /* QUEUE */
