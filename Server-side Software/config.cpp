//This file includes all function definitions that are related to the setting of server configuration.

#include "server.h"

ConfigValue ConfigValue::operator+(const ConfigValue &val) //This is kinda flaky, and not very safe at this time. Might tweak it later.
{
    if ( this->i_type != val.i_type ) //different config type?
        return val; //just pass along the config that we were attempting to add.

    ConfigValue newCfg;

    switch( this->i_type )
    {
        case CONFIGURATION_TYPE::TYPE_INT:
            newCfg.s_value = QString().number( this->s_value.toInt() + val.s_value.toInt() );
            break;

        case CONFIGURATION_TYPE::TYPE_STRING:
            newCfg.s_value = this->s_value + val.s_value;
            break;
    }

    newCfg.i_type = this->i_type;
    newCfg.i_maxValue = this->i_maxValue;
    newCfg.i_minValue = this->i_minValue;

    return newCfg;
}

bool ConfigValue::operator<(const ConfigValue &val) //input is greater than this
{
    if ( this->i_type != val.i_type ) //different config type?
        return false; //can only compare the same type of config

    switch( this->i_type )
    {
        case CONFIGURATION_TYPE::TYPE_INT:
            return this->s_value.toInt() < val.s_value.toInt();

        case CONFIGURATION_TYPE::TYPE_STRING: //we'll just compare the length for strings, I suppose.
            return this->s_value.length() < val.s_value.length();
    }

    return false;
}

bool ConfigValue::operator>(const ConfigValue &val)
{
    if ( this->i_type != val.i_type ) //different config type?
        return false; //can only compare the same type of config

    switch( this->i_type )
    {
        case CONFIGURATION_TYPE::TYPE_INT:
            return this->s_value.toInt() > val.s_value.toInt();

        case CONFIGURATION_TYPE::TYPE_STRING: //we'll just compare the length for strings, I suppose.
            return this->s_value.length() > val.s_value.length();
    }

    return false;
}

CONFIG_SETVALUE ConfigValue::setValue(const QString &val) //Working to set the value of our config setting to a string value.
{
    if ( i_type == CONFIGURATION_TYPE::TYPE_STRING )
    {
        if ( val.length() > i_maxValue )
            return CONFIG_SETVALUE::ERROR_VALUE_TOO_HIGH;

        if ( val.length() < i_minValue )
            return CONFIG_SETVALUE::ERROR_VALUE_TOO_LOW;
    }
    else if ( i_type == CONFIGURATION_TYPE::TYPE_INT )
    {
        if ( val.length() > QString().number(i_maxValue).length() ) //no way can we possibly have more than 10 digits in an int.
            return CONFIG_SETVALUE::ERROR_FAILED;

        for ( int x = 0; x < val.length();x++ ) //make sure we're not mixing numbers and letters
        {
            if ( val[x].isLetter() )
                return CONFIG_SETVALUE::ERROR_INVALID_TYPE;
        }

        return setValue(val.toInt());
    }

    s_value = val; //set the new value
    return CONFIG_SETVALUE::SUCCESS;
}

CONFIG_SETVALUE ConfigValue::setValue(const int &val) //Setting the value to a new integer value
{
    if ( val > i_maxValue )
        return CONFIG_SETVALUE::ERROR_VALUE_TOO_HIGH;

    if ( val < i_minValue )
        return CONFIG_SETVALUE::ERROR_VALUE_TOO_LOW;

    s_value = QString().number(val);
    return CONFIG_SETVALUE::SUCCESS;
}
