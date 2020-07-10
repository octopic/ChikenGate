#include <Wire.h>
#include "config.h"
#include "wifiServer.h"

extern configuration config;
/////////////////////////////////////////
///  CAPTEUR TEMPERATURE ET HIMIDITE  ///
/////////////////////////////////////////

#include <BMx280I2C.h>
#define I2C_ADDRESS_INTERIEUR 0x76
#define I2C_ADDRESS_EXTERIEUR 0x77
BMx280I2C capteurTempHum_interieur(I2C_ADDRESS_INTERIEUR);
BMx280I2C capteurTempHum_exterieur(I2C_ADDRESS_EXTERIEUR);

unsigned long timeBoucleTemperature = 0;

//valeur*100
#define TAILLE_TAB_TEMP_HUM 10000
short temperaturesInt[TAILLE_TAB_TEMP_HUM] = {0};
short humiditeInt[TAILLE_TAB_TEMP_HUM] = {0};
short temperaturesExt[TAILLE_TAB_TEMP_HUM] = {0};
short humiditeExt[TAILLE_TAB_TEMP_HUM] = {0};
int posTabTempHum = 0;
bool nouvelleMesure = false;

////////////////////////////
///  BOUTONS ET MOTEURS  ///
////////////////////////////
unsigned long timeBoucleBP = 0;
#define TEMPS_BOUCLE_BP 200

#define PIN_MOTEUR_PORTE_MONTE 19
#define PIN_MOTEUR_PORTE_DESCEND 18
#define PIN_MOTEUR_AERATION_OUVRE 5
#define PIN_MOTEUR_AERATION_FERME 17

#define PIN_ENDSTOP_PORTE_HAUT 12
#define PIN_ENDSTOP_PORTE_BAS 14
#define PIN_ENDSTOP_AERATION_OUVERT 27
#define PIN_ENDSTOP_AERATION_FERME 26

#define PIN_BP_PORTE_MONTE 16
#define PIN_BP_PORTE_DESCEND 4
#define PIN_BP_AERATION_OUVRE 2
#define PIN_BP_AERATION_FERME 15

byte etatMoteur = 0; //-1 descend, 0 arret, 1 monte
byte etatAeration = 0; //-1 ferme, 0 arret, 1 ouvre

bool etatBPPorteMonte = false;
bool etatBPPorteDescend = false;
bool etatBPAerationOuvre = false;
bool etatBPAerationFerme = false;

bool oldEtatBPPorteMonte = false;
bool oldEtatBPPorteDescend = false;
bool oldEtatBPAerationOuvre = false;
bool oldEtatBPAerationFerme = false;

byte etatEndstopPorte = 0;
byte etatEndstopAeration = 0;

/////////////////////////////////////
///  RTC & Capteur de luminosite  ///
/////////////////////////////////////
#include <RTC.h>
static DS3231 RTC;
unsigned long timeBoucleHorloge = 0;
int year, month, day, hour, minute, seconds, oldDay = 0;

#define PIN_CAPTEUR_LUM 33
bool etatCapteurLum = false;

/////////////////////////////////////
///  ephemeride et automtisation  ///
/////////////////////////////////////
#include "Dusk2Dawn.h"
Dusk2Dawn ephemeride(config.longitude, config.latitude , config.GMTshift);

int minutesRTC = 0; //temps en minutes depuis minuit
int minutesOuverture = 0;
int minutesFermeture = 0;
enum enumEtatPorte {EnBas, EnMontee, EnHaut, EnDescente, EtatPorteInconnue};
enumEtatPorte etatPorte;


