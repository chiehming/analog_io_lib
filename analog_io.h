/* analog_io_lib for Energia and the MSP430 Sensor Node Hardware
 * Forked from nRF24L01+ I/O for Energia
 *
 * Copyright (c) 2013 Eric Brundick < spirilis [at] linux  dot com >
 * Copyright (c) 2015 Luke Beno     < lgbeno   [at] analog dot io  >
 *
 *  Permission is hereby granted, free of charge, to any person 
 *  obtaining a copy of this software and associated documentation 
 *  files (the "Software"), to deal in the Software without 
 *  restriction, including without limitation the rights to use, copy, 
 *  modify, merge, publish, distribute, sublicense, and/or sell copies 
 *  of the Software, and to permit persons to whom the Software is 
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be 
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 *  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 *  DEALINGS IN THE SOFTWARE.
 */

#ifndef _ANALOG_IO_H
#define _ANALOG_IO_H

#define ANALOG_IO_LIBRARY_VERSION "1.0"

#include <Arduino.h>
#include <Print.h>
#include <SPI.h>
#include <stdint.h>
#include "nRF24L01.h"

#if defined(TARGET_IS_SNOWFLAKE_RA0) || defined(TARGET_IS_SNOWFLAKE_RA1) || defined(TARGET_IS_BLIZZARD_RB1)
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/pin_map.h>
#include <driverlib/gpio.h>
#endif

#ifndef BITF
  #define BIT0                (0x0001)
  #define BIT1                (0x0002)
  #define BIT2                (0x0004)
  #define BIT3                (0x0008)
  #define BIT4                (0x0010)
  #define BIT5                (0x0020)
  #define BIT6                (0x0040)
  #define BIT7                (0x0080)
  #define BIT8                (0x0100)
  #define BIT9                (0x0200)
  #define BITA                (0x0400)
  #define BITB                (0x0800)
  #define BITC                (0x1000)
  #define BITD                (0x2000)
  #define BITE                (0x4000)
  #define BITF                (0x8000)
#endif

 #define BLE_MODE         0x00
 #define PROPRIETARY_MODE 0x01
 #define UNDEFINED_MODE   0xFF

/* Constants for speed, radio state */
#define ENRF24_STATE_NOTPRESENT 0
#define ENRF24_STATE_DEEPSLEEP 1
#define ENRF24_STATE_IDLE 2
#define ENRF24_STATE_PTX 3
#define ENRF24_STATE_PRX 4

/* Internal IRQ handling */
#define ENRF24_IRQ_TX       0x20
#define ENRF24_IRQ_RX       0x40
#define ENRF24_IRQ_TXFAILED 0x10
#define ENRF24_IRQ_MASK     0x70

#define ENRF24_CFGMASK_IRQ 0


/* Class definition--inherits from Print so we have .print() functions */
class analog_io : public Print {
  public:
    boolean lastTXfailed;

    analog_io(uint8_t cePin, uint8_t csnPin, uint8_t irqPin);
    void begin(uint32_t datarate=1000000, uint8_t channel=0);  // Specify bitrate & channel
    void end();      // Shut it off, clear the library's state

    // I/O
    boolean available(boolean checkIrq=false);  // Check if incoming data is ready to be read
    size_t read(void *inbuf, uint8_t maxlen=32);  /* Read contents of RX buffer up to
                                                   * 'maxlen' bytes, return final length.
                                                   * 'inbuf' should be maxlen+1 since a
                                                   * null '\0' is tacked onto the end.
                                                   */
    virtual size_t write(uint8_t);  // Single-byte write, implements TX ring-buffer & auto-send
    virtual size_t write(const void *buf, size_t len) { return write((const uint8_t *)buf, len); };
    using Print::write;  // Includes the multi-byte write for repeatedly hitting write(uint8_t)
    void flush();    // Force transmission of TX ring buffer contents
    void purge();    // Ignore TX ring buffer contents, return ring pointer to 0.

    // Power-state related stuff-
    uint8_t radioState();  // Evaluate current state of the transceiver (see ENRF24_STATE_* defines)
    void deepsleep();  // Enter POWERDOWN mode, ~0.9uA power consumption
    void enableRX();   // Enter PRX mode (~14mA)
    void disableRX();  /* Disable PRX mode (PRIM_RX bit in CONFIG register)
                        * Note this won't necessarily push the transceiver into deep sleep, but rather
                        * an idle standby mode where its internal oscillators are ready & running but
                        * the RF transceiver PLL is disabled.  ~26uA power consumption.
                        */

