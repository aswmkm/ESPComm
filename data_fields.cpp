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
		
	s_fieldName = input;	
}

void DataField::SetFieldValue( String input )
{
	if ( input.length() > MAX_DATA_LENGTH )
		return; //Die here, name too long.
	
	s_fieldValue = input;
}

void DataField::SetFieldLabel( String input )
{
	s_fieldLabel = input;
}

String DataField::GenerateHTML()
{
	String HTML;
	//HTML += "<INPUT type=\"radio\" name=\"LED\" value=\"0\">Off<br>";
	if ( strlen( GetFieldLabel().c_str() ) ) //Is there a label?
		HTML += GetFieldLabel() + ": ";
	
	if ( GetType() ) //Must not be TYPE_NONE
	{
		HTML += "<INPUT ";

		switch ( GetType() )
		{
			case TYPE_INPUT_TEXT:
				HTML += "type=\"text\" ";
				break;
			case TYPE_INPUT_SUBMIT:
				HTML += "type=\"submit\" ";
				break;
			case TYPE_INPUT_RADIO:
				HTML += "type=\"radio\" ";
				break;
		}
	
		if ( strlen( GetFieldName().c_str() ) ) //If we even have a name.
			HTML += "name=\"" + GetFieldName() + "\" ";
	
		if ( strlen( GetFieldValue().c_str() ) )
			HTML += "value=\"" + GetFieldValue() + "\" ";
		
		HTML += "><br>";
	}
	else //TYPE_NONE (Text) <needs work>
	{
		if ( strlen( GetFieldValue().c_str() ) )
			HTML += GetFieldValue(); //Just return the value and move on.
		
		HTML += "<br>"; //newline
	}
	return HTML;
}
