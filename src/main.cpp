#include <Arduino.h>
#include <SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>
#include "MCS12085.h"
#include <WiFi.h>


// RFID with MFRC-522
// see https://github.com/miguelbalboa/rfid/issues/359
// https://techtutorialsx.com/2017/11/01/esp32-rfid-printing-the-mfrc522-firmware-version/

// #define LEDPIN 25

#define LORA_SCK     5    
#define LORA_MISO    19   
#define LORA_MOSI    27 
#define LORA_SS      18  
#define LORA_RST     14   
#define LORA_DI0     26  
#define LORA_BAND    868E6


#define RFID_SDA 5 
#define RFID_SCK 18 
#define RFID_MOSI 23
#define RFID_MISO 19
#define RFID_RST 22

#define OLED_I2C_ADDR 0x3C
#define OLED_RESET 16
#define OLED_SDA 4
#define OLED_SCL 15

#define MOUSE_SCLK 17
#define MOUSE_SDIO 13
// #define MOUSE_NCS 25
// #define MOUSE_NRST -1 

#define SPI_NONE -1
#define SPI_RFID 0
#define SPI_LORA 1

// some colored RFID location tags
#define LOC_START   0x0
// #define LOC_YELLOW  0xBC325E03
// #define LOC_RED     0x224016D0
// #define LOC_GREEN   0xACA2Ce03
// #define LOC_BLUE    0X7ED6B912
// #define LOC_GRAY    0xEC05D503
// #define LOC_BLACK   0x1CD9CE03  
#define LOC_YELLOW  0x4c645b03
#define LOC_RED     0x823e77d0
#define LOC_GREEN   0xec85ce03
#define LOC_BLUE    0Xce04ba12
#define LOC_GRAY    0x2c31d403
#define LOC_BLACK   0x5c34ca03  

#define NUM_TAGS 6

SSD1306 display(OLED_I2C_ADDR, OLED_SDA, OLED_SCL);

// mouse sensor
MCS12085 mouse(MOUSE_SCLK, MOUSE_SDIO);

// rfid
MFRC522 mfrc522(RFID_SDA, RFID_RST); 


long distance;
ulong location = LOC_START; // 32bit RFID UID of last seen tag
ulong destination = LOC_START;

int current_spi = SPI_NONE; 
char buffer[80];
bool info_update = false; // display update flag

const char *vehicle_id = "IMOB-A";
const char *color[] = { "START", "YELLOW", "RED", "GREEN", "BLUE", "GRAY", "BLACK" };
const ulong tags[] = { LOC_START, LOC_YELLOW, LOC_RED, LOC_GREEN, LOC_BLUE, LOC_GRAY, LOC_BLACK };

wl_status_t wifi_status = WL_DISCONNECTED;


const char* uid_to_color(ulong uid) 
{
  for (int i=0; i<=NUM_TAGS; ++i)
    if (uid == tags[i])
      return color[i];
  return itoa(uid, buffer, 16);
}

const ulong color_to_uid(const char* col) 
{
  for (int i=0; i<=NUM_TAGS; ++i)
    if (strcmp(col, color[i]) == 0)
      return tags[i];
  return 0;
}

// check if we have arrived at destination
// set a new (random) destination
void check_location() 
{  
  if (location == destination) {
    ulong new_dest = destination;
    while (new_dest == destination) 
      new_dest = tags[1 + random(NUM_TAGS)];
    destination = new_dest;
    distance = 0;
    info_update = true;

    // Serial.print("new destination: "); Serial.println(uid_to_color(destination));
  }
}


// switch SPI config on-the-fly
void spi_select(int which) {
     if (which == current_spi) return;
     SPI.end();
     
     switch(which) {
        case SPI_RFID:
          SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI);
          mfrc522.PCD_Init();   
        break;
        case SPI_LORA:
          SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
          // LoRa.setPins(LORA_SS,LORA_RST,LORA_DI0);
        break;
     }

     current_spi = which;
}

// convert 4-byte array to 32bit long
long uid_to_long(byte array[]) 
{
  long l = array[0];
  for (int i=1; i<4; ++i) {
    l = l<<8;
    l |= array[i];
  }
  return l;
}

// update display
void info() {
  display.clear();
  int y = 0;

  display.setFont(ArialMT_Plain_10);
  display.drawString(0, y, vehicle_id);
  //display.drawString(100, 0, itoa(wifi_status, buffer, 10));
  display.drawString(43, y, WiFi.localIP().toString());
  // display.drawLine(0,11, 128,11);

  y = 43;
  // display.setFont(ArialMT_Plain_16);
  display.drawString(0, y, "@");
  // display.drawString(20, y, location_uid);
  //display.drawString(20, y, itoa(location, buffer, 16));
  display.drawString(20, y, uid_to_color(location));

  // display.setFont(ArialMT_Plain_10);
  long mm = distance / 20; // travelled distence in mm
  display.drawString(90, y, itoa(mm, buffer, 10)); 

  y = 53;
  display.drawString(0, y, ">>");
  display.drawString(20, y, uid_to_color(destination));
  display.display();
}


void wifi_connect() {
  wifi_status = WiFi.begin("ssid", "pwd");
  info();

  int retry = 20;
  while ((wifi_status = WiFi.status()) != WL_CONNECTED && retry > 0)
  {
    Serial.print(".");
    info();
    retry--;
    delay(500);
  }

  if (retry == 0)
  {
    Serial.println("WiFi connection failed");
  } else {
    Serial.println("WiFi connected"); 
  }
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  info();
}


void setup()
{
  Serial.begin(115200);
  delay(100);

  // reset the OLED
  pinMode(OLED_RESET, OUTPUT);
  digitalWrite(OLED_RESET, LOW);
  delay(50);
  digitalWrite(OLED_RESET, HIGH);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "START");

  mouse.init();
  delay(100);

  // SPI.begin();                       // Init SPI bus
  spi_select(SPI_RFID);
  delay(30);
  mfrc522.PCD_Init();

  // retry?
  if (mfrc522.PCD_ReadRegister(mfrc522.VersionReg) == 0) {
    delay(100);
    mfrc522.PCD_Init();
  }
  mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details
  display.drawString(100, 0, itoa(mfrc522.PCD_ReadRegister(mfrc522.VersionReg),buffer, 16));
  // Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  display.display();

  check_location();

  // wifi_connect();
}




long last_mouse = 0;
long last_info = 0;

void loop()
{
  long now = millis();

  // mouse position update
  if (now - last_mouse > 30) {
    last_mouse = now;

    // calling these in quick succession seems to work faster
    int x = mouse.read_x(); // distance moved in dots (-127 to 128)
    int y = mouse.read_y(); // distance moved in dots (-127 to 128)
    double delta = sqrt(x*x + y*y);
    distance += delta;
    info_update |= (delta > 0);  
  }


  spi_select(SPI_RFID);
  
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent())
  if (mfrc522.PICC_ReadCardSerial())
  {
    // Dump debug info about the card; PICC_HaltA() is automatically called
    // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
    // mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
    mfrc522.PICC_HaltA();
    location = uid_to_long(mfrc522.uid.uidByte);
    check_location();
    info_update = true;
  }


  // display update 5x/sec
  if ((now - last_info > 200) && info_update) {
    last_info = now;
    info_update = false;
    info();
  }

  // delay(50);

}