/*
 * data_fields.cpp
 *
 * Created: 2/2/2018 11:26:31 AM
 *  Author: Andrew
 */ 

#include "data_fields.h"
#include "common.h"
#include "ESPComm.h"


void DataField::SetFieldName( const String &input )
{
	if ( input.length() > MAX_NAME_LENGTH )
		return; //Die here, name too long.
		
	s_fieldName = input;	
}

bool DataField::SetFieldValue( const String &input )
{
	if ( input.length() > MAX_DATA_LENGTH )
		return false; //Die here, name too long.
	
	s_fieldValue = input;
	return true;
}

void DataField::SetFieldLabel( const String &input )
{
	s_fieldLabel = input;
}

String DataField::GenerateHTML()
{
	String HTML;

	if ( strlen( GetFieldLabel().c_str() ) && GetType() != TYPE_INPUT_SUBMIT ) //Is there a label?
		HTML += GetFieldLabel() + ": ";
	
	HTML += F("<INPUT ");

	switch ( GetType() )
	{
		case TYPE_INPUT_TEXT:
			HTML += F("type=\"text\" ");
			break;
		case TYPE_INPUT_SUBMIT:
			HTML += F("type=\"submit\" ");
			break;
		case TYPE_INPUT_RADIO:
			HTML += F("type=\"radio\" ");
			break;
		case TYPE_INPUT_CHECKBOX:
			HTML += F("type=\"checkbox\" ");
			break;
		case TYPE_INPUT_PASSWORD:
			HTML += F("type=\"password\" ");
			break;
	}
	
	if ( strlen( GetFieldName().c_str() ) && GetType() != TYPE_INPUT_SUBMIT ) //If we even have a name.
		HTML += "name=\"" + GetFieldName() + "\" ";
	
	switch ( GetType() )
	{
		case TYPE_INPUT_CHECKBOX:
			if ( strlen( GetFieldValue().c_str() ) ) //Only set this if we have some value there, otherwise it'll just count as "enabled" - weird.
				HTML += "checked=\"" + GetFieldValue() + "\" ";
			break;
		default:
			HTML += "value=\"" + GetFieldValue() + "\" ";
			break;
	}			
	
	HTML += ">"; //End of the <INPUT tag
	
	if ( b_newLine )	//Generate newline?
		HTML += F("<br>");

	return HTML;
}


//  -------------------------------------------------------------------------------
//	-- DATA TABLE STUFF BELOW
//  -------------------------------------------------------------------------------

bool DataTable::RemoveElement( unsigned int index )
{
	int8_t x = IteratorFromAddress( index );
	
	if ( x > -1 )
	{
		delete p_fields[x];//delete
		p_fields.erase( p_fields.begin() + x );
		return true; //Die here.
	}
	return false;
}

DataField * DataTable::GetElementByID( unsigned int index )
{
	int8_t x = IteratorFromAddress( index );
	
	if ( x > -1 )
		return p_fields[x];

	return 0; //Default return path
}

DataField * DataTable::GetElementByName( const String &name )
{
	for ( uint8_t x = 0; x < p_fields.size(); x++ )
	{
		if ( p_fields[x]->GetFieldName() == name )
			return p_fields[x]; 
	}
	
	return 0; //default return path
}

bool DataTable::AddElement( DataField * field )
{
	if ( IteratorFromAddress( field->GetAddress() ) > -1 ) //Valid iterator found, index already taken.
	{
		delete field; //Make sure we prevent memory leaks.
		return false;
	}
	
	p_fields.push_back( field );
	return true;
}

int8_t DataTable::IteratorFromAddress( unsigned int index )
{
	for ( uint8_t x = 0; x < p_fields.size(); x++ )
	{
		if ( p_fields[x]->GetAddress() == index ) //Address exists?
			return x;
	}
	
	return -1;
}

String DataTable::GenerateTableHTML()
{
	String HTML;
	HTML += "<h2>" + s_tableName + "</h2>";
	HTML += "<table id =\"" + s_tableID + "\"  style=\"width:100%\" > ";
	HTML += "<P>";
	for ( uint8_t x = 0; x < p_fields.size(); x++ )
	{
		if ( !p_fields[x]->IsEnabled() || !p_fields[x]->IsVisible() )
			continue;
			
		HTML += p_fields[x]->GenerateHTML();
	}
	HTML += F("</P>");
	HTML += F("</table>");
	return HTML;
}

//////////////////////////////////////////////////////////////////////////
// - SPECIAL FIELD STUFF BELOW
//////////////////////////////////////////////////////////////////////////

bool STRING_Datafield::SetFieldValue( const String &value )
{
	if ( *GetVar() != value && DataField::SetFieldValue( value ) ) //Check the main variable being modified first.
	{
		*GetVar() = value;
		return true;
	}
	return false;
}

bool UINT_Datafield::SetFieldValue( const String &value )
{
	unsigned int newValue = parseInt( value );
	if ( *GetVar() != newValue && DataField::SetFieldValue( value ) )
	{
		*GetVar() = newValue;
		return true;
	}
	return false;
}

bool UINT8_Datafield::SetFieldValue( const String &value )
{
	uint8_t newValue = parseInt( value );
	if ( *GetVar() != newValue && DataField::SetFieldValue( value ) )
	{
		*GetVar() = newValue;
		return true;
	}
	return false;
}

bool BOOL_Datafield::SetFieldValue( const String &value )
{
	bool newValue = ( value.length() ? true : false );
	if ( *GetVar() != newValue && DataField::SetFieldValue( value ) )
	{
		*GetVar() = newValue;
		return true;
	}
	return false;
}