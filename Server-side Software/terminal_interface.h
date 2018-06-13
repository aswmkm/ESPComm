//
//This file contains the class definition for the user terminal.
//

#ifndef TERMINAL_INTERFACE_H
#define TERMINAL_INTERFACE_H

#include <QThread>
#include <QString>
#include <QObject>
#include <String>
#include <QTextStream>
#include <QtSql/QSqlDatabase>
#include <QtNetwork/QTcpServer>
#include "server.h"

class Terminal : public QThread
{
    Q_OBJECT
public:
    Terminal( QObject * ); //constructor
    ~Terminal(); //Desctructor
    template<typename T>
    void print( const T&, bool = false );
    QTextStream &GetCout(){ return *cout; } //return the output stream.
    virtual void run();

private:
    QTextStream *cout, *cin; //console stuff


public slots: //data received from other objects
    void displayMessage( const QString & ); //Accepts messages from signals

signals: //to be sent to other objects
    void forwardToBackend( const QString & ); //Used to forward user statements to the server backend for parsing.
};

#endif // TERMINAL_INTERFACE_H
