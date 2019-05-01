// ----------------------------------------------------------------------------
// A&O MECANUMROVER
// Rover-2 Odometer
// https://bitbucket.org/ospringauf/mecanumrover
// ----------------------------------------------------------------------------

#include "ADNS5020.h"
#include "Arduino.h"
#include <Print.h>

// ADNS5020 timings (microseconds)
#define T_PD          50000 // from power down to valid motion
#define T_WAKEUP      55000
#define T_SWW         30 // 30us between write commands
#define T_SWR         20 // 20us between write and read
#define T_SRX         1 // 500ns between read and next command
#define T_SRAD        4 // 4us between address and data
#define T_BEXIT       1 // 250ns min NCS(high) after burst
#define T_NCS_SCLK    1 // 120ns after enable NSC(low) 
#define T_SCLK_NSC_R  1 // 120ns after read before disable NCS(high)
#define T_SCLK_NSC_W  20 // 20us after write before disable
#define T_DLY_SDIO    1 // 120ns SDIO delay after SCLK
#define T_NCS_SDIO    1 // 500ns after disble to SDIO floating
#define T_HOLD        1 // 500ns
#define T_SETUP       1 // 120ns


ADNS5020::ADNS5020(uint8_t sclk, uint8_t sdio, uint8_t ncs, uint8_t nreset, int cpi) {
  _sclk = sclk;
  _sdio = sdio;
  _ncs = ncs;
  _nreset = nreset;
  _cpi = cpi;
  factor = 1;
  x = 0;
  y = 0;

  pinMode(_sclk, OUTPUT);
  pinMode(_sdio, INPUT);
  pinMode(_ncs, OUTPUT);
  pinMode(_nreset, OUTPUT);

  digitalWrite(_nreset, HIGH);

  disable();

  // delay(100);
  // reset(); // apply resolution
  resolution(cpi);
}

// formatted output for "mousecam" capture
void ADNS5020::mousecamOutput() {
  enable();
  readDelta();
  Serial.print("DELTA:");
  Serial.print(dx, DEC);
  Serial.print(" ");
  Serial.println(dy, DEC);

  readFrame();
  disable();

  Serial.print("FRAME:");
  for (int i = 0; i < ADNS5020_FRAME_LENGTH; i++) {
    uint8_t pix = frame[i];
    if ( pix < 0x10 ) Serial.print("0");
    Serial.print(pix, HEX);
  }
  Serial.println();
}


void ADNS5020::printDelta() {
  // Serial.print(motion, BIN);
  Serial.print(" SQUAL:");  Serial.print(squal, DEC);
  Serial.print(" DELTA:"); Serial.print(dx, DEC);
  Serial.print(" ");       Serial.print(dy, DEC);
  Serial.println();
}

void ADNS5020::printShort() {
  
  Serial.print(dx, DEC);
  Serial.print(",");
  Serial.print(dy, DEC);
  Serial.print(",");
  Serial.print(squal, DEC);
}


void ADNS5020::printAll() {
  Serial.print(" DX:");  printd3(dx);
  Serial.print(" DY:");  printd3(dy);
  Serial.print(" SQ:");  printd3(squal);
  Serial.print(" SU:");  printd3(shutter_upper);
  Serial.print(" SL:");  printd3(shutter_lower);
  Serial.print(" MP:");  printd3(max_pixel);
  Serial.print(" PS:");  printd3(pixel_sum);
  Serial.println();
}

void ADNS5020::printd3(int i) {
  int n = abs(i);
  if (i>=0) Serial.print(" ");
  if (n<10) Serial.print("  ");
  else if (n<100) Serial.print(" ");
  Serial.print(i);
}


/**
 * rotation + scaling
 */
float ADNS5020::transformDx() 
{
  return ((float)dx * fcos) - ((float)dy * fsin);
}

float ADNS5020::transformDy() 
{
  return ((float)dx * fsin) + ((float)dy * fcos);
}

void ADNS5020::readDelta()
{
  enable();
  motion = readRegister(ADNS5020_REG_MOTION); // Freezes DX and DY until they are read or MOTION is read again.
  dx = factor * readRegister(ADNS5020_REG_DELTA_X);
  dy = factor * readRegister(ADNS5020_REG_DELTA_Y);
  squal = readRegister(ADNS5020_REG_SQUAL);
  updatePosition();
  disable();
}


void ADNS5020::readBurst() {
  enable();
  motion = readRegister(ADNS5020_REG_MOTION); // Freezes DX and DY until they are read or MOTION is read again.
  if (motion != 0) {
    pushbyte(ADNS5020_REG_BURST_MODE);
    delayMicroseconds(4); // tSRAD= 4us min.

    dx = factor * pullbyte();
    dy = factor * pullbyte();
    squal = pullbyte();
    shutter_upper = pullbyte();
    shutter_lower = pullbyte();
    max_pixel = pullbyte();
    pixel_sum = pullbyte();

    updatePosition();
  } else {
    dx = dy = squal = 0;
  }
  disable();
  delayMicroseconds(1); // tBEXIT= 250ns min.
}


