/*
 * Copyright (c) 2019 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SM_6210.h"
#include "RFID_Global.h"
#include "RFID_SerialManager.h"

#include <QSerialPort>

//------------------------------------------------------------------------------
// Temporary buffer
//------------------------------------------------------------------------------

static QByteArray BUFFER;

//------------------------------------------------------------------------------
// Define the serial device command bytes
//------------------------------------------------------------------------------

// Header type byes
static const quint8 HEADER_START_CODE       = 0xa0;
static const quint8 HEADER_RESULT_CODE      = 0xe4;
static const quint8 HEADER_RESPONSE_CODE    = 0xe0;

// Communication modes
static const quint8 COMM_RS232              = 0x03;

// Operation command codes
static const quint8 DEV_STOP_SEARCH         = 0xa8;
static const quint8 DEV_WRITE_TAG_MW        = 0xab;
static const quint8 DEV_GET_SINGLE_PARAM    = 0x61;
static const quint8 DEV_READ_SINGLE_TAG     = 0x82;
static const quint8 DEV_READ_TAG_DATA       = 0x80;

// Data reading options
static const quint8 RFU_LABEL[2]            = {0x00, 0x00};
static const quint8 EPC_LABEL[2]            = {0x00, 0x01};
static const quint8 TID_LABEL[2]            = {0x00, 0x02};
static const quint8 USR_LABEL[2]            = {0x00, 0x03};

// Card reader paremeters
static const quint8 CRP_ADD_USERCODE        = 0x64;

//------------------------------------------------------------------------------
// Utility functions
//------------------------------------------------------------------------------

static quint8 Checksum(const QByteArray& data)
{
   quint8 checksum = 0;

   for(int i = 0; i < data.length(); ++i)
      checksum += data.at(i);

   return (~checksum) + 1;
}

//------------------------------------------------------------------------------
// Driver constructor & destructor implementations
//------------------------------------------------------------------------------

SM_6210::SM_6210()
{
   Checksum(QByteArray());

   m_selector = 0;
   m_shitCount = 0;
   m_userStartAddress = 0;
   connect(RFID_SerialManager::getInstance(),
           &RFID_SerialManager::dataReceived,
           this,
           &SM_6210::onDataReceived);
}

SM_6210::~SM_6210()
{
   disconnect(RFID_SerialManager::getInstance(),
              &RFID_SerialManager::dataReceived,
              this,
              &SM_6210::onDataReceived);
}

//------------------------------------------------------------------------------
// Driver function implementations
//------------------------------------------------------------------------------

/**
 * @brief UHF_530_RDM::scan
 * Asks the UHF reader to send EPC, TagID, User and RFU data repeatedly for
 * the current tag. If current tag is @c NULL, then the function shall ask the
 * UHF reader to perform a quick-scan for EPC in near RFID tags. If no tags are
 * found, then the function shall reset the UHF reader every 10 cycles.
 */
void SM_6210::scan()
{
   if(!currentTag()) {
      m_selector = 0;

      // Send stop-and-reset command every 10 failed cycles
      if(m_shitCount > 10) {
         m_shitCount = 0;
         QByteArray data;
         data.append(static_cast<char>(HEADER_START_CODE));
         data.append(static_cast<char>(COMM_RS232));
         data.append(static_cast<char>(DEV_STOP_SEARCH));
         data.append(static_cast<char>(0x00));
         data.append(static_cast<char>(Checksum(data)));
         RFID_SerialManager::getInstance()->writeData(data);
      }

      // Send tag read request command
      else {
         QByteArray data;
         data.append(static_cast<char>(HEADER_START_CODE));
         data.append(static_cast<char>(0x05));
         data.append(static_cast<char>(DEV_GET_SINGLE_PARAM));
         data.append(static_cast<char>(0x00));
         data.append(static_cast<char>(0x00));
         data.append(static_cast<char>(CRP_ADD_USERCODE));
         data.append(static_cast<char>(Checksum(data)));
         RFID_SerialManager::getInstance()->writeData(data);
      }
   }

   else {
      switch(m_selector) {
         case 0:
            readTid();
            break;
         case 1:
            readRfu();
            break;
         case 2:
            readUsr();
            break;
         default:
            readEpc();
            m_selector = -1;
            break;
      }

      ++m_selector;
   }
}

/**
 * @brief UHF_530_RDM::loaded
 * Returns @c true if the serial manager has a device connected and configured
 * to communicate with a baud rade of 9600.
 */
