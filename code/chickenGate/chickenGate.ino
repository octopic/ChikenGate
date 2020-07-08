#include <Wire.h>
#include "config.h"
#include "wifiServer.h"

extern configuration *config;
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

/////////////////////////////////////
///  RTC & Capteur de luminosite  ///
/////////////////////////////////////
#include <RTC.h>
static DS3231 RTC;
unsigned long timeBoucleHorloge = 0;
byte year, month, day, hour, minute, oldDay = 0;

#define PIN_CAPTEUR_LUM 33
bool etatCapteurLum = false;

/////////////////////////////////////
///  ephemeride et automtisation  ///
/////////////////////////////////////
#include "Dusk2Dawn.h"
Dusk2Dawn ephemeride(config->longitude, config->latitude , config->GMTshift);

int minutesRTC = 0; //temps en minutes depuis minuit
int minutesOuverture = 0;
int minutesFermeture = 0;
enum enumEtatPorte {EnBas, EnMontee, EnHaut, EnDescente, EtatPorteInconnue};
enumEtatPorte etatPorte;


void setup() {
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
  if (!RTC.isRunning())
  {
    RTC.setHourMode(CLOCK_H24);
    RTC.setDateTime(__DATE__, __TIME__);
  }
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
  Serial.println("fin init");

  /////////////////////////////////////
  ///  ephemeride et automtisation  ///
  /////////////////////////////////////
  int minutesRTC = hour * 24 + minute; //temps en minutes depuis minuit
  int minutesOuverture =  ephemeride.sunrise(year, month, day, false) + config->openShift;
  int minutesFermeture = ephemeride.sunset(year, month, day, false) + config->closeShift;
  etatPorte = EtatPorteInconnue;
  if (!digitalRead(PIN_ENDSTOP_PORTE_HAUT))
  {
    etatPorte = EnHaut;
  }
  if (!digitalRead(PIN_ENDSTOP_PORTE_BAS))
  {
    etatPorte = EnBas;
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  unsigned long temps = millis();

  ////////////////////////////////////////
  ///  CAPTEUR TEMPERATURE ET HIMIDITE ///
  ////////////////////////////////////////
  if (timeBoucleTemperature < temps)
  {
    Serial.println("Lance mesures");
    timeBoucleTemperature += config->temperatureLoopTime;
    capteurTempHum_interieur.measure();
    capteurTempHum_exterieur.measure();
    nouvelleMesure = true;
  }

  if (nouvelleMesure && capteurTempHum_interieur.hasValue() && capteurTempHum_exterieur.hasValue())
  {
    Serial.println("Lit mesures");
    nouvelleMesure = false;
    float tmp;
    tmp = capteurTempHum_interieur.getTemperature() * 100.0;
    temperaturesInt[posTabTempHum] = tmp;
    tmp = capteurTempHum_interieur.getHumidity() * 100.0;
    humiditeInt[posTabTempHum] = tmp;
    tmp = capteurTempHum_exterieur.getTemperature() * 100.0;
    temperaturesExt[posTabTempHum] = tmp;
    tmp = capteurTempHum_exterieur.getHumidity() * 100.0;
    humiditeExt[posTabTempHum] = tmp;

    Serial.print("ext : Temp  ");
    Serial.print(temperaturesExt[posTabTempHum]);
    Serial.print(" hum ");
    Serial.print(humiditeExt[posTabTempHum]);
    Serial.print("int : Temp ");
    Serial.print(temperaturesInt[posTabTempHum]);
    Serial.print(" hum ");
    Serial.println(humiditeInt[posTabTempHum]);


    posTabTempHum++;
    if (posTabTempHum > TAILLE_TAB_TEMP_HUM)
      posTabTempHum = 0;
  }

  ///////////////////////////
  ///  BOUTONS ET MOTEURS ///
  ///////////////////////////
  if (timeBoucleBP < temps)
  {
    timeBoucleBP += config->BPLoopTime;

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
    Serial.print("BP Mont Desc Ouv Fer SWH SWB SWO SWF MotPort MotAer");
    Serial.print(etatBPPorteMonte);
    Serial.print(etatBPPorteDescend);
    Serial.print(etatBPAerationOuvre);
    Serial.print(etatBPAerationFerme)  ;
    Serial.print(digitalRead(PIN_ENDSTOP_PORTE_HAUT));
    Serial.print(digitalRead(PIN_ENDSTOP_PORTE_BAS));
    Serial.print(digitalRead(PIN_ENDSTOP_AERATION_OUVERT));
    Serial.print(digitalRead(PIN_ENDSTOP_AERATION_FERME));
    Serial.print(etatMoteur);
    Serial.println(etatAeration);

  }

  if (etatMoteur)
  {
    if ((!digitalRead(PIN_ENDSTOP_PORTE_HAUT) && etatMoteur == 1) || (!digitalRead(PIN_ENDSTOP_PORTE_BAS) && etatMoteur == 2))
    {
      StopMoteurPorte();
    }
  }

  if (etatAeration)
  {
    if ((!digitalRead(PIN_ENDSTOP_AERATION_OUVERT) && etatAeration == 1) || (!digitalRead(PIN_ENDSTOP_AERATION_FERME) && etatAeration == 2))
    {
      StopMoteurAeration();
    }
  }
  ///////////////////////////////////
  ///  RTC & Capteur de luminosite///
  ///////////////////////////////////
  if (timeBoucleHorloge < temps)
  {
    timeBoucleHorloge += config->mainLoopTime;
    etatCapteurLum = !digitalRead(PIN_CAPTEUR_LUM);

    year = RTC.getYear();
    month = RTC.getMonth();
    day = RTC.getDay();
    hour = RTC.getHours();
    minute = RTC.getMinutes();

    Serial.print(" ");
    Serial.print(RTC.getDay());
    Serial.print("-");
    Serial.print(RTC.getMonth());
    Serial.print("-");
    Serial.print(RTC.getYear());

    Serial.print(" ");

    Serial.print(RTC.getHours());
    Serial.print(":");
    Serial.print(RTC.getMinutes());
    Serial.print(":");
    Serial.println(RTC.getSeconds());

    if (day != oldDay)
    {
      minutesOuverture =  ephemeride.sunrise(year, month, day, false) + config->openShift;
      minutesFermeture = ephemeride.sunset(year, month, day, false) + config->closeShift;
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
  digitalWrite(PIN_MOTEUR_PORTE_DESCEND, 1);
  digitalWrite(PIN_MOTEUR_PORTE_MONTE, 1);
  etatPorte = EtatPorteInconnue;


  if (!digitalRead(PIN_ENDSTOP_PORTE_HAUT))
    etatPorte = EnHaut;

  if (!digitalRead(PIN_ENDSTOP_PORTE_BAS))
    etatPorte = EnBas;

}

void MontePorte()
{
  if (digitalRead(PIN_ENDSTOP_PORTE_HAUT))
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
  if (digitalRead(PIN_ENDSTOP_PORTE_BAS))
  {
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
  digitalWrite(PIN_MOTEUR_AERATION_FERME, 1);
  digitalWrite(PIN_MOTEUR_AERATION_OUVRE, 1);
  etatAeration = 0;
}

void OuvreAeration()
{
  if (digitalRead(PIN_ENDSTOP_AERATION_OUVERT))
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
  if (digitalRead(PIN_ENDSTOP_AERATION_FERME))
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