void ADNS5020::updatePosition() {
  if (motion != 0) {
    x += dx;
    y += dy;
  }
}


void ADNS5020::readFrame() {
  writeRegister(ADNS5020_REG_PIXEL_GRAB, 1);
  int count = 0;
  do {
    uint8_t data = readRegister(ADNS5020_REG_PIXEL_GRAB);
    //if( (data & 0x80) == 0 ) // Data is valid
    {
      frame[count++] = data & 0x7f;
    }
  }
  while (count != ADNS5020_FRAME_LENGTH);
}


void ADNS5020::identify() {
  enable();
  uint8_t productId = readRegister(ADNS5020_REG_PRODUCT_ID);
  uint8_t revisionId = readRegister(ADNS5020_REG_REVISION_ID);
  Serial.print("ADNS-5020 prodId=");
  Serial.print(productId, HEX);
  Serial.print(", rev=");
  Serial.print(revisionId, HEX);
  Serial.println(productId == 0x12 ? ", OK" : ", unknown device");
  disable();
}


void ADNS5020::reset() {
  enable();

  // Initiate chip reset
  if (_nreset < 0)
    softReset();
  else
    hardReset();

  // Set resolution 
  resolution(_cpi);
  disable();
}


/**
 * set CHIP SELECT
 */
void ADNS5020::enable() {
  if (_ncs >= 0) {
    digitalWrite(_ncs, LOW);    
    delayMicroseconds(T_NCS_SCLK);
  }
  if (!_powered) powerUp();
}


/**
 * clear CHIP SELECT
 * SDIO is in high-Z (floating) when disabled (NCS=high)
 */
void ADNS5020::disable() {
  if (_ncs >= 0) {
    digitalWrite(_ncs, HIGH);
    delayMicroseconds(T_SCLK_NSC_R);
    delayMicroseconds(T_NCS_SDIO);
  }
}

void ADNS5020::softReset() {
  writeRegister(ADNS5020_REG_CHIP_RESET, 0x5a);
  delay(55); // t_WAKEUP=55ms
}


void ADNS5020::hardReset() {
  digitalWrite(_nreset, LOW);
  delayMicroseconds(T_PD); 
  digitalWrite(_nreset, HIGH);
  delayMicroseconds(T_WAKEUP); 
}

void ADNS5020::powerDown() {
  if (_powered) {
    enable();
    writeRegister(ADNS5020_REG_CONTROL, 0b00000010);
    disable();
  }
  _powered = false;
}

void ADNS5020::powerUp() {
  _powered = true;
  reset();
}


void ADNS5020::resolution(int cpi) {
  enable();
  _cpi = cpi;
  if (_cpi == 1000) {
    writeRegister(ADNS5020_REG_CONTROL, 0b00000001);
  }
  else {
    writeRegister(ADNS5020_REG_CONTROL, 0b00000000);
  }
  disable();
}


// int8_t ADNS5020::pullbyte() {
//   pinMode(_sdio, INPUT);

//   delayMicroseconds(ADNS5020_DELAY); // tHOLD = 100us min.

//   int8_t res = 0;
//   for (uint8_t i = 128; i > 0 ; i >>= 1) {
//     digitalWrite(_sclk, LOW);
//     res |= i * digitalRead(_sdio);
//     delayMicroseconds(100);
//     digitalWrite(_sclk, HIGH);
//   }

//   return res;
// }


byte ADNS5020::pullbyte() {
  pinMode(_sdio, INPUT);

  delayMicroseconds(ADNS5020_DELAY); // tHOLD = 100us min.

  byte res = 0;
  for (int i=0; i<8; ++i) {
    digitalWrite(_sclk, LOW);
    res += digitalRead(_sdio);
    res = res << 1;
    delayMicroseconds(ADNS5020_DELAY);
    digitalWrite(_sclk, HIGH);
  }

  return res;
}

void ADNS5020::pushbyte(uint8_t data) {
  pinMode (_sdio, OUTPUT);

  delayMicroseconds(ADNS5020_DELAY); // tHOLD = 100us min.

  for (uint8_t i = 128; i > 0 ; i >>= 1) {
    digitalWrite(_sclk, LOW);
    digitalWrite(_sdio, (data & i) != 0 ? HIGH : LOW);
    delayMicroseconds(ADNS5020_DELAY);
    digitalWrite(_sclk, HIGH);
  }
}


uint8_t ADNS5020::readRegister(uint8_t address) {
  address &= 0x7F; // MSB indicates read mode: 0
  pushbyte(address);
  uint8_t data = pullbyte();
  return data;
}


void ADNS5020::writeRegister(uint8_t address, uint8_t data) {
  address |= 0x80; // MSB indicates write mode: 1

  pushbyte(address);
  delayMicroseconds(ADNS5020_DELAY);

  pushbyte(data);
  delayMicroseconds(ADNS5020_DELAY); // tSWW, tSWR = 100us min.
}
