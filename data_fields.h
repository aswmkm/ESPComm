/*
 * data_fields.h
 *
 * Created: 2/2/2018 11:26:45 AM
 *  Author: Andrew Ward
 * This header file contains the definitions for all functions related to the 
 */ 

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
#define TYPE_INPUT_PASSWORD 5

#define TYPE_OUTPUT_TEXT 10

const char HTML_HEADER[] PROGMEM =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
//"<meta http-equiv=\"refresh\" content=\"3\" />"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
"</style>"
"</head>"
"<body>";

const char HTML_FOOTER[] PROGMEM =
"</body>"
"</html>";

class DataField
{
	public:
	DataField( unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", const String &defaultValue = "", bool newLine = true )
	{
		i_Address = address;
		i_Type = type;
		SetFieldName( fieldName );
		SetFieldLabel( fieldLabel );
		SetFieldValue( defaultValue );
		b_enabled = true;
		b_visible = true;
		b_newLine = newLine;
	}
	bool IsEnabled(){ return b_enabled; }
	bool IsVisible(){ return b_visible; }
	unsigned int GetAddress() { return i_Address; } //returns the address of the field (Used to make sure we're updating the proper field)
	uint8_t GetType() { return i_Type; } //Returns the field type (used for the generation of the HTML code)
	void SetFieldName( const String & ); //This sets the name for the field.
	virtual bool SetFieldValue( const String & ); //This is used for setting the data within the field.
	void SetFieldLabel( const String & ); //Used to set text label for field (if applicable)
	void SetVisible( bool vis ){ b_visible = vis; }
	void SetEnabled( bool en ){ b_enabled = en; }
	const String &GetFieldName() { return s_fieldName; }
	virtual const String GetFieldValue() { return s_fieldValue; }
	const String &GetFieldLabel() { return s_fieldLabel; }
	String GenerateHTML(); //Used to create the HTML to be appended to the body of the web page.
	
	private:
	uint8_t i_Type; //This represents the type of data field we're displaying (Text-box, radio button, etc)
	uint8_t i_Method,
			i_Function;
	unsigned int i_Address; //Represents the address number used for updating values in the field.	
	String s_fieldName; //Used for POST/GET methods.
	String s_fieldValue; //This is the data being displayed within the form (default text in a text-box for example)
	String s_fieldLabel; //Text that describes the field
	
	bool b_enabled; //Is this field enabled? 
	bool b_visible; //Is it visible in user config page? (REDUNDANT WITH ENABLE?)
	bool b_newLine; //Generate a newline in HTML following this Datafield.
};

class DataTable //This class is basically used to create sections for specific types of inputs, like SQL settings, or Time settings, etc.
{
	public:
	DataTable( const String &name = "", const String &ID = "" )
	{
		s_tableName = name;
		s_tableID = ID;
	}
	
	void SetTableName( const String &name ) { s_tableName = name; }
	String GenerateTableHTML(); //Generates the HTML for the Table.
	bool RemoveElement( unsigned int );
	bool AddElement( DataField * );
	int8_t IteratorFromAddress( unsigned int ); //Used to search for an object with the inputted address. If that object is found, the position in the vector is returned, else -1
	DataField *GetElementByID( unsigned int ); //Retrieves the element with the corresponding ID
	DataField *GetElementByName( const String & ); //Get the element by its assigned name.
	const vector<DataField *> &GetFields(){ return p_fields; }
		
	private: 
	String s_tableName;
	String s_tableID;
	vector<DataField *> p_fields; //vector containing the pointers to all of our data fields (regular fields only).
};

//SPECIAL DATAFIELDS - For directly modifying settings.
class UINT_Datafield : public DataField
{
	public:
	UINT_Datafield( unsigned int *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		fieldVar = var;
	}
	
	unsigned int *GetVar(){ return fieldVar; }
	bool SetFieldValue( const unsigned int & );
	bool SetFieldValue( const String & );
	const String GetFieldValue();
	
	private:
	unsigned int *fieldVar;
};

class UINT8_Datafield : public DataField
{
	public:
	UINT8_Datafield( uint8_t *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		fieldVar = var;
	}
	
	uint8_t *GetVar(){ return fieldVar; }
	bool SetFieldValue( const uint8_t & );
	bool SetFieldValue( const String & );
	const String GetFieldValue();
	
	private:
	uint8_t *fieldVar;
};

class BOOL_Datafield : public DataField
{
	public:
	BOOL_Datafield( bool *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		fieldVar = var;
	}
	
	bool *GetVar(){ return fieldVar; }
	bool SetFieldValue( const bool & );
	bool SetFieldValue( const String & );
	const String GetFieldValue();
		
	private:
	bool *fieldVar;
};

class STRING_Datafield : public DataField
{
	public:
	STRING_Datafield( String *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true ) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		fieldVar = var;
	}
	
	String *GetVar(){ return fieldVar; }
	bool SetFieldValue( const String & );
	const String GetFieldValue();
		
	private:
	String *fieldVar;
};


//Reactive Settings (They execute a function upon value being changed)
class STRING_S_Datafield : public DataField
{
	public:
	STRING_S_Datafield( ReactiveSetting_STRING *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true ) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		setting = var;
	}
	
	ReactiveSetting_STRING *GetSetting(){ return setting; }
	bool SetFieldValue( const String & );
	const String GetFieldValue(){ return setting->ToString(); }
	
	private:
	ReactiveSetting_STRING *setting;
};

class UINT_S_Datafield : public DataField
{
	public:
	UINT_S_Datafield( ReactiveSetting_UINT *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true ) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		setting = var;
	}
	
	ReactiveSetting_UINT *GetSetting(){ return setting; }
	bool SetFieldValue( const String & );
	const String GetFieldValue(){ return setting->ToString(); }
	
	private:
	ReactiveSetting_UINT *setting;
};

class UINT8_S_Datafield : public DataField
{
	public:
	UINT8_S_Datafield( ReactiveSetting_UINT8 *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true ) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		setting = var;
	}
	
	ReactiveSetting_UINT8 *GetSetting(){ return setting; }
	bool SetFieldValue( const String & );
	const String GetFieldValue(){ return setting->ToString(); }
	
	private:
	ReactiveSetting_UINT8 *setting;
};

class BOOL_S_Datafield : public DataField
{
	public:
	BOOL_S_Datafield( ReactiveSetting_BOOL *var, unsigned int address, uint8_t type, const String &fieldLabel = "", const String &fieldName = "", bool newLine = true ) :
	DataField( address, type, fieldLabel, fieldName, "", newLine )
	{
		setting = var;
	}
	
	ReactiveSetting_BOOL *GetSetting(){ return setting; }
	bool SetFieldValue( const String & );
	const String GetFieldValue(){ return setting->ToString(); }
	
	private:
	ReactiveSetting_BOOL *setting;
};


#endif /* DATA_FIELDS_H_ */