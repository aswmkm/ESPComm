/*
 * data_fields.cpp
 *
 * Created: 2/2/2018 11:26:31 AM
 *  Author: Andrew
 */ 

#include "data_fields.h"
#include "common.h"


void DataField::SetFieldName( String input )
{
	if ( input.length() > MAX_NAME_LENGTH )
		return; //Die here, name too long.
		
	s_FieldName = input;	
}

void DataField::SetFieldData( String input )
{
	if ( input.length() > MAX_DATA_LENGTH )
		return; //Die here, name too long.
	
	s_FieldData = input;
}
