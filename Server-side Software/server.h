//
// This file contains class definitions for the server backend stuff.
// This class is used to load certain settings from a local file, and store values for later reference by the rest of the program.
// Settings include: SQL Hostname, SQL Port, TCP receive ports
//
#ifndef SERVER_H
#define SERVER_H

#include <QFile>
#include <QVector>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QTextStream>
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include "socket.h"
#include "common.h"

class BackendServer : public QObject
{
    Q_OBJECT
public:
    BackendServer();
    ~BackendServer();
    bool LoadConfigFromFile( const QString & ); //Loads the confguration data from a specific file
    //uint &GetSQLPort(){ return i_sqlPort; } //Return the SQL port number.
    //uint &GetTCPPort(){ return i_tcpPort; } //Return the TCP port number
    void printToConsole( QString, uint = VERBOSE_PRIORITY::PRIORITY_LOW ); //This sends text to the terminal thread.
    //QString &GetSQLHostname(){ return settingsMap->value(); } //Return the hostname string
    bool beginSQLConnection( uint ); // This function attempts to establish the connection to the SQL server where data is stored. Settings are class member variables.
    bool beginTCPServer( uint ); // This function allows TCP/IP connections (for data reception) to be made to the host device. Settings are class member variables.

private: //member variables and whatnots - mostly config related
    QSqlDatabase *p_Database; // database pointer
    QTcpServer *p_Server; // server pointer
    QMap<QString, QString> *settingsMap; //store settings strings here.

    QVector<ClientSocket *> p_Sockets; //a list of all of the client sockets

public slots: //data received from other objects
    void parseConsoleMessage( QString );
    void onClientConnected(); //Handles the connection of a new client socket
    void onClientDisconnected( ClientSocket * ); //Handles the disconnection of a client socket
    void onClientCommunication( QString ); //This handles the data from connected client sockets.


signals: //data sent to other objects in the project (most likely ust the user interface)
    void printMessage( QString );
};

#endif // SERVER_H