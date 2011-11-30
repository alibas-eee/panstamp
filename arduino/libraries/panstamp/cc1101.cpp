/**
 * Copyright (c) 2011 Daniel Berenguer <dberenguer@usapiens.com>
 * 
 * This file is part of the panStamp project.
 * 
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 * 
 * panLoader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with panLoader; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 
 * USA
 * 
 * Author: Daniel Berenguer
 * Creation date: 03/03/2011
 */

#include "WProgram.h"
#include "cc1101.h"
#include "nvolat.h"

/**
 * Macros
 */
// Wait until SPI MISO line goes low
#define wait_Miso()  while(bitRead(PORT_SPI_MISO, BIT_SPI_MISO))
// Get GDO0 pin state
#define getGDO0state()  bitRead(PORT_GDO0, BIT_GDO0)
// Wait until GDO0 line goes high
#define wait_GDO0_high()  while(!getGDO0state())
// Wait until GDO0 line goes low
#define wait_GDO0_low()  while(getGDO0state())
// Select (SPI) CC1101
#define cc1101_Select()  bitClear(PORT_SPI_SS, BIT_SPI_SS)
// Deselect (SPI) CC1101
#define cc1101_Deselect()  bitSet(PORT_SPI_SS, BIT_SPI_SS)
// Read CC1101 Config register
#define readConfigReg(regAddr)    readReg(regAddr, CC1101_CONFIG_REGISTER)
// Read CC1101 Status register
#define readStatusReg(regAddr)    readReg(regAddr, CC1101_STATUS_REGISTER)

 /**
  * PATABLE
  */
const byte paTable[8] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60};

/**
 * writeReg
 * 
 * Write single register into the CC1101 IC via SPI
 * 
 * 'regAddr'	Register address
 * 'value'	Value to be writen
 */
void CC1101::writeReg(byte regAddr, byte value) 
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(regAddr);                    // Send register address
  spi.send(value);                      // Send value
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * writeBurstReg
 * 
 * Write multiple registers into the CC1101 IC via SPI
 * 
 * 'regAddr'	Register address
 * 'buffer'	Data to be writen
 * 'len'	Data length
 */
void CC1101::writeBurstReg(byte regAddr, byte* buffer, byte len)
{
  byte addr, i;
  
  addr = regAddr | WRITE_BURST;         // Enable burst transfer
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(addr);                       // Send register address
  
  for(i=0 ; i<len ; i++)
    spi.send(buffer[i]);                // Send value

  cc1101_Deselect();                    // Deselect CC1101  
}

/**
 * cmdStrobe
 * 
 * Send command strobe to the CC1101 IC via SPI
 * 
 * 'cmd'	Command strobe
 */     
void CC1101::cmdStrobe(byte cmd) 
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(cmd);                        // Send strobe command
  cc1101_Deselect();                    // Deselect CC1101 
}

/**
 * readReg
 * 
 * Read CC1101 register via SPI
 * 
 * 'regAddr'	Register address
 * 'regType'	Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
 * 
 * Return:
 * 	Data byte returned by the CC1101 IC
 */
byte CC1101::readReg(byte regAddr, byte regType) 
{
  byte addr, val;

  addr = regAddr | regType;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(addr);                       // Send register address
  val = spi.send(0x00);                 // Read result
  cc1101_Deselect();                    // Deselect CC1101

  return val;
}

/**
 * readBurstReg
 * 
 * Read burst data from CC1101 via SPI
 * 
 * 'buffer'	Buffer where to copy the result to
 * 'regAddr'	Register address
 * 'len'	Data length
 */
