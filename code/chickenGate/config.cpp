#include "config.h"


configuration *config;
Preferences prefs;

boolean IsInitialProgramLoad()
{
  const String version_date = __DATE__ __TIME__;
  uint16_t len = version_date.length();
  boolean is_ipl = false;

  String version_date_eeprom = prefs.getString("firstBoot");

  if (!version_date_eeprom.compareTo(version_date))
  {
    prefs.putString("firstBoot", version_date);
    is_ipl = true;
  }
  return is_ipl;
}

void InitEEPROM()
{
  prefs.begin("firstBoot", false);
  prefs.begin("config");

  if (IsInitialProgramLoad())
  {
    LoadEEPROMValues();
  }
  else
  {
    LoadDefaultValues();
    SaveToEEPROM();

  }
}

void LoadDefaultValues()
{
  config->temperatureLoopTime = 2000;
  config->BPLoopTime = 200;
  config->mainLoopTime = 2000;

  config->longitude = 43.605604;
  config->latitude = -1.062740;

  config->GMTshift = 1.0; // in hours
  config->closeShift = 30; // in minutes
  config->openShift = 0; // in minutes
}

void LoadEEPROMValues()
{
  size_t schLen = prefs.getBytesLength("config");
  char buffer[schLen]; // prepare a buffer for the data
  prefs.getBytes("config", buffer, schLen);
  config = (configuration *)buffer;
}

void SaveToEEPROM()
{
  prefs.putBytes("config", config, sizeof(configuration));
}
