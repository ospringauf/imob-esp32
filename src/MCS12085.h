// ----------------------------------------------------------------------------
// IMOB VEHICLE
// Odometer, based on MCS-12085 optical mosue sensor
// ----------------------------------------------------------------------------

#ifndef __MCS12085_H__
#define __MCS12085_H__

#include "Arduino.h"


// MCS-12085 optical mouse sensor
// see https://github.com/jgrahamc/mcs12085
// see http://rogerrowland.blogspot.com/2014/06/hacking-optical-mouse.html
// datasheet http://www.rmrsystems.co.uk/download/MCS12085.pdf

class MCS12085 {
  public:
    MCS12085(uint8_t sck, uint8_t sdio);

    void init();    
    int read_x();
    int read_y();
    
  private:   
    uint8_t _sck;
    uint8_t _sdio;

    void tick();
    void tock();
    int read_bit();
    byte read_byte();
    void write_bit(byte b);
    void write_byte(byte w);
    void wr_pause();
    void rw_pause();
    int convert(byte);

  
};

#endif  // __MCS12085_H__
