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

        cMessage *endServiceEvent;
        cMessage *sendStatusEvent; //Evento que informa a la entidad que envie una respuesta

        cOutVector bufferSizeVector;
        cOutVector packetDropVector;

        //Funciones
        void sendPacket(); //Funcion para enviar paquetes con informacion
        void sendStatus(); //Funcion para enviar informacion de estado
        void enqueueStatus(cMessage *msg); //Funcion para guardar dentro de buffer Data, los mensajes de status aun no enviados
    public:
        TransportRx();
        virtual ~TransportRx();
    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportRx);