bool SM_6210::loaded()
{
   RFID_SerialManager* sm = RFID_SerialManager::getInstance();
   if(sm->connected())
      return sm->baudRate() == 9600;

   return false;
}

//------------------------------------------------------------------------------
// Tag data access functions
//------------------------------------------------------------------------------

/**
 * @brief UHF_530_RDM::readEpc
 * Sends an EPC data request packet to the UHF reader.
 */
void SM_6210::readEpc()
{
   const quint8 dataLength = 6;
   const quint8 startAddress = 2;

   QByteArray data;
   data.append(static_cast<char>(HEADER_START_CODE));
   data.append(static_cast<char>(0x06));
   data.append(static_cast<char>(DEV_READ_TAG_DATA));
   data.append(static_cast<char>(EPC_LABEL[0]));
   data.append(static_cast<char>(EPC_LABEL[1]));
   data.append(static_cast<char>(startAddress));
   data.append(static_cast<char>(dataLength));
   data.append(static_cast<char>(Checksum(data)));

   RFID_SerialManager::getInstance()->writeData(data);
}

/**
 * @brief UHF_530_RDM::readTid
 * Sends a TagID data request packet to the UHF reader.
 */
void SM_6210::readTid()
{
   const quint8 dataLength = 6;
   const quint8 startAddress = 0;

   QByteArray data;
   data.append(static_cast<char>(HEADER_START_CODE));
   data.append(static_cast<char>(0x06));
   data.append(static_cast<char>(DEV_READ_TAG_DATA));
   data.append(static_cast<char>(TID_LABEL[0]));
   data.append(static_cast<char>(TID_LABEL[1]));
   data.append(static_cast<char>(startAddress));
   data.append(static_cast<char>(dataLength));
   data.append(static_cast<char>(Checksum(data)));

   RFID_SerialManager::getInstance()->writeData(data);
}

/**
 * @brief UHF_530_RDM::readRfu
 * Sends a RFU data request packet to the UHF reader.
 */
void SM_6210::readRfu()
{
   const quint8 dataLength = 4;
   const quint8 startAddress = 0;

   QByteArray data;
   data.append(static_cast<char>(HEADER_START_CODE));
   data.append(static_cast<char>(0x06));
   data.append(static_cast<char>(DEV_READ_TAG_DATA));
   data.append(static_cast<char>(RFU_LABEL[0]));
   data.append(static_cast<char>(RFU_LABEL[1]));
   data.append(static_cast<char>(startAddress));
   data.append(static_cast<char>(dataLength));
   data.append(static_cast<char>(Checksum(data)));

   RFID_SerialManager::getInstance()->writeData(data);
}

/**
 * @brief UHF_530_RDM::readUsr
 * Sends an user data request packet to the UHF reader.
 */
void SM_6210::readUsr()
{
   // Reset user start address if necessary
   if(m_userStartAddress > 24)
      m_userStartAddress = 0;

   // Set data length & start address
   const quint8 dataLength = 8;
   const quint8 startAddress = m_userStartAddress;

   // Generate & send packet
   QByteArray data;
   data.append(static_cast<char>(HEADER_START_CODE));
   data.append(static_cast<char>(0x06));
   data.append(static_cast<char>(DEV_READ_TAG_DATA));
   data.append(static_cast<char>(USR_LABEL[0]));
   data.append(static_cast<char>(USR_LABEL[1]));
   data.append(static_cast<char>(startAddress));
   data.append(static_cast<char>(dataLength));
   data.append(static_cast<char>(Checksum(data)));
   RFID_SerialManager::getInstance()->writeData(data);

   // Increase start address by data length
   m_userStartAddress += dataLength;
}

//------------------------------------------------------------------------------
// Tag management functions
//------------------------------------------------------------------------------

/**
 * @brief UHF_530_RDM::killTag
 *
 * Sends a tag kill command to the UHF reader, returns @c true if the UHF reader
 * managed to kill the current tag, @c false if there was an error.
 */
bool SM_6210::killTag()
{
   return false;
}

/**
 * @brief UHF_530_RDM::lockTag
 *
 * Sends a lock tag command to the UHF reader, returns @c true if the UHF reader
 * managed to lock the current tag, @c false if there was an error.
 */
bool SM_6210::lockTag()
{
   return false;
}

/**
 * @brief UHF_530_RDM::eraseTag
 *
 * Tries to erase all EPC, User and RFU data from the current tag.
 * Returns @c true on success, @c false on failure.
 */
