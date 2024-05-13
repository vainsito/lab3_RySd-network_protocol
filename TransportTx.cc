#ifndef TRANSPORTTX
#define TRANSPORTTX

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class TransportTx: public cSimpleModule {
private:
    cOutVector bufferSizeQueue; // Vector para el tam de el buffer de la cola
    cOutVector packetDropQueue;
    cQueue buffer;
    cMessage *endServiceEvent;
    simtime_t serviceTime;
    float packetRate;
public:
    TransportTx();
    virtual ~TransportTx();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportTx);

TransportTx::TransportTx() {
    endServiceEvent = NULL; // Inicializa el evento de fin de servicio
}

TransportTx::~TransportTx() {
    cancelAndDelete(endServiceEvent); // Cancela y elimina el evento de fin de servicio
}

void TransportTx::initialize() {
    buffer.setName("buffer"); // Nombre del buffer
    bufferSizeQueue.setName("bufferSizeQueue"); // Nombre del vector de tamano del buffer
    packetDropQueue.setName("packetDropQueue"); 
    packetDropQueue.record(0); // Inicializa el vector en 0
    endServiceEvent = new cMessage("endService"); // Creo el mensaje de fin de servicio
    packetRate = 1.0;
}

void TransportTx::finish() {}

void TransportTx::handleMessage(cMessage *msg) {
    // Primer caso, si el mensaje es el evento de fin de servicio
    if (msg == endServiceEvent){
        // Si hay paquetes en el buffer, envia el siguiente
        if (!buffer.isEmpty()) {
            // Desencola el paquete
            cPacket *pkt = (cPacket*) buffer.pop();
            // Envio del paquete
            send(pkt, "toOut$o");
            // Empiezo un nuevo servicio
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, endServiceEvent);
        }
    } else { // Si el mensaje es un paquete de datos
        if (buffer.getLength() >= par("bufferSize").intValue()) {
            // Elimina el paquete
            delete msg;
            this->bubble("packet-dropped");
            packetDropQueue.record(1);
        } else {
            // Encolo el mensaje dependiendo del tipo de paquete
            if (msg->getKind() == 2){
                packetRate = packetRate*2; // Si es un paquete de tipo 2, se duplica la tasa de paquetes
            } else if (msg->getKind() == 3){
                packetRate = packetRate/2; // Si es un paquete de tipo 3, se reduce la tasa de paquetes
            } else{
                // Encolo el paquete
                buffer.insert(msg);
                bufferSizeQueue.record(buffer.getLength());
                // Si el servidor esta inactivo
                if (!endServiceEvent->isScheduled()) {
                    // Inicio el servicio
                    scheduleAt(simTime() + 0, endServiceEvent);
                }
            }
        }
    }
}

#endif
