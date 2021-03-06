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

#define DOORCLOSED 9 // angle when closed
#define DOOROPEN 131 // angle when open
#define DOORPIN 10 // pin for servo
#define SERVODISABLE 13 // when HIGH servo is disabled

#include <Servo.h>
Servo doorServo;  // create servo object to control a servo
int doorSpeed = 100;  // how many milliseconds delay between degrees
byte position,lastPosition = DOOROPEN;

Button orbit_button = Button(BUTTON_ORBIT);
Button sf_button = Button(BUTTON_SF);

int orbit_status = -1;
int sf_status = -1;
unsigned long orbit_timeout = 0;
unsigned long sf_timeout = 0;
unsigned long time = 0;

const char okResponse[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n\r\n"
    "<head><style> td, a { font-size: 5em; } table, .btn { display: block; }"
    ".on { background-color: green; } .off { background-color: red; }"
    "</style></head><body><table><tr><td>garden $F</td><td>hose $F</td></tr>"
    "<tr><td><a class='btn on' href=/orbit/on/10>on</a></td>"
        "<td><a class='btn on' href=/sf/on/10>on</a></td></tr>"
    "<tr><td><a class='btn off' href=/orbit/off>off</a></td>"
        "<td><a class='btn off' href=/sf/off>off</a></td></tr>"
    "<tr><td>$D</td><td>$D</td></tr>" // times remaining for both waters
    "<tr><td>door position is</td><td>$D</td></tr>"
    "</table></body>"
;

const char on[] PROGMEM = "ON";
const char off[] PROGMEM = "OFF";
const char unknown[] PROGMEM = "?";

#define STATUS_STR(status_int) (status_int > 0 ? on : (status_int == 0 ? off : unknown))

const char redirectHeader[] PROGMEM =
    "HTTP/1.1 302\r\n"
    "Pragma: no-cache\r\n"
    "Location: /\r\n"
;

void setup() {
  pinMode(DOORPIN,OUTPUT);
  pinMode(SERVODISABLE,OUTPUT);
  digitalWrite(SERVODISABLE,HIGH);
  doorServo.attach(DOORPIN);
  doorServo.write(DOOROPEN);
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
  time = millis();
  if (sf_timeout && time >= sf_timeout) {
    sf_off();
    sf_timeout = 0;
  }
  if (orbit_timeout && time >= orbit_timeout) {
    orbit_off();
    orbit_timeout = 0;
  }
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  // check if valid tcp data is received
  if (pos) {
    bfill = ether.tcpOffset();
    char* data = (char *) Ethernet::buffer + pos;
    Serial.println(data);
    // receive buf hasn't been clobbered by reply yet
    if (strncmp("GET /orbit/on", data, 13) == 0) {
      orbit_on();
      orbit_timeout = time + (unsigned long)atoi(data+14) * 60000;
    }
    else if (strncmp("GET /orbit/off", data, 14) == 0) {
      orbit_off();
    }
    else if (strncmp("GET /sf/on", data, 10) == 0) {
      sf_on();
      sf_timeout = time + (unsigned long)atoi(data+11) * 60000;
    }
    else if (strncmp("GET /sf/off", data, 11) == 0) {
      sf_off();
    }
    else if (strncmp("GET /foodop", data, 11) == 0) {
      position = DOOROPEN;
      door(position);
    }
    else if (strncmp("GET /foodcl", data, 11) == 0) {
      position = DOORCLOSED;
      door(position);
    }
    else {
      bfill.emit_p(okResponse, STATUS_STR(orbit_status), STATUS_STR(sf_status), orbit_timeout - time, sf_timeout - time, position);
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
void emit_status(int water_status, unsigned long timeout, BufferFiller& buf) {
  if (water_status == 0)
    buf.emit_p(PSTR("OFF "));
  else if (water_status == 1)
    buf.emit_p(PSTR("ON "));
  else
    buf.emit_p(PSTR("? "));

  if (timeout) {
    buf.emit_p(PSTR("$F"), timeout - time);
  }
}

void door(int angle) {
  doorServo.write(angle);
  digitalWrite(SERVODISABLE,LOW);
  int step = 1; // must be 1 for "i != angle" to work
  if (angle < lastPosition) step *= -1; // count the right direction
  for(int i=lastPosition; i != angle;  i += step){
    doorServo.write(i);
    delay(doorSpeed);
  }
  doorServo.write(angle);
  delay(1000); // wait for servo to arrive before disabling it
  digitalWrite(SERVODISABLE,HIGH);
  lastPosition = angle;
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