void setup()
{
  Serial.begin(115200);
  Serial.println("Debut");

  InitEEPROM();
  InitWIFI();

  /////////////////////////////////////////
  ///  CAPTEUR TEMPERATURE ET HIMIDITE  ///
  /////////////////////////////////////////
  Serial.println("Init capteur");
  Wire.begin();

  if (!capteurTempHum_interieur.begin())
    Serial.println("Erreur connection temperature interieur");

  if (!capteurTempHum_exterieur.begin())
    Serial.println("Erreur connection temperature exterieur");

  capteurTempHum_interieur.resetToDefaults();
  capteurTempHum_exterieur.resetToDefaults();

  capteurTempHum_interieur.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  capteurTempHum_interieur.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  capteurTempHum_exterieur.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  capteurTempHum_exterieur.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  if (capteurTempHum_interieur.isBME280())
    capteurTempHum_interieur.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
  if (capteurTempHum_exterieur.isBME280())
    capteurTempHum_exterieur.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);

  /////////////
  ///  RTC  ///
  /////////////
  Serial.println("intit RTC");
  RTC.begin();
  /*if (!RTC.isRunning())
    {
    RTC.setHourMode(CLOCK_H24);
    RTC.setDateTime(__DATE__, __TIME__);
    }*/
  year = RTC.getYear();
  month = RTC.getMonth();
  day = RTC.getDay();
  hour = RTC.getHours();
  minute = RTC.getMinutes();

  ///////////////////////////////
  ///  Capteur de luminosite  ///
  ///////////////////////////////
  pinMode(PIN_CAPTEUR_LUM, INPUT);

  ////////////////////////////
  ///  BOUTONS ET MOTEURS  ///
  ////////////////////////////
  Serial.println("init capteur et moteur");
  pinMode(PIN_MOTEUR_PORTE_MONTE, OUTPUT);
  pinMode(PIN_MOTEUR_PORTE_DESCEND, OUTPUT);
  pinMode(PIN_MOTEUR_AERATION_OUVRE, OUTPUT);
  pinMode(PIN_MOTEUR_AERATION_FERME, OUTPUT);

  pinMode(PIN_ENDSTOP_PORTE_HAUT, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_PORTE_BAS, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_AERATION_OUVERT, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_AERATION_FERME, INPUT_PULLUP);

  pinMode(PIN_BP_PORTE_MONTE, INPUT_PULLUP);
  pinMode(PIN_BP_PORTE_DESCEND, INPUT_PULLUP);
  pinMode(PIN_BP_AERATION_OUVRE, INPUT_PULLUP);
  pinMode(PIN_BP_AERATION_FERME, INPUT_PULLUP);
  StopMoteurPorte();
  StopMoteurAeration();


  /////////////////////////////////////
  ///  ephemeride et automtisation  ///
  /////////////////////////////////////
  int minutesRTC = hour * 24 + minute; //temps en minutes depuis minuit
  int minutesOuverture =  ephemeride.sunrise(year, month, day, false) + config.openShift;
  int minutesFermeture = ephemeride.sunset(year, month, day, false) + config.closeShift;
  etatPorte = EtatPorteInconnue;
  etatEndstopPorte = 0;
  if (!digitalRead(PIN_ENDSTOP_PORTE_HAUT))
  {
    etatPorte = EnHaut;
    etatEndstopPorte = 1;
  }
  if (!digitalRead(PIN_ENDSTOP_PORTE_BAS))
  {
    etatPorte = EnBas;
    etatEndstopPorte = 2;

  }

  Serial.println("fin init");
}


void loop() {
  // put your main code here, to run repeatedly:
  unsigned long temps = millis();
  ServerLoop();
  ////////////////////////////////////////
  ///  CAPTEUR TEMPERATURE ET HIMIDITE ///
  ////////////////////////////////////////
  if (timeBoucleTemperature < temps)
  {
    //Serial.println("Lance mesures");
    timeBoucleTemperature += config.temperatureLoopTime;
    capteurTempHum_interieur.measure();
    capteurTempHum_exterieur.measure();
    nouvelleMesure = true;
  }

  if (nouvelleMesure && capteurTempHum_interieur.hasValue() && capteurTempHum_exterieur.hasValue())
  {
    //Serial.println("Lit mesures");
    nouvelleMesure = false;
    float tmp;

    posTabTempHum++;
    if (posTabTempHum > TAILLE_TAB_TEMP_HUM)
      posTabTempHum = 0;

    tmp = capteurTempHum_interieur.getTemperature() * 100.0;
    temperaturesInt[posTabTempHum] = tmp;
    tmp = capteurTempHum_interieur.getHumidity() * 100.0;
    humiditeInt[posTabTempHum] = tmp;
    tmp = capteurTempHum_exterieur.getTemperature() * 100.0;
    temperaturesExt[posTabTempHum] = tmp;
    tmp = capteurTempHum_exterieur.getHumidity() * 100.0;
    humiditeExt[posTabTempHum] = tmp;

    Serial.print("ext : Temp  ");
    Serial.print((float)(temperaturesExt[posTabTempHum]) / 100.0);
    Serial.print(" hum ");
    Serial.print((float)(humiditeExt[posTabTempHum]) / 100.0);
    Serial.print(" int : Temp ");
    Serial.print((float)(temperaturesInt[posTabTempHum]) / 100.0);
    Serial.print(" hum ");
    Serial.print((float)(humiditeInt[posTabTempHum]) / 100.0);
    if(etatCapteurLum)
    Serial.println(" Jour");
    else
    Serial.println(" Nuit");



  }

  ///////////////////////////
  ///  BOUTONS ET MOTEURS ///
  ///////////////////////////
  if (timeBoucleBP < temps)
  {
    timeBoucleBP += config.BPLoopTime;

    etatBPPorteMonte = !digitalRead(PIN_BP_PORTE_MONTE);
    if (etatBPPorteMonte != oldEtatBPPorteMonte)
    {
      if (etatBPPorteMonte)
        MontePorte();
      oldEtatBPPorteMonte = etatBPPorteMonte;
    }

    etatBPPorteDescend = !digitalRead(PIN_BP_PORTE_DESCEND);
    if (etatBPPorteDescend != oldEtatBPPorteDescend)
    {
      if (etatBPPorteDescend)
        DescendPorte();
      oldEtatBPPorteDescend = etatBPPorteDescend;
    }

    etatBPAerationOuvre = !digitalRead(PIN_BP_AERATION_OUVRE);
    if (etatBPAerationOuvre != oldEtatBPAerationOuvre)
    {
      if (etatBPAerationOuvre)
        OuvreAeration();
      oldEtatBPAerationOuvre = etatBPAerationOuvre;
    }

    etatBPAerationFerme = !digitalRead(PIN_BP_AERATION_FERME);
    if (etatBPAerationFerme != oldEtatBPAerationFerme)
    {
      if (etatBPAerationFerme)
        FemrmeAeration();
      oldEtatBPAerationFerme = etatBPAerationFerme;
    }
    etatEndstopPorte = 0;
    if (!digitalRead(PIN_ENDSTOP_PORTE_HAUT))
      etatEndstopPorte += 1;
    if (!digitalRead(PIN_ENDSTOP_PORTE_BAS))
      etatEndstopPorte += 2;

    etatEndstopAeration = 0;
    if (!digitalRead(PIN_ENDSTOP_AERATION_OUVERT))
      etatEndstopAeration += 1;
    if (!digitalRead(PIN_ENDSTOP_AERATION_FERME))
      etatEndstopAeration += 2;
  }

  if (etatMoteur)
  {
    if ((etatEndstopPorte == 1 && etatMoteur == 1) || (etatEndstopPorte == 2 && etatMoteur == 2))
    {
      StopMoteurPorte();
    }
  }

  if (etatAeration)
  {
    if ((etatEndstopAeration == 1 && etatAeration == 1) || (etatEndstopAeration == 2 && etatAeration == 2))
    {
      StopMoteurAeration();
    }
  }

  ///////////////////////////////////
  ///  RTC & Capteur de luminosite///
  ///////////////////////////////////
  if (timeBoucleHorloge < temps)
  {
    timeBoucleHorloge += config.mainLoopTime;
    etatCapteurLum = !digitalRead(PIN_CAPTEUR_LUM);

    year = RTC.getYear();
    month = RTC.getMonth();
    day = RTC.getDay();
    hour = RTC.getHours();
    minute = RTC.getMinutes();
    seconds = RTC.getSeconds();

    Serial.print(" ");
    Serial.print(day);
    Serial.print("-");
    Serial.print(month);
    Serial.print("-");
    Serial.print(year);

    Serial.print(" ");

    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.println(seconds);

    if (day != oldDay)
    {
      minutesOuverture = ephemeride.sunrise(year, month, day, false) + config.openShift;
      minutesFermeture = ephemeride.sunset(year, month, day, false) + config.closeShift;
    }

    minutesRTC = hour * 24 + minute; //temps en minutes depuis minuit

    if (minutesRTC >= minutesOuverture && (etatPorte == EnBas || etatPorte == EtatPorteInconnue))
      MontePorte();
    if (minutesRTC >= minutesFermeture && (etatPorte == EnHaut || etatPorte == EtatPorteInconnue))
      DescendPorte();
  }
}

