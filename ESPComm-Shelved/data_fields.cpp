/*
 * data_fields.cpp
 *
 * Created: 2/2/2018 11:26:31 AM
 *  Author: Andrew
 */ 


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

String DataField::GenerateHTML() //Baseclass GenerateHTML
{
	String HTML;

	if ( strlen( GetFieldLabel().c_str() ) && GetType() != FIELD_TYPE::SUBMIT ) //Is there a label?
		HTML += GetFieldLabel() + ": ";
	
	HTML += F("<INPUT ");

	switch ( GetType() )
	{
		case FIELD_TYPE::TEXT:
			HTML += F("type=\"text\" ");
			break;
		case FIELD_TYPE::SUBMIT:
			HTML += F("type=\"submit\" ");
			break;
		case FIELD_TYPE::RADIO:
			HTML += F("type=\"radio\" ");
			break;
		case FIELD_TYPE::CHECKBOX:
			HTML += F("type=\"checkbox\" ");
			break;
		case FIELD_TYPE::PASSWORD:
			HTML += F("type=\"password\" ");
			break;
	}
	
	if ( strlen( GetFieldName().c_str() ) && GetType() != FIELD_TYPE::SUBMIT ) //If we even have a name.
		HTML += "name=\"" + GetFieldName() + "\" ";
	
	switch ( GetType() )
	{
		case FIELD_TYPE::CHECKBOX:
			if ( strlen( GetFieldValue().c_str() ) && GetFieldValue() != String(false) ) //Only set this if we have some value there, otherwise it'll just count as "enabled" - weird.
				HTML += "checked ";
				//HTML += "checked=\"" + GetFieldValue() + "\" ";
			break;
		default:
			HTML += "value=\"" + GetFieldValue() + "\" ";
			break;
	}			
	
	HTML += ">"; //End of the <INPUT 
	
	if ( b_newLine )	//Generate newline?
		HTML += F("<br>");

	return HTML;
}

String SSID_Datafield::GenerateHTML() //This is a specific type of field, tailored to a specific function.
{
	String HTML;

	if ( strlen( GetFieldLabel().c_str() ) ) //Is there a label?
		HTML += GetFieldLabel() + ": ";
	
	HTML += F("<select ");
	HTML += "id=\"" + String(GetAddress()) + "\" name=\"" + GetFieldName() + "\" >";
	
	if ( WiFi.scanComplete() <= 0 )
	{
		if ( !WiFi.isConnected() )
			HTML += F("<option >N/A</option>");
		else 
			HTML += "<option >" + WiFi.SSID() + " (Connected)</option>";
	}
	else
	{
		for ( uint8_t x = 0; x < WiFi.scanComplete(); x++ )
		{
			HTML += "<option value=\"" + String(x) + "\"";
			if ( WiFi.SSID( x ) == WiFi.SSID() )
			{
				HTML += F(" selected ");
				HTML += ">" + WiFi.SSID(x) + " (Connected)";
			}
			else
				HTML += ">" + WiFi.SSID(x); 
				
			HTML += F("</option>");
		}
	}
	
	HTML += F("</select>"); //End of the <INPUT
	
	if ( DoNewline() )	//Generate newline?
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
// - SPECIAL FIELD STUFF BELOW - The idea here is that we're modifying the variables that are tied to the objects, 
// - rather than the local variables that are stored in the DataField class. This will allow us to view changes to these variables 
// - made by other methods (serial parser for example) in the HTML, without the need for matching the var and the FieldValue string. 
//////////////////////////////////////////////////////////////////////////

bool STRING_Datafield::SetFieldValue( const String &value )
{
	if ( *GetVar() != value  ) //Check the main variable being modified first.
	{
		*GetVar() = value;
		return true;
	}
	return false;
}

bool UINT_Datafield::SetFieldValue( const String &value )
{
	unsigned int newValue = parseInt( value );
	if ( *GetVar() != newValue  )
	{
		*GetVar() = newValue;
		return true;
	}
	return false;
}

bool UINT_Datafield::SetFieldValue( const unsigned int &value )
{
	if ( *GetVar() != value  )
	{
		*GetVar() = value;
		return true;
	}
	return false;
}

bool UINT8_Datafield::SetFieldValue( const String &value )
{
	uint8_t newValue = parseInt( value );
	if ( *GetVar() != newValue  )
	{
		*GetVar() = newValue;
		return true;
	}
	return false;
}

bool UINT8_Datafield::SetFieldValue( const uint8_t &value )
{
	if ( *GetVar() != value  )
	{
		*GetVar() = value;
		return true;
	}
	return false;
}

bool BOOL_Datafield::SetFieldValue( const String &value )
{
	bool newValue = ( parseInt( value ) || value.length() );
	if ( *GetVar() != newValue  )
	{
		*GetVar() = newValue;
		return true;
	}
	return false;
}

bool BOOL_Datafield::SetFieldValue( const bool &value )
{
	if ( *GetVar() != value  )
	{
		*GetVar() = value;
		return true;
	}
	return false;
}

const String STRING_Datafield::GetFieldValue()
{
	return *GetVar();
}

const String UINT_Datafield::GetFieldValue()
{
	return String( *GetVar() );
}

const String UINT8_Datafield::GetFieldValue()
{
	return String( *GetVar() );
}

const String BOOL_Datafield::GetFieldValue()
{
	return String( *GetVar() );
}