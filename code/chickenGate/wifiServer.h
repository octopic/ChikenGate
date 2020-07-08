
#ifndef WIFISERVER_H
#define WIFISERVER_H

#include <WiFi.h>
#include <WebServer.h>

extern short temperaturesInt[];
extern short humiditeInt[];
extern short temperaturesExt[];
extern short humiditeExt[];
extern int posTabTempHum;
extern byte year;
extern byte month;
extern byte day;
extern byte hour;
extern byte minute;
extern bool etatCapteurLum;
extern int minutesOuverture;
extern int minutesFermeture;
extern byte etatMoteur; //-1 descend, 0 arret, 1 monte
extern byte etatAeration; //-1 ferme, 0 arret, 1 ouvre

extern bool etatBPPorteMonte ;
extern bool etatBPPorteDescend;
extern bool etatBPAerationOuvre;
extern bool etatBPAerationFerme;

extern byte etatEndstopPorte;
extern byte etatEndstopAeration;
//extern enum enumEtatPorte etatPorte;
extern void MontePorte();
extern void DescendPorte();
extern void StopMoteurPorte();
extern void StopMoteurAeration();
extern void OuvreAeration();
extern void FemrmeAeration();

void InitWIFI();
void ServerLoop();



#endif
