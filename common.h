/*
 * common.h
 *
 * Created: 1/31/2018 2:13:42 PM
 *  Author: Andrew Ward
 * The point of this file is to house common defines for the project. 
 */ 


#ifndef COMMON_H_
#define COMMON_H_

using namespace std;

#define NULL_CHAR '/0'
#define MAX_BUFFERSIZE 128

#define VERBOSE_MAX 2

#define PRIORITY_LOW 2
#define PRIORITY_HIGH 1

struct TIMEZONES 
{
	
};

const char CMD_PREFIX = '/', // "\"
		   DATA_SPLIT = ':'; //Char used to split multiple strings of data in a serial commamnd stream

const char CMD_DISCONNECT = 'd',
		   CMD_UPDATE = 'u', //"field address #":"data"
		   CMD_LOGIN = 'l', //"username":"password"
		   CMD_CONNECT = 'c', //"SSID":"Password"
		   CMD_NETWORKS = 's', //scan
		   CMD_CREATEFIELD = 'f', //field <address#>:<type#>
		   CMD_NETINFO = 'i', //request current network info <if connected>
		   CMD_AP = 'a', //"ssid":"password"
		   CMD_VERBOSE = 'v', //<mode> can be 0 or any non-zero value, as well as 'on' or 'off'
		   CMD_PROGRAM = 'p', //stores specified values to eeprom so that they will load automatically in the future
		   CMD_TIME = 't'; //Used to set system time. No args returns time, args set time.
		   


#endif /* COMMON_H_ */