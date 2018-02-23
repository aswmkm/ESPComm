/*
 * time.h
 *
 * Created: 2/22/2018 3:34:20 PM
 *  Author: Andrew Ward
 * This header contains all of the definitions for the Time class
 */ 

#include "common.h"
#include "c_types.h"
#include <WString.h>

#ifndef TIME_H_
#define TIME_H_

#define MAX_YEAR 99 //Only working within bounds of last two digits (NOT Year 2100 compliant, as if that matters)
#define NUM_MONTHS_YEAR 12 //Number of months to a year
#define NUM_HOURS_DAY 24 //Number of hours toa day

#define TIME_SECOND 1
#define TIME_MINUTE 2
#define TIME_HOUR 3
#define TIME_DAY 4
#define TIME_MONTH 5
#define TIME_YEAR 6

class Time
{
	public: 
	Time()
	{	
		i_year = i_second = i_minute = i_timeZone = 0; //Init to 0
		i_hour = i_day = i_month = 1; //Default to 1, as we cannot have a day 0, or month 0, etc.
	}
	Time( const Time &T2 )
	{
		SetTime( T2 );
	}
	
	bool SetTime( const uint8_t &yr, const uint8_t &mo, const uint8_t &day, const uint8_t &hr, const uint8_t &min, const uint8_t &sec );
	bool SetTime( const Time &T2 ); //For copying values over
	bool SetTime( const Time *T2 ); //For copying values over
	bool IsAhead( const Time * );
	bool IsBehind( const Time *T2 ){ return !IsAhead(T2); }
	uint8_t GetMonthDays( uint8_t ); //Used to determine the proper number of days in a specific month.
	bool SetTimeZone( const uint8_t &zone ){ i_timeZone = zone; }
	uint8_t GetTimeZone(){ return i_timeZone; }
	bool IncrementTime( unsigned int, uint8_t ); //Input increment amount, along with time unit.
	String GetTimeStr(); //Returns a formatted string, representing the current saved time.
	
	//Operator stuff.
	bool operator< ( const Time &T2 ); //see time.cpp
	bool operator> ( const Time &T2 ){ return !(*this < T2 ); }
	bool operator<= ( const Time &T2 ){ return *this < T2; }
	bool operator>= ( const Time &T2 ){ return !(*this < T2 ); }
	bool operator== ( const Time &T2 ); //see time.cpp
	bool operator!= ( const Time &T2 ){ return !(*this == T2);}
	Time& operator= ( const Time &T2 );
	//
	
	private:
	uint8_t AdvanceMonthYear( unsigned int ); //Used to advance the month, and possibly the year, using a value of days as an input. Returns the remainder of days.
	
	uint8_t i_second,
		i_minute,
		i_hour,
		i_day,
		i_month,
		i_year,
		i_timeZone;
};



#endif /* TIME_H_ */