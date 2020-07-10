#include "wifiServer.h"
//https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/

#include "config.h"
extern configuration config;

const char* ssid = "ChickenGate";
const char* password = "PiouPiouPiou";

/* Put IP Address details */
//IPAddress local_ip(192, 168, 1, 1);
//IPAddress gateway(192, 168, 1, 1);
//IPAddress subnet(255, 255, 255, 0);

WebServer server(80);



String SendHTML()
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ChickenGate</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ChickenGate</h1>\n";
  ptr += "<h3>Gestion de poulailler Piou Piou</h3>\n";

  ptr += "<p>Date : " ;ptr += day ;ptr += "/" ;ptr += month ;ptr += "/" ;ptr += year ;ptr += " " ;ptr += hour ;ptr += ":" ;ptr += minute ;ptr += " + decalage " ; ptr += config.GMTshift ; ptr += "h</p>\n";
  ptr += "<p>Heure de lever de soleil : "; ptr += minutesOuverture / 60 ; ptr += ":" ; ptr += minutesOuverture % 60 ; ptr +=  " + declage " ; ptr += config.openShift ; ptr += "min </p>\n";
  ptr += "<p>Heure de coucher de soleil : " ; ptr += minutesFermeture / 60 ; ptr += ":" ; ptr += minutesFermeture % 60; ptr += " + declage " ; ptr += config.closeShift ; ptr += "min </p>\n";

  ptr += "<p>Interieur : Temperature "; ptr += (float)(temperaturesInt[posTabTempHum])/100.0; ptr += "°C, hygrometire : "; ptr += (float)(humiditeInt[posTabTempHum])/100.0; ptr += "% </p> ";

  ptr += "<p>Exterieur : Temperature "; ptr += (float)(temperaturesExt[posTabTempHum])/100.0; ptr += "°C, hygrometire : "; ptr += (float)(humiditeExt[posTabTempHum])/100.0; ptr += "% </p> ";

  ptr += "<p>Capteur de luminausite : ";
  if (etatCapteurLum)
    ptr += "jour </p> \n";
  else
    ptr += "nuit </p> \n";



  if (etatMoteur == 1)
  {
    ptr += "<p>Porte Status: Monte </p> \n";
    ptr += "<a class = \"button button-off\" href=\"/porteMonteOff\">Arret</a>\n";
    ptr += "<a class=\"button button-on\" href=\"/porteDescendOn\">Descend</a>\n";
  }
  else if (etatMoteur == 2)
  {
    ptr += "<p>Porte Status: Decsend</p>\n";
    ptr += "<a class=\"button button-on\" href=\"/porteMonteOn\">Monte</a>\n";
    ptr += "<a class=\"button button-off\" href=\"/porteDescendOff\">Arret</a>\n";
  }
  else
  {
    ptr += "<p>Porte Status: Arret</p>\n";
    ptr += "<a class=\"button button-on\" href=\"/porteMonteOn\">Monte</a>\n";
    ptr += "<a class=\"button button-on\" href=\"/porteDescendOn\">Descend</a>\n";
  }

  if (etatAeration == 1)
  {
    ptr += "<p>Aeration Status: Ouvre</p>\n";
    ptr += "<a class=\"button button-off\" href=\"/aerationOuvreOff\">Arret</a>\n";
    ptr += "<a class=\"button button-on\" href=\"/aerationFermeOn\">Ferme</a>\n";
  }
  else if (etatAeration == 2)
  {
    ptr += "<p>Aeration Status: Ferme</p>\n";
    ptr += "<a class=\"button button-on\" href=\"/aerationOuvreOn\">Ouvre</a>\n";
    ptr += "<a class=\"button button-off\" href=\"/aerationFermeOff\">Arret</a>\n";
  }
  else
  {
    ptr += "<p>Aeration Status: Arret</p>\n";
    ptr += "<a class=\"button button-on\" href=\"/aerationOuvreOn\">Ouvre</a>\n";
    ptr += "<a class=\"button button-on\" href=\"/aerationFermeOn\">Ferme</a>\n";
  }

  ptr += "<p>Etat des fin de courses porte : Haut " ;ptr += etatEndstopPorte & 1 ;ptr += ", Bas " ;ptr += etatEndstopPorte & 2 ;ptr += "</p>\n";
  ptr += "<p>Etat des fin de courses aeration : Haut " ;ptr += etatEndstopAeration & 1 ;ptr += ", Bas " ;ptr += etatEndstopAeration & 2 ;ptr += "</p>\n";
  return ptr;
}

void handle_OnConnect()
{
  server.send(200, "text/html", SendHTML());
}

void handle_porteMonteOn()
{
  MontePorte();
  server.send(200, "text/html", SendHTML());
}

void handle_porteMonteOff()
{
  StopMoteurPorte();
  server.send(200, "text/html", SendHTML());
}

void handle_porteDescendOn()
{
  DescendPorte();
  server.send(200, "text/html", SendHTML());
}

void handle_porteDescendOff()
{
  StopMoteurPorte();
  server.send(200, "text/html", SendHTML());
}

void handle_aerationOuvreOn()
{
  OuvreAeration();
  server.send(200, "text/html", SendHTML());
}

void handle_aerationOuvreOff()
{
  StopMoteurAeration();
  server.send(200, "text/html", SendHTML());
}

void handle_aerationFermeOn()
{
  FemrmeAeration();
  server.send(200, "text/html", SendHTML());
}

void handle_aerationFermeOff()
{
  StopMoteurAeration();
  server.send(200, "text/html", SendHTML());
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}



void InitWIFI()
{
  Serial.print("setting server ");
  Serial.println(ssid);
  //WiFi.begin(ssid, password);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  //WiFi.softAPConfig(local_ip, gateway, subnet);

  Serial.print("AP IP address: ");
  Serial.println(IP);
  delay(100);
  server.on("/", handle_OnConnect);
  server.on("/porteMonteOn", handle_porteMonteOn);
  server.on("/porteDescendOn", handle_porteDescendOn);
  server.on("/aerationOuvreOn", handle_aerationOuvreOn);
  server.on("/aerationFermeOn", handle_aerationFermeOn);
  server.on("/porteMonteOff", handle_porteMonteOff);
  server.on("/porteDescendOff", handle_porteDescendOff);
  server.on("/aerationOuvreOff", handle_aerationOuvreOff);
  server.on("/aerationFermeOff", handle_aerationFermeOff);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void ServerLoop()
{
  server.handleClient();
}