    // Custom tweaks to RF parameters, packet parameters
    void autoAck(boolean onoff=true);  // Enable/disable auto-acknowledgements (enabled by default)
    void setChannel(uint8_t channel);
    void setTXpower(int8_t dBm=0);  // Only a few values supported by this (0, -6, -12, -18 dBm)
    void setSpeed(uint32_t rfspeed);  // Set 250000, 1000000, 2000000 speeds.
    void setCRC(boolean onoff, boolean crc16bit=false); /* Enable/disable CRC usage inside nRF24's
                                                         * hardware packet engine, specify 8 or
                                                         * 16-bit CRC.
                                                         */
    // Set AutoACK retry count, timeout params (0-15, 250-4000 respectively)
    void setAutoAckParams(uint8_t autoretry_count=15, uint16_t autoretry_timeout=2000);

    // Protocol addressing -- receive, transmit addresses
    void setAddressLength(size_t len);  // Valid parameters = 3, 4 or 5.  Defaults to 5.
    void setRXaddress(const void *rxaddr);    // 3-5 byte RX address loaded into pipe#1
    void setTXaddress(const void *txaddr);    // 3-5 byte TX address loaded into TXaddr register
    
    // Miscellaneous feature
    boolean rfSignalDetected();  /* Read RPD register to determine if transceiver has presently detected an RF signal
                                  * of -64dBm or greater.  Only works in PRX (enableRX()) mode.
                                  */

    // Query current parameters
    int getChannel(void) { return _readReg(RF24_RF_CH); };
    uint32_t getSpeed(void);
    int8_t getTXpower(void);
    size_t getAddressLength(void) { return rf_addr_width; };
    void getRXaddress(void *);
    void getTXaddress(void *);
    boolean getAutoAck(void);
    unsigned int getCRC(void);
    void bleTransmit(char* name, uint8_t* payload=0x00, uint8_t payload_len=1);
    void bleSingleTransmit(char* name, uint8_t* payload=0x00, uint8_t payload_len=1);
    void bleStart();
    void bleEnd(void);
    void flushBle();

  private:
    uint8_t rf_status;
    uint8_t rf_addr_width;
    uint8_t txbuf_len;
    uint8_t txbuf[32];
    uint8_t txaddr_bak[5];
    uint8_t buf[32];
    uint8_t lastirq, readpending;
    uint8_t _cePin, _csnPin, _irqPin;
    uint8_t backup_regs[11];
    uint8_t backup[12];
    uint8_t chRf[3];
    uint8_t chLe[3];
    uint8_t i, L, ch;
    uint8_t mode;


    void _bleTx(char* name, uint8_t name_len, uint8_t* payload, uint8_t payload_len);
    uint8_t _readReg(uint8_t addr);
    void _readRegMultiLSB(uint8_t addr, uint8_t *buf, size_t len);
    void _writeReg(uint8_t addr, uint8_t val);
    void _writeRegMultiLSB(uint8_t addr, uint8_t *buf, size_t len);
    void _issueCmd(uint8_t cmd);
    void _readCmdPayload(uint8_t addr, uint8_t *buf, size_t len, size_t maxlen);
    void _issueCmdPayload(uint8_t cmd, uint8_t *buf, size_t len);
    uint8_t _irq_getreason();
    uint8_t _irq_derivereason();  // Get IRQ status from rf_status w/o querying module over SPI.
    void _irq_clear(uint8_t irq);
    boolean _isAlive();
    void _readTXaddr(uint8_t *buf);
    void _writeRXaddrP0(uint8_t *buf);
    void _maintenanceHook();  // Handles IRQs and purges RX queue when erroneous contents exist.
    void _btLePacketEncode(uint8_t* packet, uint8_t len, uint8_t chan);
    void _btLeWhiten(uint8_t* data, uint8_t len, uint8_t whitenCoeff);
    uint8_t _btLeWhitenStart(uint8_t chan);
    uint8_t  _swapbits(uint8_t a);
    void _btLeCrc(const uint8_t* data, uint8_t len, uint8_t* dst);
    void _backupRegisters(void);
    void _restoreRegisters(void);
    void _configBle(void);

/* Private planning:
   Need to keep track of:
	RF status (since we get it after every SPI communication, might as well store it)
	RF channel (to refresh in order to reset PLOS_CNT)
	RF speed (to determine if autoAck makes any sense, since it doesn't @ 250Kbps)
	Address Length (to determine how many bytes to read out of set(RX|TX)address())
	32-byte TX ring buffer
	Ring buffer position
 */
};

#endif
