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

#define TYPE_REMOVE -1
#define TYPE_NONE 0
#define TYPE_INPUT_RADIO 1
#define TYPE_INPUT_TEXT 2
#define TYPE_INPUT_SUBMIT 3
#define TYPE_INPUT_CHECKBOX 4

#define TYPE_OUTPUT_TEXT 10

#define METHOD_NONE 0
#define METHOD_POST 1
#define METHOD_GET 2

class DataField
{
	public:
	DataField( int address, uint8_t type, String fieldLabel = "", String fieldName = "", String defaultValue = "" )
	{
		i_Address = address;
		i_Type = type;
		SetFieldName( fieldName );
		SetFieldLabel( fieldLabel );
		SetFieldValue( defaultValue );
	}
	int GetAddress() { return i_Address; } //returns the address of the field (Used to make sure we're updating the proper field)
	uint8_t GetType() { return i_Type; } //Returns the field type (used for the generation of the HTML code)
	void SetFieldName( String ); //This sets the name for the field.
	void SetFieldValue( String ); //This is used for setting the data within the field.
	void SetFieldLabel( String ); //Used to set text label for field (if applicable)
	String GetFieldName() { return s_fieldName; }
	String GetFieldValue() { return s_fieldValue; }
	String GetFieldLabel() { return s_fieldLabel; }
	String GenerateHTML(); //Used to create the HTML to be appended to the body of the web page.
	
	private:
	uint8_t i_Type; //This represents the type of data field we're displaying (Text-box, radio button, etc)
	uint8_t i_Method,
			i_Function;
	int i_Address; //Represents the address number used for updating values in the field.	
	String s_fieldName; //Used for POST/GET methods.
	String s_fieldValue; //This is the data being displayed within the form (default text in a text-box for example)
	String s_fieldLabel;
	
};


#endif /* DATA_FIELDS_H_ */