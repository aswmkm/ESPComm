#include "terminal_interface.h"
#include "common.h"
#include <QTextStream>
#include <QString>

Terminal::Terminal() //Constructor
{
    cout = new QTextStream(stdout); //Output stream to console interface
    cin = new QTextStream(stdin); //Input stream from console interface

    start();
}
Terminal::~Terminal() //Destructor
{
    delete cout;
    delete cin;
}

template<typename T>
void Terminal::print( const T &str, bool newline )
{
    *cout << str;
    if ( newline )
        *cout << endl;
}

void Terminal::run() //Code thet gets called for each thread.
{
    QString s_input; //Storage for the input stream

   forever //This loop is what processes all user inputs. if it ends, it should call for all other services to end as well.
   {
        cin->readLineInto(&s_input);
        s_input = s_input.toLower(); //Convert to lowercase first


        if ( s_input == CMD_EXIT || s_input == CMD_QUIT )
            break; //end the loop
        else
            emit forwardToBackend(s_input); //Send directly to the backend for now
    }
}

void Terminal::displayMessage(QString msg) //display the mes
{
    *cout << msg << endl; //print the message to the console using the textstream
}
