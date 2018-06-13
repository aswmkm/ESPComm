#include <QCoreApplication>


#include "terminal_interface.h"
#include "server.h"
#include "common.h"
#include <iostream>
#include <QThread>

using namespace std;

//Basically the Terminal needs to be able to modify any configuration variables, as well as start and stop the SQL and TCP servers..

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    BackendServer serverBackend( &a ); //TCP/SQL server object
    Terminal serverTerminal( &a ); //User interface object

    //Connect signals/slots here
    a.connect(&serverTerminal,SIGNAL(finished()), &a, SLOT(quit())); //Needed to close the window once the terminal thread is closed.
    serverBackend.connect(&serverBackend, SIGNAL(printMessage(QString)), &serverTerminal, SLOT(displayMessage(const QString &)));
    serverTerminal.connect(&serverTerminal, SIGNAL(forwardToBackend(QString)), &serverBackend, SLOT(parseConsoleMessage(QString)));
    //

    serverBackend.LoadConfigFromFile(CONFIG_FILE); //must be called after we've established the signals/slots for message handling. just call it here
    return a.exec();
}


