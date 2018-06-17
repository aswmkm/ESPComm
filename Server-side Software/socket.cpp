#include "socket.h"
#include "server.h"

ClientSocket::ClientSocket( QTcpSocket *socket )
{
    parent_socket = socket;
    socket->setParent( this ); //this makes the QTcpSocket a child of this subclassed object (memory management)

    connect(socket, SIGNAL(readyRead()), this, SLOT(AddToBuffer()) );
    connect(socket, SIGNAL(disconnected()), this, SLOT(onTCPSocketClosed()));
    b_storingData = false; //init to false;
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
    QString newData = parent_socket->readAll();
    int length = newData.length();

    if ( !length ) //Just to make sure..
        return;

    if ( newData.contains(CHAR_FLAG_TIME) ) //device is requesting a time update
    {
        //The following command is specifically formatted to work with the ATMC firmware.
        sendMessage( QDateTime::currentDateTime().toString("'th'hh'm'mm's'ss' ty'yyyy' tr'M' td'd").append(CHAR_CR) );
        //
        return; //just end here
    }
    else if ( newData.contains(CHAR_FLAG_STORE_BEGIN) ) //data we are sending is to be stored into the SQL Database, and possibly in local storage
    {
        newData.remove(CHAR_FLAG_STORE_BEGIN);
        b_storingData = true;
    }
    else if ( newData.contains(CHAR_FLAG_STORE_END) ) //end of data storage
    {
        newData.remove(CHAR_FLAG_STORE_END);
        b_storingData = false;
    }
    else if ( newData.contains(CHAR_FLAG_DEVID) )
    {
        newData.remove(CHAR_FLAG_DEVID);
        s_deviceID = newData;
        return; //just end here
    }

    if ( newData.contains(CHAR_NL) || newData.contains(CHAR_CR) ) //we're reading till a newline or carriage return?
    {
        newData.remove(CHAR_NL); //if it exists
        newData.remove(CHAR_CR);
        s_buffer.append(newData);
        emit forwardString( getAddress() + "(" + getDeviceID() + ") says: " + s_buffer ); //send to the console, etc.
        s_buffer.clear(); //empty the buffer
    }
    else
        s_buffer.append(newData); //just add it to the buffer.
}

TRANSMISSION_CONDITION ClientSocket::sendMessage(const QString &msg)
{
    if( msg.length() > MAX_MSG_LENGTH - 1 )
        return TRANSMISSION_CONDITION::ERROR_MSGLEN;

    QByteArray block;
    block.append(msg);

    if ( !msg.endsWith(CHAR_CR) )
        block.append(CHAR_CR); //all messages should end with a carriage return.

    getTcpSocket()->write(block);
    while ( getTcpSocket()->bytesToWrite() ) //do we have bytes that have yet to be written?
    {
       if ( !getTcpSocket()->waitForBytesWritten(/*timeout here*/) )
           return TRANSMISSION_CONDITION::ERROR_GENERIC;
    }
    return TRANSMISSION_CONDITION::ERROR_NONE;
}
