// ----------------------------------------------------------------------------
// A&O MECANUMROVER
// Rover-2 Odometer
// https://bitbucket.org/ospringauf/mecanumrover
// ----------------------------------------------------------------------------

#ifndef __ADNS5020_H__
#define __ADNS5020_H__

#include "Arduino.h"

#define ADNS5020_REG_PRODUCT_ID     0x00
#define ADNS5020_REG_REVISION_ID    0x01
#define ADNS5020_REG_MOTION         0x02
#define ADNS5020_REG_DELTA_X        0x03
#define ADNS5020_REG_DELTA_Y        0x04
#define ADNS5020_REG_SQUAL          0x05
#define ADNS5020_REG_BURST_MODE     0x63
#define ADNS5020_REG_CONTROL        0x0d
#define ADNS5020_REG_PIXEL_GRAB     0x0b
#define ADNS5020_REG_CHIP_RESET     0x3a

#define ADNS5020_FRAME_LENGTH       225
#define ADNS5020_DELAY              10

// Avago ADNS-5020-EN optical mouse sensor
// see http://strofoland.com/arduino-projects/reading-a5020-optical-sensor-using-arduino-part2/
// see https://www.bidouille.org/hack/mousecam

class ADNS5020 {
  public:
    ADNS5020(uint8_t sclk, uint8_t sdio, uint8_t ncs, uint8_t nreset, int cpi);

    // uint8_t motion;
    // int dx;
    // int dy;
    // uint8_t squal;
    // uint8_t shutter_upper;
    // uint8_t shutter_lower;
    // uint8_t max_pixel;
    // uint8_t pixel_sum;
    // uint8_t frame[ADNS5020_FRAME_LENGTH];
    
    byte motion; // motion flag is in MSB(!)
    int8_t dx;
    int8_t dy;
    byte squal;
    byte shutter_upper;
    byte shutter_lower;
    byte max_pixel;
    byte pixel_sum;
    byte frame[ADNS5020_FRAME_LENGTH];


    int factor;
    int x;
    int y;

    // rotation/scaling param (used in Odometer)
    float fcos = 1;
    float fsin = 0;

    void reset();    
    void softReset();
    void hardReset();
    void powerUp();
    void powerDown();

    void setTransform(float angle, float scale) {
      fcos = scale * cos(angle);
      fsin = scale * sin(angle);
    }
    float transformDx();
    float transformDy();

    void resolution(int cpi);
    void identify();
    void readDelta();
    void readBurst();
    void readFrame();
    void mousecamOutput();
    void printDelta();
    void printAll();
    void printShort();

    
  private:   
    uint8_t _sclk;
    uint8_t _sdio;
    uint8_t _ncs;
    uint8_t _nreset;
    int _cpi;

    bool _powered = true;

    void enable();  // NCS=high
    void disable(); // NCS=low

    void setDelta(int8_t rx, int8_t ry) {
      dx = rx;
      dy = ry;
    }

    void pushbyte(uint8_t data);
    byte pullbyte();
    uint8_t readRegister(uint8_t address);
    void writeRegister(uint8_t address, uint8_t data);
    void printd3(int i);
    void updatePosition();
  
};

#endif  // __ADNS5020_H__