void StopMoteurPorte()
{
  Serial.println("stop porte");
  digitalWrite(PIN_MOTEUR_PORTE_DESCEND, 1);
  digitalWrite(PIN_MOTEUR_PORTE_MONTE, 1);
  etatPorte = EtatPorteInconnue;

  if (etatEndstopPorte == 1)
    etatPorte = EnHaut;

  if (etatEndstopPorte == 2)
    etatPorte = EnBas;
}

void MontePorte()
{
  if ((etatEndstopPorte & 1) == 0)
  {
    Serial.println("monte porte");
    digitalWrite(PIN_MOTEUR_PORTE_DESCEND, 1);
    delay(100);
    digitalWrite(PIN_MOTEUR_PORTE_MONTE, 0);
    etatPorte = EnMontee;
    etatMoteur = 1;
  }
  else
  {
    StopMoteurPorte();
  }
}

void DescendPorte()
{
  if ((etatEndstopPorte & 2) == 0)
  {
    Serial.println("descend porte");
    digitalWrite(PIN_MOTEUR_PORTE_MONTE, 1);
    delay(100);
    digitalWrite(PIN_MOTEUR_PORTE_DESCEND, 0);
    etatPorte = EnDescente;
    etatMoteur = 2;
  }
  else
  {
    StopMoteurPorte();
  }
}

void StopMoteurAeration()
{
  Serial.println("stop aeration");
  digitalWrite(PIN_MOTEUR_AERATION_FERME, 1);
  digitalWrite(PIN_MOTEUR_AERATION_OUVRE, 1);
  etatAeration = 0;
}

void OuvreAeration()
{
  Serial.println("ouvre aeration");
  if ((etatEndstopAeration & 1) == 0)
  {
    digitalWrite(PIN_MOTEUR_AERATION_FERME, 1);
    delay(100);
    digitalWrite(PIN_MOTEUR_AERATION_OUVRE, 0);
    etatAeration = 1;
  }
  else
  {
    StopMoteurAeration();
  }
}

void FemrmeAeration()
{
   Serial.println("ferme aeration");
  if ((etatEndstopAeration & 2) == 0)
  {
    digitalWrite(PIN_MOTEUR_AERATION_OUVRE, 1);
    delay(100);
    digitalWrite(PIN_MOTEUR_AERATION_FERME, 0);
    etatAeration = 2;
  }
  else
  {
    StopMoteurAeration();
  }
}
