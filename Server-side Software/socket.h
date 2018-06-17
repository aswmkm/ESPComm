#ifndef SOCKET_H
#define SOCKET_H

#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QtNetwork/QTcpSocket>
#include <QDateTime>
#include <QHostAddress>
#include <QTimeZone>
#include "common.h"

class ClientSocket : public QTcpSocket
{
    Q_OBJECT //signals/slots support
public:
    explicit ClientSocket( QTcpSocket * );
    QTcpSocket *getTcpSocket(){ return parent_socket; };
    QString &getDeviceID(){ return s_deviceID; }
    QString getAddress(){ return parent_socket->peerAddress().toString();}
    TRANSMISSION_CONDITION sendMessage( const QString & );
    ~ClientSocket();

public slots:
    void AddToBuffer();
    void onTCPSocketClosed();

signals:
    void forwardString( const QString & ); //Send the compiled string to the server for further processing
    void socketClosed( ClientSocket * );

private:
    QString s_buffer; //the char buffer
    QString s_deviceID;
    QTcpSocket *parent_socket;
    bool b_storingData;
};

#endif // SOCKET_H
