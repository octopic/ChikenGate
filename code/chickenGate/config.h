#ifndef CONFIG_H
#define config_H
#include <Preferences.h>


typedef struct { //used as storage for the eeprom
  int temperatureLoopTime = 2000;
  int BPLoopTime = 200;
  int mainLoopTime = 2000;

  float longitude =43.605604;
  float latitude = -1.062740;

  float GMTshift = 0.0; // in hours
  int closeShift = 30; // in minutes
  int openShift = 0; // in minutes

} configuration;

void InitEEPROM();
boolean IsInitialProgramLoad();
void LoadEEPROMValues();
void SaveToEEPROM();
void LoadDefaultValues();

#endif