bool SM_6210::eraseTag()
{
   QByteArray epc;
   for(int i = 0; i < 12; ++i)
      epc.append(static_cast<char>(0x00));

   QByteArray usr;
   for(int i = 0; i < 13; ++i)
      usr.append(static_cast<char>(0x00));

   QByteArray rfu;
   for(int i = 0; i < 8; ++i)
      rfu.append(static_cast<char>(0x00));

   return writeEpc(epc) && writeRfu(rfu) && writeUserData(usr);
}

//------------------------------------------------------------------------------
// Data writing function
//------------------------------------------------------------------------------

bool SM_6210::writeEpc(const QByteArray& epc)
{
   if(currentTag())
      return writeData(epc, EPC_LABEL, 2, 6);

   return false;
}

bool SM_6210::writeRfu(const QByteArray& rfu)
{
   if(currentTag())
      return writeData(rfu, RFU_LABEL, 0, 4);

   return false;
}

bool SM_6210::writeUserData(const QByteArray& userData)
{
   if(currentTag()) {
      bool ok = true;
      ok &= writeData(userData.mid(0, 16), USR_LABEL, 0, 8);
      ok &= writeData(userData.mid(16, 32), USR_LABEL, 8, 8);
      ok &= writeData(userData.mid(32, 48), USR_LABEL, 16, 8);
      ok &= writeData(userData.mid(48, 64), USR_LABEL, 24, 8);
      return ok;
   }

   return false;
}

//------------------------------------------------------------------------------
// Data management functions
//------------------------------------------------------------------------------

/**
 * @brief UHF_530_RDM::onDataReceived
 * @param data
 *
 * Slot function called when the serial manager detects any incoming data from
 * the UHF serial reader.
 *
 * This function appends the given @a data to a buffer and lets the data
 * interpretation functions to read and manage the buffer automatically.
 *
 * If for some reason the buffer size is too large, then this function shall
 * clear the buffer to avoid memory problems.
 */
void SM_6210::onDataReceived(const QByteArray& data)
{
   // Data is empty, abort
   if(data.isEmpty())
      return;

   // Driver not ready, abort
   if(!loaded())
      return;

   // Append data to buffer
   BUFFER.append(data);

   // Interpret received data
   if(BUFFER.size() > 0) {
      // Try to interpret packet...
      if(readAckPacket())
         return;
      if(readEpcPacketFromScan())
         return;
      else if(readEpcPacket())
         return;
      else if(readTagIdPacket())
         return;
      else if(readRfuPacket())
         return;
      else if(readUsrPacket())
         return;
      else if(readStdResponse())
         return;
      else if(readStdResult())
         return;

      // Packet not read, increase shit count in order to send reset command
      // to UHF reader every now an then
      ++m_shitCount;

      // Clear buffer if it exceeds max size
      if(BUFFER.size() > RFID_MAX_BUFFER_SIZE)
         BUFFER.clear();
   }
}

//------------------------------------------------------------------------------
// Packet interpretation functions
//------------------------------------------------------------------------------

/**
 * @brief UHF_530_RDM::readAckPacket
 *
 * Reads and interprets a single-tag search acknowledge packet from the UHF
 * reader. If an ACK packet is found and read successfully, then the function
 * shall return @c true and will respond to the UHF reader so that normal tag
 * searching operations can begin.
 */
bool SM_6210::readAckPacket()
{
   // Get information packet header shift
   int i;
   for(i = 0; i < BUFFER.length(); ++i)
      if(BUFFER.at(i) == static_cast<char>(HEADER_RESPONSE_CODE))
         break;

   // Check buffer size
   if(i + 8 > BUFFER.length())
      return false;

   // Check header
   bool ok = true;
   ok &= BUFFER[i + 0] == static_cast<char>(HEADER_RESPONSE_CODE);
   ok &= BUFFER[i + 1] == static_cast<char>(0x06);
   ok &= BUFFER[i + 2] == static_cast<char>(DEV_GET_SINGLE_PARAM);
   ok &= BUFFER[i + 3] == static_cast<char>(0x00);
   ok &= BUFFER[i + 4] == static_cast<char>(0x00);
   ok &= BUFFER[i + 5] == static_cast<char>(CRP_ADD_USERCODE);
   ok &= BUFFER[i + 6] == static_cast<char>(0x00);
   ok &= BUFFER[i + 7] == static_cast<char>(Checksum(BUFFER.mid(i, 7)));

   // Header ok, respond with acknowledgement packet and delete read bytes
   if(ok) {
      BUFFER.remove(i, 8);

      QByteArray data;
      data.append(static_cast<char>(HEADER_START_CODE));
      data.append(static_cast<char>(COMM_RS232));
      data.append(static_cast<char>(DEV_READ_SINGLE_TAG));
      data.append(static_cast<char>(0x00));
      data.append(static_cast<char>(Checksum(data)));
      RFID_SerialManager::getInstance()->writeData(data);
   }

   // Return value
   return ok;
}

