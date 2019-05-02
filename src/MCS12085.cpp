// ----------------------------------------------------------------------------
// IMOB VEHICLE
// Odometer, based on MCS-12085 optical mosue sensor
// ----------------------------------------------------------------------------

#include "MCS12085.h"
#include "Arduino.h"
// #include <Print.h>

// The time of a clock pulse. This will be used twice to make the clock
// signal: cycle us low and then cycle us high
#define MCS12085_CYCLE 25

MCS12085::MCS12085(uint8_t sck, uint8_t sdio) {
  _sck = sck;
  _sdio = sdio;
}


// set up the pins for the clock and data
void MCS12085::init()
{
  // When not being clocked the clock pin needs to be high
  pinMode(_sck, OUTPUT);
  digitalWrite(_sck, HIGH);

  pinMode(_sdio, OUTPUT);
  digitalWrite(_sdio, LOW);
}

// perform a single clock tick of 25us low
void MCS12085::tick()
{
  digitalWrite(_sck, LOW);
  delayMicroseconds(MCS12085_CYCLE);
  digitalWrite(_sck, HIGH);
}

// finish the clock pulse by waiting during the high period
void MCS12085::tock()
{
  delayMicroseconds(MCS12085_CYCLE);
}

// read a single bit from the chip by creating a clock
// pulse and reading the value returned
int MCS12085::read_bit()
{
  tick();

  int r = (digitalRead(_sdio) == HIGH);

  tock();
  return r;
}

// Reads 8 bits from the sensor MSB first
byte MCS12085::read_byte()
{
  int bits = 8;
  byte value = 0x80;
  byte b = 0;

  while (bits > 0) {
    if ( read_bit() ) {
      b |= value;
    }

    value >>= 1;
    --bits;
  }

  pinMode(_sdio, OUTPUT);
  digitalWrite(_sdio, LOW);

  return b;
}

// write a single bit to the chip by creating a clock
// pulse and writing the value
void MCS12085::write_bit(byte b)    // 1 or 0
{
  // Set the data pin value ready for the write and then clock

  if ( b ) {
    digitalWrite(_sdio, HIGH);
  } else {
    digitalWrite(_sdio, LOW);
  }

  tick();
  tock();
  digitalWrite(_sdio, LOW);
}

// write a byte to the sensor MSB first
void MCS12085::write_byte(byte w)      // Number to get the bits from
{
  int bits = 8;

  while ( bits > 0 ) {
    write_bit(w & 0x80);
    w <<= 1;
    --bits;
  }

  pinMode(_sdio, INPUT);
}

// pause between a write and a read to the sensor
void MCS12085::wr_pause()
{
  delayMicroseconds(100);
}

// pause between a read and a write to the sensor
void MCS12085::rw_pause()
{
  delayMicroseconds(250);
}

// converts a byte into a signed 8-bit int
int MCS12085::convert(byte b)
{
  if ( b < 128 ) {
     return int(b);
  } else {
     return -(int(b ^ 0xFF) + 1);
  }
}

// read the change in X position since last read
int MCS12085::read_x()
{
  write_byte(0x02); // 0x02 = Read DX Register
  wr_pause();
  int i = convert(read_byte());
  rw_pause();
  return i;
}

// read the change in Y position since last read
int MCS12085::read_y()
{
  write_byte(0x03); // 0x03 = Read DY Register
  wr_pause();
  int i = convert(read_byte());
  rw_pause();
  return i;
}




