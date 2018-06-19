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
#include <QDataStream>
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QSqlQuery>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include "socket.h"
#include "common.h"

enum CONFIGURATION_TYPE
{
    TYPE_STRING = 0,
    TYPE_INT
};

enum CONFIG_SETVALUE
{
    ERROR_FAILED = 0, //false
    SUCCESS, //true
    ERROR_INVALID_TYPE,
    ERROR_VALUE_TOO_HIGH,
    ERROR_VALUE_TOO_LOW
};

class ConfigValue
{
public:
    ConfigValue( QString value = "", int maxLength = 0, int minLength = 0 ) : s_value(value),
        i_type(CONFIGURATION_TYPE::TYPE_STRING), i_maxValue(maxLength), i_minValue(minLength < 0 ? 0 : minLength) {}

    ConfigValue( int value, int maxval = 0, int minval = 0 ) : s_value(QString().number(value)),
        i_type(CONFIGURATION_TYPE::TYPE_INT), i_maxValue(maxval), i_minValue(minval) {}

    ConfigValue( const char* value, int maxLength = 0, int minLength = 0 ) : s_value(QString(value)),
        i_type(CONFIGURATION_TYPE::TYPE_STRING), i_maxValue(maxLength), i_minValue(minLength < 0 ? 0 : minLength) {}


    CONFIGURATION_TYPE getConfigType() const { return i_type; }
    int getMaxValue() const { return i_maxValue; }
    int getMinValue() const { return i_minValue; }
    CONFIG_SETVALUE setValue( const int & );
    //bool setValue( float val );
    CONFIG_SETVALUE setValue( const QString & );

    void operator =(const QString &str) { s_value = str; }
    bool operator ==(const QString &str) const { return s_value == str; }
    bool operator ==(const uint &i) const { return s_value.toUInt() == i; }
    bool operator ==(const int &i) const { return s_value.toInt() == i; }
    template <typename T>
    bool operator !=(const T &i){ return !operator ==(i); }
    const QString &operator()() const{ return s_value; }
    ConfigValue operator+(const ConfigValue &val);
    bool operator<(const ConfigValue &val);
    bool operator>(const ConfigValue &val);

    int toInt() const { return s_value.toInt(); }
    QString getString() const { return s_value; }

private:
    QString s_value;
    CONFIGURATION_TYPE i_type;

    int    i_maxValue,
           i_minValue;
};

class BackendServer : public QObject
{
    Q_OBJECT
public:
    BackendServer( QObject * );
    ~BackendServer();
    bool LoadConfigFromFile( const QString & ); //Loads the confguration data from a specific file
    bool saveConfigFile( const QString & ); //used to save configuration data to a file (File is created if it does nto exist)
    void printToConsole( const QString &, VERBOSE_PRIORITY = VERBOSE_PRIORITY::PRIORITY_LOW ); //This sends text to the terminal thread.
    bool beginSQLConnection( const uint ); // This function attempts to establish the connection to the SQL server where data is stored. Settings are class member variables.
    bool beginTCPServer( const uint ); // This function allows TCP/IP connections (for data reception) to be made to the host device. Settings are class member variables.
    //The functions below handle parsed args from parseConsoleMessage
    void handleCloseServers( const QStringList & );
    void handleBeginServers( const QStringList & );
    void handleSetConfig( const QStringList & );
    void handleTCPClients( const QStringList & ); //handle the displaying of connected TCP clients, also handles forced connection closing.
    void handleSendTCPData( const QStringList & ); //handles the sending of data to a specific client.
    //
    bool sendToSocket( uint, QString & ); //arguments are the socket index for the p_Sockets vector, and the message.
    bool sendToSocket( ClientSocket *, QString & ); //arguments are a pointer to the socket, and the message

    QStringList parseConsoleArgs( const QString & ); //this function is responsible for breaking the input string from the console into usable args.
    void closeTCPServer();

    const QSqlDatabase* getSQLDatabase(){ return p_Database; }
    bool createDBTables( const QStringList & );


private: //member variables and whatnots - mostly config related
    QSqlDatabase *p_Database; // database pointer
    QTcpServer *p_Server; // server pointer
    QMap<QString, ConfigValue> *settingsMap; //store settings strings here.

    QVector<ClientSocket *> p_Sockets; //a vector of all of the client sockets

public slots: //data received from other objects
    void parseConsoleMessage( const QString & );
    void onClientConnected(); //Handles the connection of a new client socket
    void onClientDisconnected( ClientSocket * ); //Handles the disconnection of a client socket
    void onClientCommunication( const QString & ); //This handles the data from connected client sockets.


signals: //data sent to other objects in the project (most likely just the user interface)
    void printMessage( QString );
};

#endif // SERVER_H