/**
 * @brief UHF_530_RDM::readStdResult
 *
 * Reads and interprets any result packet from the UHF reader, we don't really
 * do something with this information, but it is important to manage the
 * incoming data buffer so that we can extract information from useful data
 * packets.
 *
 * Returns @c true if a result packet was found and ignored, @c false if no
 * response packet was found.
 */
bool SM_6210::readStdResult()
{
   // Get result packet header shift
   int shift;
   for(shift = 0; shift < BUFFER.length(); ++shift) {
      // Header result code found, get packet size and remove it from buffer
      if(BUFFER.at(shift) == static_cast<char>(HEADER_RESULT_CODE)) {
         if(BUFFER.length() > shift + 1) {
            int packetSize = BUFFER.at(shift + 1);
            BUFFER.remove(0, shift + packetSize);
            return true;
         }

         // Get out of loop
         break;
      }
   }

   // Cannot get packet size, ignore
   return false;
}

/**
 * @brief UHF_530_RDM::readStdResponse
 *
 * Reads and interprets any response packet from the UHF reader, we don't really
 * do something with this information, but it is important to manage the
 * incoming data buffer so that we can extract information from useful data
 * packets.
 *
 * Returns @c true if a response packet was found and ignored, @c false if no
 * response packet was found.
 */
bool SM_6210::readStdResponse()
{
   // Get result packet header shift
   int shift;
   for(shift = 0; shift < BUFFER.length(); ++shift) {
      // Header response code found, remove it if packet size is less than 6
      if(BUFFER.at(shift) == static_cast<char>(HEADER_RESPONSE_CODE)) {
         if(BUFFER.length() > shift + 1) {
            int packetSize = BUFFER.at(shift + 1);
            if(packetSize < 6) {
               BUFFER.remove(0, shift + packetSize);
               return true;
            }
         }

         // Get out of loop
         break;
      }
   }

   // Cannot get packet size, ignore
   return false;
}

/**
 * @brief UHF_530_RDM::readTagIdPacket
 *
 * Reads and interprets a TagID information packet, returns @c true on success
 * or @c false on failure.
 */
bool SM_6210::readTagIdPacket()
{
   bool success = false;
   QByteArray tid = readInformationPacket(TID_LABEL, &success);
   if(success)
      emit tidFound(tid);

   return success;
}

/**
 * @brief UHF_530_RDM::readEpcPacket
 *
 * Reads and interprets an EPC information packet, returns @c true on success
 * or @c false on failure.
 */
bool SM_6210::readEpcPacket()
{
   bool success = false;
   QByteArray epc = readInformationPacket(EPC_LABEL, &success);
   if(success)
      emit epcFound(epc);

   return success;
}

/**
 * @brief UHF_530_RDM::readRfuPacket
 *
 * Reads and interprets a RFU information packet, returns @c true on success
 * or @c false on failure.
 */
bool SM_6210::readRfuPacket()
{
   bool success = false;
   QByteArray rfu = readInformationPacket(RFU_LABEL, &success);
   if(success)
      emit rfuFound(rfu);

   return success;
}

/**
 * @brief UHF_530_RDM::readUsrPacket
 *
 * Reads and interprets an user data packet for the first 13 bytes of data,
 * returns @c true if the packet was read successfully, @c false if there
 * was an error while reading and interpreting the packet.
 */
bool SM_6210::readUsrPacket()
{
   int startAddress = 0;
   bool success = false;
   QByteArray usr = readInformationPacket(USR_LABEL, &success, &startAddress);

   if(success) {
      const int datagram = startAddress / 8;
      if(datagram > RFID_NUM_USER_DATAGRAMS - 1 || datagram < 0)
         return false;

      emit usrFound(usr, datagram);
   }

   return success;
}

