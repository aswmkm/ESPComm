#ifndef SOCKET_H
#define SOCKET_H

#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QtNetwork/QTcpSocket>

class ClientSocket : public QTcpSocket
{
    Q_OBJECT //signals/slots support
public:
    explicit ClientSocket( QTcpSocket * );
    QTcpSocket *getTcpSocket(){ return parent_socket; };
    ~ClientSocket();

public slots:
    void AddToBuffer();
    void onTCPSocketClosed();

signals:
    void forwardString( const QString & ); //Send the compiled string to the server for further processing
    void socketClosed( ClientSocket * );

private:
    QString s_buffer; //the char buffer
    QTcpSocket *parent_socket;
};

#endif // SOCKET_H
