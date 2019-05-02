#include <Arduino.h>
#include <SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>
// #include "ADNS5020.h"
#include "MCS12085.h"


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
// #define LORA_BAND    868E6


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

SSD1306 display(OLED_I2C_ADDR, OLED_SDA, OLED_SCL);

// Avago ADNS-5020-EN mouse sensor (optical flow), bit-banged "SPI" 
// param: SCLK, SDIO, NCS, NRESET, CPI
// ADNS5020 mouse(MOUSE_SCLK, MOUSE_SDIO, MOUSE_NCS, MOUSE_NRST, 500);
MCS12085 mouse(MOUSE_SCLK, MOUSE_SDIO);

MFRC522 mfrc522(RFID_SDA, RFID_RST);  // Create MFRC522 instance


long distance;
char location_uid[40];

int current_spi = -1; // -1 - NOT STARTED   0 - RFID   1 - LORA
char buffer[80];

void spi_select(int which) {
     if (which == current_spi) return;
     SPI.end();
     
     switch(which) {
        case 0:
          SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI);
          mfrc522.PCD_Init();   
        break;
        case 1:
          SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
          // LoRa.setPins(LORA_SS,LORA_RST,LORA_DI0);
        break;
     }

     current_spi = which;
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}


void setup()
{
  strcpy(location_uid, "?");

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

  display.drawString(0, 0, "IMOB VEHICLE READY");
  display.display();

  // mouse.reset();
  // mouse.identify();
  mouse.init();
  delay(100);

  // SPI.begin();                       // Init SPI bus
  spi_select(0);
  mfrc522.PCD_Init();                // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}


void info() {
  display.clear();

  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "IMOB");
  display.drawLine(0,11, 128,11);

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 16, location_uid);
  display.setFont(ArialMT_Plain_10);
  display.drawString(90, 16, itoa(distance/10, buffer, 10));
  display.display();
}


long last_mouse = 0;
long last_info = 0;

void loop()
{
  long now = millis();

  if (now - last_mouse > 50) {
    last_mouse = now;

    // calling these in quick succession seems to work faster
    int x = mouse.read_x(); // distance moved in dots (-127 to 128)
    int y = mouse.read_y(); // distance moved in dots (-127 to 128)
    // Serial.print("dx=");
    // Serial.print(xVal);
    // Serial.print(", dy=");
    // Serial.println(yVal); 
    distance += sqrt(x*x + y*y);
    // Serial.println(distance);

    // mouse.readBurst();
    // if (mouse.motion != 0) {
    //   // mouse.printDelta();
    //   mouse.printAll();
    //   distance += sqrt(mouse.dx*mouse.dx + mouse.dy*mouse.dy);
      
    //   // Serial.println(distance);
    // }
  }


  if (now - last_info > 200) {
    last_info = now;
    info();
  }

  // delay(50);

  spi_select(0);
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  // mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  mfrc522.PICC_HaltA();

  array_to_string(mfrc522.uid.uidByte, 4, location_uid);
}