/**
 * @brief UHF_530_RDM::readEpcPacketFromScan
 *
 * Reads and interprets quick-scan EPC information packets. Returns @c true if
 * packet was read successfully, @c false if there was an error while reading
 * and interpreting the packet data.
 */
bool SM_6210::readEpcPacketFromScan()
{
   bool success = false;
   QByteArray epc = readInformationPacket(EPC_LABEL, &success,
                                          nullptr, nullptr, true, false);

   if(success)
      emit epcFound(epc);

   return success;
}

//------------------------------------------------------------------------------
// Generic packet reading & writing functions
//------------------------------------------------------------------------------

/**
 * @brief SM_6210::writeData
 * @param data data to write
 * @param label section in which to write the data into (EPC, RFU, etc..)
 * @param startAddress start address of data section
 * @param length number of addresses to write
 * @return
 *
 * Writes the given @a data to the current RFID tag
 */
bool SM_6210::writeData(const QByteArray& data,
                        const quint8 label[2],
                        const quint8 startAddress,
                        const quint8 length)
{
   // Generate packet
   QByteArray packet;
   packet.append(static_cast<char>(HEADER_START_CODE));
   packet.append(static_cast<char>(DEV_WRITE_TAG_MW));
   packet.append(static_cast<char>(label[0]));
   packet.append(static_cast<char>(label[1]));
   packet.append(static_cast<char>(startAddress));
   packet.append(static_cast<char>(length));
   packet.append(data);
   packet.insert(1, static_cast<char>(packet.length()));
   packet.append(static_cast<char>(Checksum(packet)));

   // Send packet
   bool ok = true;
   RFID_SerialManager* sm = RFID_SerialManager::getInstance();
   for(int i = 0; i < 10; ++i)
      ok &= (sm->writeData(packet) == packet.length());

   // Serial error
   return ok;
}

/**
 * @brief SM_6210::readInformationPacket
 * @param label data type label (two bytes)
 * @param ok set to @c true if packet is read successfully
 * @param startAddress start address of data
 * @param length length of received data
 * @param verifyChecksum verify incoming data with checksum
 *
 * Reads and interprets any information packet received from the SM-6210 reader
 */
QByteArray SM_6210::readInformationPacket(const quint8 label[2],
                                          bool* ok,
                                          int* startAddress,
                                          int* length,
                                          const bool singleTag,
                                          const bool verifyChecksum)
{
   // Verify arguments
   Q_ASSERT(ok != Q_NULLPTR);

   // Set success parameter to false
   *ok = false;

   // Get information packet header shift
   int shift;
   for(shift = 0; shift < BUFFER.length(); ++shift)
      if(BUFFER.at(shift) == static_cast<char>(HEADER_RESPONSE_CODE))
         break;

   // Packet just contains response code, wait for more data
   if(BUFFER.length() < shift + 1)
      return QByteArray();

   // Verify packet size
   quint8 size = static_cast<quint8>(BUFFER.at(shift + 1));
   if(BUFFER.length() > shift + size) {
      // Check response labels
      bool headerOk = true;
      headerOk &= BUFFER[shift + 3] == static_cast<char>(label[0]);
      headerOk &= BUFFER[shift + 4] == static_cast<char>(label[1]);

      // Check response type
      if(singleTag)
         headerOk &= BUFFER[shift + 2] == static_cast<char>(DEV_READ_SINGLE_TAG);
      else
         headerOk &= BUFFER[shift + 2] == static_cast<char>(DEV_READ_TAG_DATA);

      // Check header to see if this is a
      if(!headerOk) {
         *ok = false;
         return QByteArray();
      }

      // Get start address
      if(startAddress)
         *startAddress = static_cast<int>(BUFFER[shift + 5]);

      // Get data length (in bytes)
      quint8 len = static_cast<quint8>(BUFFER[shift + 6]) * 2;
      if(length)
         *length = static_cast<int>(len);

      // Verify checksum and get read data
      quint8 checksum = static_cast<quint8>(BUFFER[shift + len + 7]);
      if(checksum == Checksum(BUFFER.mid(shift, len + 7)) || !verifyChecksum) {
         // Get EPC
         QByteArray data;
         for(int j = 0; j < len; ++j)
            data.append(BUFFER.at(shift + 7 + j));

         // Remove read data from buffer
         BUFFER.remove(0, shift + len + 7);

         // Return obtained data
         *ok = true;
         return data;
      }
   }

   // Checksum or packet size invalid
   return QByteArray();
}
