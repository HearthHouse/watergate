#include <EtherCard.h>

static byte myip[] = { 192,168,1,95 };
static byte gwip[] = { 192,168,1,1 };
static byte mymac[] = { 0xDE,0xAD,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer
static BufferFiller bfill;  // used as cursor while filling the buffer

#define PIN_SF A0 // sparkfun valve. set PIN_SF high for on, low for off

#define PIN_ON A1  // orbit valve. pulse PIN_ON for on, pulse PIN_OFF for off
#define PIN_OFF A2

#define BUTTON_ORBIT 1
#define BUTTON_SF 2

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
  pinMode(BUTTON_SF, OUTPUT);
  pinMode(BUTTON_ORBIT, OUTPUT);
  digitalWrite(BUTTON_ORBIT, HIGH);
  digitalWrite(BUTTON_SF, HIGH);

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
          "<style> button a { font-size: 30px; }</style>"
          "<body>"
          ), okHeader);
      bfill.emit_p(PSTR("<br>orbit: "));
      emit_status(orbit_status, bfill);
      bfill.emit_p(PSTR("<br><button><a href=/orbit/on>Orbit on</a></button>"
                        "<button><a href=/orbit/off>Orbit off</a></button>"));

      bfill.emit_p(PSTR("<br><br>sf: "));
      emit_status(sf_status, bfill);
      bfill.emit_p(PSTR("<br><button><a href=/sf/on>SF on</a></button>"
                        "<button><a href=/sf/off>SF off</a></button>"
                        "</body>"));
      ether.httpServerReply(bfill.position()); // send web page data
      return;
    }

    bfill.emit_p(PSTR("$F\r\n"), redirectHeader);
    ether.httpServerReply(bfill.position()); // redirect to /
  }

  if (digitalRead(BUTTON_ORBIT) == 0) {
    Serial.println("Orbit button");
    if (orbit_status) {
      orbit_off();
    } else {
      orbit_on();
    }
  }

  if (digitalRead(BUTTON_SF) == 0) {
    Serial.println("SF button");
    if (sf_status) {
      sf_off();
    } else {
      sf_on();
    }
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
