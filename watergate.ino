#include <EtherCard.h>

static byte myip[] = { 
  192,168,13,95 };
static byte gwip[] = { 
  192,168,13,1 };
static byte mymac[] = {  
  0xDE,0xAD,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer
static BufferFiller bfill;  // used as cursor while filling the buffer

#define PIN_SF A0 // sparkfun valve. set PIN_SF high for on, low for off

#define PIN_ON A1  // orbit valve. pulse PIN_ON for on, pulse PIN_OFF for off
#define PIN_OFF A2

int orbit_status = -1;
int sf_status = -1;

void setup() {
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A0, LOW);
  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);

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
#if SERIAL
    Serial.println(data);
#endif
    // receive buf hasn't been clobbered by reply yet
    if (strncmp("GET /orbit/on", data, 13) == 0)
      orbit_on();
    else if (strncmp("GET /orbit/off", data, 14) == 0)
      orbit_off();
    else if (strncmp("GET /sf/on", data, 10) == 0)
      sf_on();
    else if (strncmp("GET /sf/off", data, 11) == 0)
      sf_off();

    bfill.emit_p(PSTR("\norbit: "));
    emit_status(orbit_status, bfill);

    bfill.emit_p(PSTR("\nsf: "));
    emit_status(sf_status, bfill);

    ether.httpServerReply(bfill.position()); // send web page data
  }
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
  pulse(PIN_ON);
  orbit_status = 1;
}

void orbit_off() {
  pulse(PIN_OFF);
  orbit_status = 0;
}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}

void sf_on() {
  digitalWrite(PIN_SF, HIGH);
  sf_status = 1;
}

void sf_off() {
  digitalWrite(PIN_SF, LOW);
  sf_status = 0;
}

