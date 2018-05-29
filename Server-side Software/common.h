#ifndef COMMON_H
#define COMMON_H

const char CHAR_CONFIG_SPLIT = '=', //When parsing config files, we'll split on the equals sign. It just makes sense
           CHAR_WHITESPACE = ' ';

const char
              //CLIENT COMMANDS BELOW HERE
              CMD_EXIT[] = "exit",
              CMD_QUIT[] = "quit",
              CMD_BEGIN[] = "begin",
              CMD_CLOSE[] = "close",
              CMD_RESTART[] = "restart",
              CMD_LOADCFG[] = "loadcfg", //for loading custom config files?
              CMD_CLIENTS[] = "clients",
              CMD_CONFIG[] = "config",
              //
              //CONFIG FILE STRINGS BELOW HERE
              CONFIG_FILE[] = "config.cfg", //default config file name
              CONFIG_SQL_DBTYPE[] = "sql_dbtype",
              CONFIG_SQL_HOSTNAME[] = "sql_hostname",
              CONFIG_SQL_DBNAME[] = "sql_dbname",
              CONFIG_SQL_DBUSER[] = "sql_dbuser",
              CONFIG_SQL_DBPASS[] = "sql_dbpassword",
              CONFIG_SQL_PORT[] = "sql_port",
              CONFIG_TCP_DUPLICATECONNECTIONS[] = "tcp_duplicates",
              CONFIG_TCP_MAXCONNECTIONS[] = "tcp_maxconnections",
              CONFIG_TCP_PORT[] = "tcp_port",
              CONFIG_TCP_AUTOSTART[] = "tcp_autostart",
              CONFIG_VERBOSITY[] = "verbose";
              //

#define DEFAULT_SQL_PORT 1443 //Known to be the common default port for SQL
#define DEFAULT_SQL_HOSTNAME "localhost"
#define DEFAULT_TCP_PORT 1000 //Seems as good as any other?
#define DEFAULT_TCP_MAXCONNECTIONS 10

enum VERBOSE_PRIORITY
{
    PRIORITY_HIGH = 0,
    PRIORITY_MEDIUM, //1
    PRIORITY_LOW //2
};


#endif // COMMON_H
