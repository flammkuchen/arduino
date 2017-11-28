///////////////////////////////////////////////////////////////
//
// Arduino-Sketch: Über Shield an Ethernet anmelden
//                  a) mit statischer Adresse, oder
//                  b) mit dynamischer Adresse über DHCP
//
///////////////////////////////////////////////////////////////

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <stdlib.h>

// Wir schließen eine grüne LED an Digital-Pin 6
// und eine rote LED an Digital-Pin 7 an.
const int greenLED = 6;
const int redLED = 7;

// Temperaturvariablen
int LM35 = A0;
float SensorValue = 0;
float temperatur = 0;

// Lichtsensor (Fotowiderstand)
int eingangLicht = A1;
int helligkeit = 0;

///////////////////////////////////////////////////////////////
// Wir benötigen eine MAC-Adresse.
// Sofern das Ethernet Shield keine Adresse 
// (meist als aufgeklebter Sticker)
// besitzt, müssen wir unsere eigene erfinden:
///////////////////////////////////////////////////////////////

byte mac[] = {
  0x00, 0xCC, 0xBB, 0xAA, 0xDE, 0x02
};
IPAddress server(192, 168, 2, 112); //mqtt broker (laptop oder raspi)

// Die nachfolgende Methode wird aufgerufen, 
// sobald eine Nachricht  für das angegebene Topic eintrifft

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht eingetroffen [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  // Zusätzlich zur Ausgabe über den seriellen Monitor
  // konvertieren wir die Nachricht in eine Zeichenkette
  // und anschließend in eine Ganzzahl:
  char *buffer = (char *) malloc(length);
  strcpy(buffer, (char *)payload);
  int blinks = String(buffer).toInt();


  // Kam als Wert 0, gibt es keine ungelesenen Nachrichten
  // => Wir lassen die grüne LED für 3 Sekunden leuchten:
  if (blinks == 0) {
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    delay(3000);
    digitalWrite(greenLED, LOW);
  }
  // Kam als Wert > 0, soll die rote LED so oft blinken
  // wie es ungelesene Nachrichten gibt:
  else {
    digitalWrite(greenLED, LOW);
    for (int i = 0; i < blinks; i++) {
      digitalWrite(redLED, HIGH);
      delay(1000);
      digitalWrite(redLED, LOW);
      delay(1000);
    }
  }
}
// EthernetClient: API zum Zugriff auf das Ethernet Shield
EthernetClient ethClient;
PubSubClient client(ethClient);

void sendSensorData(){
  if (client.connected()){
    String tempString = String(temperatur, 1);
    char charBuf[50];
    tempString.toCharArray(charBuf, 50); 
    client.publish("/haus/heizungsraum/temperatur", charBuf);

    String lightString = String(helligkeit);
    lightString.toCharArray(charBuf, 50); 
    client.publish("/haus/heizungsraum/helligkeit", charBuf);

    if (helligkeit > 130)
      client.publish("/haus/heizungsraum/lichtIstAn", "1");
    else
      client.publish("/haus/heizungsraum/lichtIstAn", "0");
  }
}

void reconnect() {
  // Solange wiederholen bis Verbindung wiederhergestellt ist
  while (!client.connected()) {
    Serial.print("Versuch des MQTT Verbindungsaufbaus...");
    //Versuch die Verbindung aufzunehmen
    if (client.connect("arduinoClient")) {
      Serial.println("Erfolgreich verbunden!");
      // Nun versendet der Arduino eine Nachricht in outTopic ...
      sendSensorData();
      // und meldet sich für bei inTopic für eingehende Nachrichten an
      // client.subscribe("inTopic");
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println(" Nächster Versuch in 5 Sekunden");
      // 5 Sekunden Pause vor dem nächsten Versuch
      delay(5000);
    }
  }
}

void setup() {
  pinMode(redLED, OUTPUT); // rote LED
  pinMode(greenLED, OUTPUT); // grüne LED
  Serial.begin(9600); // Serielle Verbindung öffnen
  // Broker erreichbar über ip-Adresse = server, port = 1883
  client.setServer(server, 1883); // Adresse des MQTT-Brokers
  client.setCallback(callback);   // Handler für eingehende Nachrichten
  
  //  Ethernet-Verbindung öffnen und DHCP um IP-Adresse bitten:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("IP-Adressanfrage über DHCP fehlgeschlagen");
    
    while (true) {} // Unendliche Warteschleife
  }

  // Kleine Pause
  delay(1500);
  
  // Adresse über seriellen Port ausgeben:
  Serial.print("Meine IP Adresse lautet: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  //Temperatur und helligkeit lesen, durchschnitt ermitteln, anzeigen und senden
  float sumTemperature = 0;
  int sumHelligkeit = 0;
  for (int i = 1; i <= 5; i++){
    sumHelligkeit = sumHelligkeit + analogRead(eingangLicht); //Die Spannung an dem Fotowiderstand auslesen
    sumTemperature = sumTemperature + (5.0 * analogRead(LM35) * 100.0) / 1024;
    delay(1000); // 1 Sekunde Pause zwischen den Messungen machen
  }
    
  temperatur = sumTemperature / 5;
  helligkeit = sumHelligkeit / 5;

  Serial.print("Temperatur: ");
  Serial.print(temperatur, 1); //Ausgabe der Temperatur mit einer Nachkommastelle
  Serial.print(" °C");
  Serial.print("\n");
  Serial.print("Helligkeit: "); //Ausgabe der Temperatur mit einer Nachkommastelle
  Serial.println(helligkeit);
  
  // Solange probieren bis es klappt:
  if (!client.connected()) {
    reconnect();//verbindet und sendet
  }
  else {
    sendSensorData(); //sendet nur
  }
  client.loop();
  //DHCP am Laufen halten
  DhcpMaintain();
}

void DhcpMaintain(){
//////////////////////////////////////////////////////////////
  // IP-Adressen werden von DHCP nur bis zu  einer 
  // "Haltbarkeitsfrist" vergeben. Daher ist nach 
  // einer bestimmten Zeit ein Neuabonnement nötig:
  //////////////////////////////////////////////////////////////  
  switch (Ethernet.maintain())
  {
    case 1:
      Serial.println("Fehler: Adresserneuerung abgelehnt");
      break;
    case 2:
      Serial.println("Adresserneuerung erfolgreich");
      break;
    case 3:
      Serial.println("Fehler: Rebinding fehlgeschlagen");
      break;
    case 4:
      Serial.println("Rebinding erfolgreich");
      break;
    default: // Nichts ist passiert
      break;
  }  
}

