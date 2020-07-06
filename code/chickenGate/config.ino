#include <Preferences.h>

Preferences prefs;




extern configuration *config;


void InitEEPROM()
{
  prefs.begin("config");
  size_t schLen = prefs.getBytesLength("config");
  char buffer[schLen]; // prepare a buffer for the data
  prefs.getBytes("config", buffer, schLen);
  config=(configuration *)buffer;
}
