/*
 * settings.h
 *
 * Created: 2/27/2018 2:22:42 PM
 *  Author: Andrew Ward
 * The purpose of this file is to house the CommSettings Class stuff.
 * All settings that can be configured via the parser, web interface, EEPROM, etc will use one of these object classes.
 * The idea with these objects is that when a setting is changed, a specific function can be called automatically.
 */ 

#ifndef SETTINGS_H_
#define SETTINGS_H_

class ReactiveSetting
{
	public:
	ReactiveSetting( function<void(void)> &onChanged )
	{
		func = onChanged;
	}
	
	void SetOnChanged( function<void(void)> &onChanged ){ func = onChanged; }
	virtual const String ToString();
	function<void(void)> GetOnChanged(){ return func; }
		
	private:
	function<void(void)> func;
};

class ReactiveSetting_BOOL : public ReactiveSetting
{
	public:
	ReactiveSetting_BOOL( bool value, function<void(void)> &fn ) : ReactiveSetting( fn )
	{
		var = value; //initial value
	}
	bool SetValue( const bool &value )
	{
		if ( var != value )
		{
			var = value;
			ReactiveSetting::GetOnChanged()();
			return true;
		}
		return false;
	}
	const String ToString(){ return String(var); }
	private:
	bool var;
};

class ReactiveSetting_UINT8: public ReactiveSetting
{
	public:
	ReactiveSetting_UINT8( uint8_t value, function<void(void)> &fn ) : ReactiveSetting( fn )
	{
		var = value;
	}
	bool SetValue( const uint8_t &value )
	{
		if ( var != value )
		{
			var = value;
			ReactiveSetting::GetOnChanged()();
			return true;
		}
		return false;
	}
	const String ToString(){ return String(var); }
		
	private:
	uint8_t var;
};

class ReactiveSetting_UINT: public ReactiveSetting
{
	public:
	ReactiveSetting_UINT( unsigned int value, function<void(void)> &fn ) : ReactiveSetting( fn )
	{
		var = value;
	}
	bool SetValue( const unsigned int &value )
	{
		if ( var != value )
		{
			var = value;
			ReactiveSetting::GetOnChanged()();
			return true;
		}
		return false;
	}
	const String ToString(){ return String(var); }
		
	private:
	unsigned int var;
};

class ReactiveSetting_STRING: public ReactiveSetting
{
	public:
	ReactiveSetting_STRING( String &value, function<void(void)> &fn ) : ReactiveSetting( fn )
	{
		var = value;
	}
	bool SetValue( const String &value )
	{
		if ( var != value )
		{
			var = value;
			ReactiveSetting::GetOnChanged()();
			return true;
		}
		return false;
	}
	const String ToString(){ return var; }
	
	private:
	String var;
};




#endif /* SETTINGS_H_ */