#ifndef COMMON_H
#define COMMON_H

const char
        CHAR_CONFIG_SPLIT = '=', //When parsing config files, we'll split on the equals sign. It just makes sense
        CHAR_WHITESPACE = ' ',
        CHAR_CR = '\r',
        CHAR_NL = '\n',
        CHAR_COMMENT_1 = ';',//used to parse comments in config files, etc.
        CHAR_COMMENT_2 = '#', //used to parse comments in config files, etc.
        CHAR_FLAG_DEVID = 0x1, //SOH ascii char - nonprintable - used to indicate that we're receiving the connecting device ID for the socket.
        CHAR_FLAG_STORE_BEGIN = 0x2, //STX ascii char - nonprintable
        CHAR_FLAG_STORE_END = 0x4, //EOT ascii char - nonprintable
        CHAR_FLAG_TIME = 't',//0x5, //ENG ascii char - nonprintable - used to determine when the client is requesting a system time update

        //CLIENT COMMANDS BELOW HERE
        CMD_EXIT[] = "exit",
        CMD_QUIT[] = "quit",
        CMD_BEGIN[] = "begin",
        CMD_CLOSE[] = "close",
        CMD_SAVE[] = "save",
        CMD_RESTART[] = "restart",
        CMD_LOADCFG[] = "loadcfg", //for loading custom config files?
        CMD_CLIENTS[] = "clients",
        CMD_CONFIG[] = "config",
        CMD_SQL[] = "sql",
        CMD_TCP[] = "tcp",
        CMD_ALL[] = "all",
        CMD_SEND[] = "send", // for sending some data to a specific connected client.
        //
        //CONFIG FILE STRINGS BELOW HERE
        CONFIG_FILE[] = "config.cfg", //default config file name
        CONFIG_SQL_AUTOSTART[] = "sql_autostart",
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
#define MAX_MSG_LENGTH 64 //Maximum message length when sending data to a client (64 is default MAX for arduino)

enum VERBOSE_PRIORITY
{
    PRIORITY_HIGH = 0,
    PRIORITY_MEDIUM, //1
    PRIORITY_LOW //2
};

enum TRANSMISSION_CONDITION
{
    ERROR_GENERIC = 0,
    ERROR_NONE, //1 (true)
    ERROR_MSGLEN, //length of message is too long.
    ERROR_TIMEOUT //transmission timed out
};


#endif // COMMON_H
