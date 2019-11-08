// THeREMIN-Air
// Diego Parbuono

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include "AppleMidi.h"

#define NODE_NUMBER 

Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

String gNodeName;
int trigPin = 5;    // TriggerD1
int echoPin = 4;    // EchoD2
long duration, cm;
int sens = 5;
int cm_new = 20;
int cm_old = 20;
unsigned long t0 = millis();
bool isConnected = false;

APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h

// Forward declaration
void OnAppleMidiConnected(uint32_t ssrc, char* name);
void OnAppleMidiDisconnected(uint32_t ssrc);
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity);
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity);

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  // Serial communications and wait for port to open:
  Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  WiFiManager wifiManager;
  Serial.print(F("Getting IP address..."));

 wifiManager.autoConnect("AutoConnectAP");
 wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F(""));
  Serial.println(F("WiFi connected"));


  Serial.println();
  Serial.print(F("IP address is "));
  Serial.println(WiFi.localIP());

  Serial.println(F("OK, now make sure you an rtpMIDI session that is Enabled"));
  Serial.print(F("Add device named Arduino with Host/Port "));
  Serial.print(WiFi.localIP());
  Serial.println(F(":5004"));
  Serial.println(F("Then press the Connect button"));
  Serial.println(F("Then open a MIDI listener (eg MIDI-OX) and monitor incoming notes"));
  gNodeName = String("Theremin-") + String(NODE_NUMBER);
  
  MDNS.begin(gNodeName.c_str());
 
  // Create a session and wait for a remote host to connect to us
  AppleMIDI.begin(gNodeName.c_str());
  const char* service = "apple-midi";
  const char* protocol = "udp";
  int port = 5004;
  Serial.print("Adding mDNS service ");
  Serial.print(service);
  Serial.print(" ");
  Serial.print(protocol);
  Serial.print(" ");
  Serial.println(port);
  MDNS.addService(service, protocol, port);

  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  AppleMIDI.OnReceiveNoteOff(OnAppleMidiNoteOff);

  Serial.println(F("Sending NoteOn/Off of note 45, every second"));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  MDNS.update();
  yield();
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  
  // Convert the time into a distance
  cm_new = map(constrain(duration, 20, 3000), 50, 2000, 0, 84); 
  Serial.print(cm_new);
  Serial.print("value_midi, ");
  Serial.println();
  delay(250);
  yield();
  AppleMIDI.read();


 
if (cm_new != cm_old) // only if a new sensor value is detected, a new nite will be send. IF the object stays stable infront of the sensor, no note will be send.
{
    
    AppleMIDI.sendNoteOff(cm_old, 0, 1);
     yield();
    AppleMIDI.sendNoteOn(cm_new,127,1);
    yield();
    
 // AppleMIDI.sendControlChange(1,cm_new,10);
    cm_old = cm_new;
     yield();
    
  }
  delay(30);
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnAppleMidiConnected(uint32_t ssrc, char* name) {
  isConnected  = true;
  Serial.print(F("Connected to session "));
  Serial.println(name);
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnAppleMidiDisconnected(uint32_t ssrc) {
  isConnected  = false;
  Serial.println(F("Disconnected"));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity) {
  Serial.print(F("Incoming NoteOn from channel:"));
  Serial.print(channel);
  Serial.print(F(" note:"));
  Serial.print(note);
  Serial.print(F(" velocity:"));
  Serial.print(velocity);
  Serial.println();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity) {
  Serial.print(F("Incoming NoteOff from channel:"));
  Serial.print(channel);
  Serial.print(F(" note:"));
  Serial.print(note);
  Serial.print(F(" velocity:"));
  Serial.print(velocity);
  Serial.println();
}


void OnAppleMidiControlChange(byte channel, byte note, byte value)
{
  Serial.print(F("Got control change "));
  Serial.print(note);
  Serial.print(F(" - "));
  Serial.print(value);
  Serial.println();
    }
