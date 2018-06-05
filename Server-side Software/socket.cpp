#include "socket.h"

ClientSocket::ClientSocket( QTcpSocket *socket )
{
    parent_socket = socket;
    parent_socket->setParent( this ); //this makes the QTcpSocket a child of this subclassed object (memory management)

    connect(this, SIGNAL(readyRead()), this, SLOT(AddToBuffer()) );
    connect(this, SIGNAL(disconnected()), this, SLOT(onTCPSocketClosed()));
}

ClientSocket::~ClientSocket()
{
    delete parent_socket; //delete the pointer to the Tcp Socket
}

void ClientSocket::onTCPSocketClosed()
{
    parent_socket->close(); //close just in case
    emit socketClosed( this ); //inform the server backend
}

void ClientSocket::AddToBuffer()
{
    QString newData = QString(parent_socket->readAll());
    if ( newData.contains('\n') || newData.contains('\r') ) //we're reading till a newline or carriage return?
    {
        newData.remove('\n');
        newData.remove('\r');
        s_buffer.append(newData);
        emit forwardString( s_buffer ); //send to the console, etc.
        s_buffer.clear(); //empty the buffer
    }
    else
        s_buffer.append(newData); //just add it to the buffer.
}
