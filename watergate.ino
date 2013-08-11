#include <EtherCard.h>
#include "button.h"
static byte myip[] = { 192,168,1,95 };
static byte gwip[] = { 192,168,1,1 };
static byte mymac[] = { 0xDE,0xAD,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[1024]; // tcp/ip send and receive buffer
static BufferFiller bfill;  // used as cursor while filling the buffer

#define PIN_SF A0 // sparkfun valve. set PIN_SF high for on, low for off

#define PIN_ON A2  // orbit valve. pulse PIN_ON for on, pulse PIN_OFF for off
#define PIN_OFF A1

#define BUTTON_ORBIT 3
#define BUTTON_SF 2

Button orbit_button = Button(BUTTON_ORBIT);
Button sf_button = Button(BUTTON_SF);

int orbit_status = -1;
int sf_status = -1;

char okHeader[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
;

char redirectHeader[] PROGMEM =
    "HTTP/1.1 302\r\n"
    "Pragma: no-cache\r\n"
    "Location: /\r\n"
;

void setup() {
  pinMode(PIN_SF, OUTPUT);
  pinMode(PIN_ON, OUTPUT);
  pinMode(PIN_OFF, OUTPUT);
  digitalWrite(PIN_SF, LOW);
  digitalWrite(PIN_ON, LOW);
  digitalWrite(PIN_OFF, LOW);

  Serial.begin(57600);
  Serial.println("\n[backSoon]");

  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0) 
    Serial.println( "Failed to access Ethernet controller");

  ether.staticSetup(myip, gwip);

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
}

void loop() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  // check if valid tcp data is received
  if (pos) {
    bfill = ether.tcpOffset();
    char* data = (char *) Ethernet::buffer + pos;
    Serial.println(data);
    // receive buf hasn't been clobbered by reply yet
    if (strncmp("GET /orbit/on", data, 13) == 0)
      orbit_on();
    else if (strncmp("GET /orbit/off", data, 14) == 0)
      orbit_off();
    else if (strncmp("GET /sf/on", data, 10) == 0)
      sf_on();
    else if (strncmp("GET /sf/off", data, 11) == 0)
      sf_off();
    else {
      bfill.emit_p(PSTR("$F\r\n"
        "<head><style> body, a { font-size: 5em; } table, button { width: 100%; }"
        ".on { background-color: green; } .off { background-color: red; }"
        "</style></head><body><table><tr><td>orbit</td><td>garden</td></tr>"
        "<tr><td><button class='on'><a href=/orbit/on/10>on</a></button></td>"
        "<td><button class='on'><a href=/sf/on/10>on</a></button></td></tr>"
        "<tr><td><button class='off'><a href=/orbit/off>off</a></button></td>"
        "<td><button class='off'><a href=/sf/off>off</a></button></td></tr>"
        "<tr><td>"
          ), okHeader);
      emit_status(orbit_status, bfill);
      bfill.emit_p(PSTR("</td><td>"));
      emit_status(sf_status, bfill);
      bfill.emit_p(PSTR("</td></tr></table></body>"));
      ether.httpServerReply(bfill.position()); // send web page data
      return;
    }

    bfill.emit_p(PSTR("$F\r\n"), redirectHeader);
    ether.httpServerReply(bfill.position()); // redirect to /
  }
/*
  if (orbit_button.pressed()) {
    Serial.println("Orbit button");
    if (orbit_status) {
      orbit_off();
    } else {
      orbit_on();
    }
  }

  if (sf_button.pressed()) {
    Serial.println("SF button");
    if (sf_status) {
      sf_off();
    } else {
      sf_on();
    }
  }
*/
}
void emit_status(int water_status, BufferFiller& buf) {
  if (water_status == 0)
    buf.emit_p(PSTR("OFF"));
  else if (water_status == 1)
    buf.emit_p(PSTR("ON"));
  else
    buf.emit_p(PSTR("?"));
}

void orbit_on() {
  Serial.println("Orbit on");
  pulse(PIN_ON);
  orbit_status = 1;
}

void orbit_off() {
  Serial.println("Orbit off");
  pulse(PIN_OFF);
  orbit_status = 0;
}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}

void sf_on() {
  Serial.println("SF on");
  digitalWrite(PIN_SF, HIGH);
  sf_status = 1;
}

void sf_off() {
  Serial.println("SF off");
  digitalWrite(PIN_SF, LOW);
  sf_status = 0;
}