void CC1101::readBurstReg(byte * buffer, byte regAddr, byte len) 
{
  byte addr, i;
  
  addr = regAddr | READ_BURST;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  spi.send(addr);                       // Send register address
  for(i=0 ; i<len ; i++)
    buffer[i] = spi.send(0x00);         // Read result byte by byte
  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * reset
 * 
 * Reset CC1101
 */
void CC1101::reset(void) 
{
  cc1101_Deselect();                    // Deselect CC1101
  delayMicroseconds(5);
  cc1101_Select();                      // Select CC1101
  delayMicroseconds(10);
  cc1101_Deselect();                    // Deselect CC1101
  delayMicroseconds(41);
  cc1101_Select();                      // Select CC1101

  wait_Miso();                          // Wait until MISO goes low
  spi.send(CC1101_SRES);                // Send reset command strobe
  wait_Miso();                          // Wait until MISO goes low

  cc1101_Deselect();                    // Deselect CC1101

  setDefaultRegs();                     // Reconfigure CC1101
  setRegsFromEeprom();                  // Take user settings from EEPROM
}

/**
 * setDefaultRegs
 * 
 * Configure CC1101 registers
 */
void CC1101::setDefaultRegs(void) 
{
  byte defSyncWrd[] = {CC1101_DEFVAL_SYNC1, CC1101_DEFVAL_SYNC0};

byte val = readReg(CC1101_SYNC1, CC1101_CONFIG_REGISTER);
Serial.println(val, HEX);

  writeReg(CC1101_IOCFG2,  CC1101_DEFVAL_IOCFG2);
  writeReg(CC1101_IOCFG1,  CC1101_DEFVAL_IOCFG1);
  writeReg(CC1101_IOCFG0,  CC1101_DEFVAL_IOCFG0);
  writeReg(CC1101_FIFOTHR,  CC1101_DEFVAL_FIFOTHR);
  writeReg(CC1101_PKTLEN,  CC1101_DEFVAL_PKTLEN);
  writeReg(CC1101_PKTCTRL1,  CC1101_DEFVAL_PKTCTRL1);
  writeReg(CC1101_PKTCTRL0,  CC1101_DEFVAL_PKTCTRL0);

  // Set default synchronization word
  setSyncWord(defSyncWrd, false);

  // Set default device address
  setDevAddress(CC1101_DEFVAL_ADDR, false);
  // Set default frequency channel
  setChannel(CC1101_DEFVAL_CHANNR, false);
  
  writeReg(CC1101_FSCTRL1,  CC1101_DEFVAL_FSCTRL1);
  writeReg(CC1101_FSCTRL0,  CC1101_DEFVAL_FSCTRL0);

  // Set default carrier frequency = 868 MHz
  carrierFreq = CFREQ_LAST;
  setCarrierFreq(CFREQ_868, false);

  writeReg(CC1101_MDMCFG4,  CC1101_DEFVAL_MDMCFG4);
  writeReg(CC1101_MDMCFG3,  CC1101_DEFVAL_MDMCFG3);
  writeReg(CC1101_MDMCFG2,  CC1101_DEFVAL_MDMCFG2);
  writeReg(CC1101_MDMCFG1,  CC1101_DEFVAL_MDMCFG1);
  writeReg(CC1101_MDMCFG0,  CC1101_DEFVAL_MDMCFG0);
  writeReg(CC1101_DEVIATN,  CC1101_DEFVAL_DEVIATN);
  writeReg(CC1101_MCSM2,  CC1101_DEFVAL_MCSM2);
  writeReg(CC1101_MCSM1,  CC1101_DEFVAL_MCSM1);
  writeReg(CC1101_MCSM0,  CC1101_DEFVAL_MCSM0);
  writeReg(CC1101_FOCCFG,  CC1101_DEFVAL_FOCCFG);
  writeReg(CC1101_BSCFG,  CC1101_DEFVAL_BSCFG);
  writeReg(CC1101_AGCCTRL2,  CC1101_DEFVAL_AGCCTRL2);
  writeReg(CC1101_AGCCTRL1,  CC1101_DEFVAL_AGCCTRL1);
  writeReg(CC1101_AGCCTRL0,  CC1101_DEFVAL_AGCCTRL0);
  writeReg(CC1101_WOREVT1,  CC1101_DEFVAL_WOREVT1);
  writeReg(CC1101_WOREVT0,  CC1101_DEFVAL_WOREVT0);
  writeReg(CC1101_WORCTRL,  CC1101_DEFVAL_WORCTRL);
  writeReg(CC1101_FREND1,  CC1101_DEFVAL_FREND1);
  writeReg(CC1101_FREND0,  CC1101_DEFVAL_FREND0);
  writeReg(CC1101_FSCAL3,  CC1101_DEFVAL_FSCAL3);
  writeReg(CC1101_FSCAL2,  CC1101_DEFVAL_FSCAL2);
  writeReg(CC1101_FSCAL1,  CC1101_DEFVAL_FSCAL1);
  writeReg(CC1101_FSCAL0,  CC1101_DEFVAL_FSCAL0);
  writeReg(CC1101_RCCTRL1,  CC1101_DEFVAL_RCCTRL1);
  writeReg(CC1101_RCCTRL0,  CC1101_DEFVAL_RCCTRL0);
  writeReg(CC1101_FSTEST,  CC1101_DEFVAL_FSTEST);
  writeReg(CC1101_PTEST,  CC1101_DEFVAL_PTEST);
  writeReg(CC1101_AGCTEST,  CC1101_DEFVAL_AGCTEST);
  writeReg(CC1101_TEST2,  CC1101_DEFVAL_TEST2);
  writeReg(CC1101_TEST1,  CC1101_DEFVAL_TEST1);
  writeReg(CC1101_TEST0,  CC1101_DEFVAL_TEST0);
}

/**
 * init
 * 
 * Initialize CC1101
 */
void CC1101::init(void) 
{
  spi.init();                           // Initialize SPI interface
  pinMode(GDO0, INPUT);                 // Config GDO0 as input

  reset();                              // Reset CC1101
//  setDefaultRegs();                     // Configure CC1101 registers
//  setRegsFromEeprom();                  // Take user settings from EEPROM

  // Configure PATABLE
  writeBurstReg(CC1101_PATABLE, (byte*)paTable, 8);
}

/**
 * setSyncWord
 * 
 * Set synchronization word
 * 
 * 'sync'	Synchronization word
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101::setSyncWord(byte *sync, bool save) 
{
  if ((syncWord[0] != sync[0]) || (syncWord[1] != sync[1]))
  {
    writeReg(CC1101_SYNC1, sync[0]);
    writeReg(CC1101_SYNC0, sync[1]);
    memcpy(syncWord, sync, sizeof(syncWord));
    // Save in EEPROM
    if (save)
    {
      EEPROM.write(EEPROM_SYNC_WORD, sync[0]);
      EEPROM.write(EEPROM_SYNC_WORD + 1, sync[1]);
    }
  }
}

/**
 * setDevAddress
 * 
 * Set device address
 * 
 * 'addr'	Device address
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101::setDevAddress(byte addr, bool save) 
{
  if (devAddress != addr)
  {
    writeReg(CC1101_ADDR, addr);
    devAddress = addr;
    // Save in EEPROM
    if (save)
      EEPROM.write(EEPROM_DEVICE_ADDR, addr);  
  }
}

/**
 * setChannel
 * 
 * Set frequency channel
 * 
 * 'chnl'	Frequency channel
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101::setChannel(byte chnl, bool save) 
{
  if (channel != chnl)
  {
    writeReg(CC1101_CHANNR,  chnl);
    channel = chnl;
    // Save in EEPROM
    if (save)
      EEPROM.write(EEPROM_FREQ_CHANNEL, chnl);
  }
}

/**
 * setCarrierFreq
 * 
 * Set carrier frequency
 * 
 * 'freq'	New carrier frequency
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101::setCarrierFreq(byte freq, bool save) 
{
  if (carrierFreq != freq)
  {
    switch(freq)
    {
      case CFREQ_915:
        writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_915);
        writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_915);
        writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_915);
        break;
      default:
        writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_868);
        writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_868);
        writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_868);
        break;
    }
    
    carrierFreq = freq;
    
    // Save in EEPROM
    if (save)
      EEPROM.write(EEPROM_CARRIER_FREQ, freq);
  }
}

/**
 * setRegsFromEeprom
 * 
 * Set registers from EEPROM
 */
void CC1101::setRegsFromEeprom(void)
{
  byte bVal;
  byte arrV[2];

  // Read Carrier Frequency from EEPROM
  bVal = EEPROM.read(EEPROM_CARRIER_FREQ);
  // Set carrier frequency
  if (bVal < CFREQ_LAST)
    setCarrierFreq(bVal, false);
  // Read RF channel from EEPROM
  bVal = EEPROM.read(EEPROM_FREQ_CHANNEL);
  // Set RF channel
  if (bVal < NUMBER_OF_FCHANNELS )
    setChannel(bVal, false);
  // Read Sync word from EEPROM
  arrV[0] = EEPROM.read(EEPROM_SYNC_WORD);
  arrV[1] = EEPROM.read(EEPROM_SYNC_WORD + 1);
  // Set Sync word
  if (((arrV[0] != 0x00) && (arrV[0] != 0xFF)) || ((arrV[1] != 0x00) && (arrV[1] != 0xFF)))
    setSyncWord(arrV, false);
  // Read device address from EEPROM
  bVal = EEPROM.read(EEPROM_DEVICE_ADDR);
  // Set device address
  if (bVal > 0)
    setDevAddress(bVal, false);
}

/**
 * setPowerDownState
 * 
 * Put CC1101 into power-down state
 */
void CC1101::setPowerDownState() 
{
  // Comming from RX state, we need to enter the IDLE state first
  cmdStrobe(CC1101_SIDLE);
  // Enter Power-down state
  cmdStrobe(CC1101_SPWD);
}

/**
 * sendData
 * 
 * Send data packet via RF
 * 
 * 'packet'	Packet to be transmitted
 *
 *  Return:
 *    True if the transmission succeeds
 *    False otherwise
 */
boolean CC1101::sendData(CCPACKET packet)
{
  // Enter RX state
  setRxState();

  // Check that the RX state has been entered
  while (readStatusReg(CC1101_MARCSTATE) != 0x0D)
    delay(1);
  delayMicroseconds(500);

  // Set data length at the first position of the TX FIFO
  writeReg(CC1101_TXFIFO,  packet.length);
  // Write data into the TX FIFO
  writeBurstReg(CC1101_TXFIFO, packet.data, packet.length);

  // CCA enabled: will enter TX state only if the channel is clear
  cmdStrobe(CC1101_STX);

  // Check that TX state is being entered (state = RXTX_SETTLING)
  if(readStatusReg(CC1101_MARCSTATE) != 0x15)
    return false;

  // Wait for the sync word to be transmitted
  wait_GDO0_high();

  // Wait until the end of the packet transmission
  wait_GDO0_low();

  // Flush TX FIFO. Don't uncomment
  // cmdStrobe(CC1101_SFTX);
  
  // Enter back into RX state
  setRxState();

  // Check that the TX FIFO is empty
  if((readStatusReg(CC1101_TXBYTES) & 0x7F) == 0)
    return true;

  return false;
}

/**
 * receiveData
 * 
 * Read data packet from RX FIFO
 * 
 * Return:
 * 	Amount fo bytes received
 * 'packet'	Container for the packet received
 */
byte CC1101::receiveData(CCPACKET * packet)
{
  byte val;

  // Any byte waiting to be read?
  if (readStatusReg(CC1101_RXBYTES) & 0x7F)
  {
    // Read data length
    packet->length = readConfigReg(CC1101_RXFIFO);

    // If packewt is too long
    if (packet->length > CC1101_DATA_LEN)
      packet->length = 0;   // Discard packet
    else
    {
      // Read data packet
      readBurstReg(packet->data, CC1101_RXFIFO, packet->length);
      // Read RSSI
      packet->rssi = readConfigReg(CC1101_RXFIFO);
      // Read LQI and CRC_OK
      val = readConfigReg(CC1101_RXFIFO);
      packet->lqi = val & 0x7F;
      packet->crc_ok = bitRead(val, 7);
    }
  }
  else
    packet->length = 0;

  // Flush RX FIFO
  cmdStrobe(CC1101_SFRX);

  // Enter back into RX state
  setRxState(); 

  return packet->length;
}

