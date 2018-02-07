/*
 * data_fields.h
 *
 * Created: 2/2/2018 11:26:45 AM
 *  Author: Andrew Ward
 * This header file contains the definitions for all functions related to the 
 */ 

#include "common.h"
#include <vector>
#include <WString.h>

#ifndef DATA_FIELDS_H_
#define DATA_FIELDS_H_

#define MAX_DATA_LENGTH 16
#define MAX_NAME_LENGTH 16 

class DataField
{
	public:
	int GetIndex() { return i_Address; } //returns the address of the field (Used to make sure we're updating the proper field)
	uint8_t GetType() { return i_Type; } //Returns the field type (used for the generation of the HTML code)
	void SetFieldName( String ); //This sets the name for the field.
	void SetFieldData( String ); //This is used for setting the data within the field.
	String GetFieldName() { return s_FieldName; }
	String GetFieldData() { return s_FieldData; }
	
	private:
	uint8_t i_Type; //This represents the type of data field we're displaying (Textbox, radio button, etc)
	int i_Address; //Represents the address number used for updating values in the field.	
	String s_FieldName;
	String s_FieldData;
	
};


#endif /* DATA_FIELDS_H_ */