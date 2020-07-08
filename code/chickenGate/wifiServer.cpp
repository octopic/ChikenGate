#include "wifiServer.h"
//https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/

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
  ptr += "<title>LED Control</title>\n";
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
  ptr += "<h1>ESP32 Web Server</h1>\n";
  ptr += "<h3>Using Access Point(AP) Mode</h3>\n";

  if (etatMoteur == 1)
  {
    ptr += "<p>Porte Status: Monte</p><a class=\"button button-off\" href=\"/porteMonteOff\">Arret</a>\n";
    ptr += "<p>Porte Status: Monte</p><a class=\"button button-on\" href=\"/porteDescendOn\">Descend</a>\n";
  }
  else if (etatMoteur == 2)
  {
    ptr += "<p>Porte Status: Descend</p><a class=\"button button-on\" href=\"/porteMonteOn\">Monte</a>\n";
    ptr += "<p>Porte Status: Descend</p><a class=\"button button-off\" href=\"/porteDescendOff\">Arret</a>\n";
  }
  else
  {
    ptr += "<p>Porte Status: Arret</p><a class=\"button button-on\" href=\"/porteMonteOn\">Monte</a>\n";
    ptr += "<p>Porte Status: Arret</p><a class=\"button button-on\" href=\"/porteDescendOn\">Descend</a>\n";
  }
  
  if (etatAeration == 1)
  {
    ptr += "<p>Aeration Status: Ouvre</p><a class=\"button button-off\" href=\"/aerationOuvreOff\">Arret</a>\n";
    ptr += "<p>Aeration Status: Ouvre</p><a class=\"button button-on\" href=\"/aerationFermeOn\">Ferme</a>\n";
  }
  else if (etatAeration == 2)
  {
    ptr += "<p>Aeration Status: Ferme</p><a class=\"button button-on\" href=\"/aerationOuvreOn\">Ouvre</a>\n";
    ptr += "<p>Aeration Status: Ferme</p><a class=\"button button-off\" href=\"/aerationFermeOff\">Arret</a>\n";
  }
  else
  {
    ptr += "<p>Aeration Status: Arret</p><a class=\"button button-on\" href=\"/aerationOuvreOn\">Ouvre</a>\n";
    ptr += "<p>Aeration Status: Arret</p><a class=\"button button-on\" href=\"/aerationFermeOn\">Ferme</a>\n";
  }
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
