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

//------------------------------------------------------------------------------
// Library headers
//------------------------------------------------------------------------------

#include "RFID.h"
#include "RFID_Reader.h"
#include "RFID_SerialManager.h"

//------------------------------------------------------------------------------
// RFID Reader Drivers
//------------------------------------------------------------------------------

#include "SM_6210.h"

//------------------------------------------------------------------------------
// Qt libraries
//------------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>

#include <QTimer>
#include <QMessageBox>

//------------------------------------------------------------------------------
// Utility functions
//------------------------------------------------------------------------------

static QString HexDump(const char* data, size_t size)
{
   char str[4096] = "";
   char ascii[17];

   size_t i, j;
   ascii[16] = '\0';
   for(i = 0; i < size; ++i) {
      sprintf(str + strlen(str), "%02X ", static_cast<quint8>(data[i]));

      if(data[i] >= ' ' &&  data[i] <= '~')
         ascii[i % 16] = data[i];
      else
         ascii[i % 16] = '.';

      if((i+1) % 8 == 0 || i+1 == size) {
         sprintf(str + strlen(str), " ");
         if((i+1) % 16 == 0)
            sprintf(str + strlen(str), "|  %s \n", ascii);

         else if(i+1 == size) {
            ascii[(i+1) % 16] = '\0';

            if((i+1) % 16 <= 8)
               sprintf(str + strlen(str), " ");
            for(j = (i+1) % 16; j < 16; ++j)
               sprintf(str + strlen(str), "   ");

            sprintf(str + strlen(str), "|  %s \n", ascii);
         }
      }
   }

   return QString(str);
}

//------------------------------------------------------------------------------
// Constructor, destructor & instance access functions
//------------------------------------------------------------------------------

/**
 * Pointer to the only instance of the @c RFID_Bridge class
 */
static RFID* INSTANCE = Q_NULLPTR;

/**
 * @brief RFID_Bridge::RFID_Bridge
 *
 * Initializes internal variables and begins the periodic scan loop
 */
RFID::RFID()
{
   // Set null reader
   m_reader = Q_NULLPTR;

   // Configure watchdog timer
   m_watchdog.setInterval(RFID_CURRENT_TAG_TIMEOUT);
   connect(&m_watchdog, &QTimer::timeout, this, &RFID::resetCurrentTag);

   // Start scanner & watchdog timer
   QTimer::singleShot(1000, this, SLOT(scan()));
   QTimer::singleShot(1000, &m_watchdog, SLOT(start()));
}

/**
 * @brief RFID_Bridge::~RFID_Bridge
 *
 * Destroys the current RFID reader class before the @c RFID_Bridge class
 * is destroyed
 */
RFID::~RFID()
{
   unloadReader();
}

/**
 * @brief RFID_Bridge::getInstance
 * @returns the only instance of the @a RFID_Bridge class
 */
RFID* RFID::getInstance()
{
   if(INSTANCE == Q_NULLPTR)
      INSTANCE = new RFID;

   return INSTANCE;
}

//------------------------------------------------------------------------------
// Misc. info access functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_Bridge::chipCount
 * @returns the number of tags read by the current RFID reader
 */
int RFID::tagCount() const
{
   return rfidTags().count();
}

/**
 * @brief RFID_Bridge::readerAccessible
 * @returns @c true if a RFID reader driver is loaded and the current RFID
 *          reader is ready to be used
 */
bool RFID::readerAccessible() const
{
   if(reader() != Q_NULLPTR)
      return reader()->loaded();

   return false;
}

/**
 * @brief RFID_Bridge::reader
 * @returns a pointer to the current RFID reader driver
 */
RFID_Reader* RFID::reader() const
{
   return m_reader;
}

/**
 * @brief RFID_Bridge::currentTag
 * @returns a pointer to the current RFID tag being scanned/written by the
 *          RFID reader
 */
RFID_Tag* RFID::currentTag()
{
   if(reader())
      return reader()->currentTag();

   return Q_NULLPTR;
}

/**
 * @brief RFID_Bridge::rfidTags
 * @returns a list of RFIDS tags found by the RFID reader
 */
RFID_TagList RFID::rfidTags() const
{
   return m_tags;
}

/**
 * @brief RFID_Bridge::rfidReaders
 * @returns a list with supported RFID reader devices
 */
QStringList RFID::rfidReaders() const
{
   return QStringList {
      "SM-6210 USB UHF RFID Programmer"
   };
}

/**
 * @brief RFID::getUserData
 * Generates a @c QByteArray from all the user data sections of the given @a
 * tag.
 */
QByteArray RFID::getUserData(const RFID_Tag* tag) const
{
   Q_ASSERT(tag);

   QByteArray data;
   for(int i = 0; i < RFID_NUM_USER_DATAGRAMS; ++i)
      data.append(tag->usr[i]);

   return data;
}

/**
 * @brief RFID::generateMemoryMap
 * @param tag
 * @returns a string with all the data found in the given RFID @a tag
 */
QString RFID::generateMemoryMap(const RFID_Tag* tag) const
{
   Q_ASSERT(tag);

   // Init dump string
   QString dump;

   // Add tag ID data
   dump.append(tr("# Tag ID (%1 bytes)\n").arg(tag->tid.length()));
   dump.append(HexDump(
                  tag->tid.constData(),
                  static_cast<size_t>(tag->tid.length())));
   dump.append("\n");

   // Add EPC data
   dump.append(tr("# EPC (%1 bytes)\n").arg(tag->epc.length()));
   dump.append(HexDump(
                  tag->epc.constData(),
                  static_cast<size_t>(tag->epc.length())));
   dump.append("\n");

   // Add user data
   QByteArray usr = getUserData(tag);
   dump.append(tr("# User data (%1 bytes)\n").arg(usr.length()));
   dump.append(HexDump(
                  usr.constData(),
                  static_cast<size_t>(usr.length())));
   dump.append("\n");

   // Add RFU data
   dump.append(tr("# RFU (%1 bytes)\n").arg(tag->rfu.length()));
   dump.append(HexDump(
                  tag->rfu.constData(),
                  static_cast<size_t>(tag->rfu.length())));
   dump.append("\n");

   // Return dump string
   return dump;
}

/**
 * @brief RFID_Bridge::clearHistory
 *
 * Clears the RFID tag list and read count indicators
 */
void RFID::clearHistory()
{
   m_tags.clear();
   resetCurrentTag();
   emit tagCountChanged();
}

//------------------------------------------------------------------------------
// Reader loading/unloading
//------------------------------------------------------------------------------

/**
 * @brief RFID_Bridge::unloadReader
 *
 * Unloads the current RFID reader driver from memory
 */
void RFID::unloadReader()
{
   if(m_reader) {
      clearHistory();
      disconnect(m_reader, &RFID_Reader::epcFound, this, &RFID::onEpcFound);
      disconnect(m_reader, &RFID_Reader::tidFound, this, &RFID::onTidFound);
      disconnect(m_reader, &RFID_Reader::usrFound, this, &RFID::onUsrFound);
      disconnect(m_reader, &RFID_Reader::rfuFound, this, &RFID::onRfuFound);

      m_reader->deleteLater();
      m_reader = Q_NULLPTR;
   }
}

/**
 * @brief RFID_Bridge::setReader
 * @param index index of the RFID reader from the list returned by the
 *        @c rfidReaders() function
 */
void RFID::setReader(const int index)
{
   if(index == 0)
      setReader(new SM_6210());
}

/**
 * @brief RFID_Bridge::setReader
 * @param newReader pointer to the new RFID reader driver
 *
 * Changes the RFID reader driver used to read, write and manage RFID tags
 */
void RFID::setReader(RFID_Reader* newReader)
{
   if(newReader) {
      unloadReader();
      clearHistory();

      m_reader = newReader;
      connect(m_reader, &RFID_Reader::epcFound, this, &RFID::onEpcFound);
      connect(m_reader, &RFID_Reader::tidFound, this, &RFID::onTidFound);
      connect(m_reader, &RFID_Reader::usrFound, this, &RFID::onUsrFound);
      connect(m_reader, &RFID_Reader::rfuFound, this, &RFID::onRfuFound);

      emit readerChanged();
   }
}

//------------------------------------------------------------------------------
// Reader interface functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_Bridge::lockTag
 *
 * Asks the user for confirmation to lock the current tag manged by the RFID
 * reader. If user acknowledgement is given, then the driver shall use its
 * own means to block the current RFID tag->
 */
void RFID::lockTag()
{
   int r = QMessageBox::question(Q_NULLPTR,
                                 tr("Block tag"),
                                 tr("Are you sure you want to block tag?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);

   if(r == QMessageBox::Yes && reader()) {
      if(reader()->lockTag())
         QMessageBox::information(Q_NULLPTR,
                                  tr("Block tag"),
                                  tr("Current tag was successfully blocked")
                                 );

      else
         QMessageBox::critical(Q_NULLPTR,
                               tr("Block tag"),
                               tr("An error occurred while trying to "
                                  "block the current tag"));
   }
}

/**
 * @brief RFID_Bridge::killTag
 *
 * Asks the user for confirmation to kill the current tag manged by the RFID
 * reader. If user acknowledgement is given, then the driver shall use its
 * own means to kill the current RFID tag->
 */
void RFID::killTag()
{
   int r = QMessageBox::question(Q_NULLPTR,
                                 tr("Kill tag"),
                                 tr("Are you sure you want to kill tag?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);

   if(r == QMessageBox::Yes && reader()) {
      if(reader()->killTag())
         QMessageBox::information(Q_NULLPTR,
                                  tr("Kill tag"),
                                  tr("Current tag was successfully "
                                     "killed"));
      else
         QMessageBox::critical(Q_NULLPTR,
                               tr("Kill tag"),
                               tr("An error occurred while trying to "
                                  "kill the current tag"));
   }
}

/**
 * @brief RFID_Bridge::eraseTag
 *
 * Asks the user for confirmation to format the current tag manged by the RFID
 * reader. If user acknowledgement is given, then the driver shall use its
 * own means to format the current RFID tag->
 */
void RFID::eraseTag()
{
   int r = QMessageBox::question(Q_NULLPTR,
                                 tr("Format tag"),
                                 tr("Are you sure you want to format tag?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);

   if(r == QMessageBox::Yes && reader()) {
      if(reader()->eraseTag())
         QMessageBox::information(Q_NULLPTR,
                                  tr("Erase tag"),
                                  tr("Current tag was successfully "
                                     "erased."));
      else
         QMessageBox::critical(Q_NULLPTR,
                               tr("Erase tag"),
                               tr("An error occurred while trying to "
                                  "erase/format the current tag"));
   }
}

/**
 * @brief RFID_Bridge::writeEpc
 * @param epc new EPC data
 * @return @c true if operation is successfull, @c false if not
 *
 * Changes the EPC data of the current RFID tag using the means implemented
 * by the RFID reader driver loaded in memory.
 */
bool RFID::writeEpc(const QByteArray& epc)
{
   if(readerAccessible())
      return reader()->writeEpc(epc);

   return false;
}

/**
 * @brief RFID_Bridge::writeRfu
 * @param rfu new reserved for future use data
 * @return @c true if operation is successfull, @c false if not
 *
 * Changes the RFU data of the current RFID tag using the means implemented
 * by the RFID reader driver loaded in memory.
 */
bool RFID::writeRfu(const QByteArray& rfu)
{
   if(readerAccessible())
      return reader()->writeRfu(rfu);

   return false;
}

/**
 * @brief RFID_Bridge::writeUserData
 * @param userData new user data
 * @return @c true if operation is successfull, @c false if not
 *
 * Changes the user data of the current RFID tag using the means implemented
 * by the RFID reader driver loaded in memory.
 */
bool RFID::writeUserData(const QByteArray& userData)
{
   if(readerAccessible())
      return reader()->writeUserData(userData);

   return false;
}

/**
 * @brief RFID_Bridge::scan
 * Scans for new and existing RFID tags and updates their information.
 */
void RFID::scan()
{
   if(readerAccessible())
      reader()->scan();

   QTimer::singleShot(RFID_CURRENT_TAG_TIMEOUT / 50, this, &RFID::scan);
}

/**
 * @brief RFID::resetCurrentTag
 *
 * Called when the watchdog timer expires, this happens when the reader cannot
 * communicate with a tag after some amount of time.
 */
void RFID::resetCurrentTag()
{
   if(reader()) {
      reader()->setCurrentTag(Q_NULLPTR);
      emit currentTagChanged();
   }

   m_watchdog.start();
}

//------------------------------------------------------------------------------
// Slots for incoming RFID tag data management
//------------------------------------------------------------------------------

/**
 * @brief RFID::onEpcFound
 * @param epc
 *
 * Updates the @a epc data of the current tag or registers a new tag based
 * on current tag information
 */
void RFID::onEpcFound(const QByteArray& epc)
{
   RFID_Tag* tag = new RFID_Tag;
   tag->epc = epc;

   if(currentTag()) {
      if(currentTag()->epc != epc && !currentTag()->epc.isEmpty())
         updateTagList(tag);
      else {
         updateTagData(&currentTag()->epc, epc);
         free(tag);
      }
   }

   else
      updateTagList(tag);
}

/**
 * @brief RFID::onTidFound
 * @param tid
 *
 * Updates the @a tid data of the current tag or registers a new tag based
 * on current tag information
 */
void RFID::onTidFound(const QByteArray& tid)
{
   RFID_Tag* tag = new RFID_Tag;
   tag->tid = tid;

   if(currentTag()) {
      if(currentTag()->tid != tid && !currentTag()->tid.isEmpty())
         updateTagList(tag);
      else {
         updateTagData(&currentTag()->tid, tid);
         updateTagList(currentTag());
         free(tag);
      }
   }

   else
      updateTagList(tag);
}

/**
 * @brief RFID::onUsrFound
 * @param usr user data
 *
 * Updates the @a usr data of the current tag or registers a new tag based
 * on current tag information
 */
void RFID::onUsrFound(const QByteArray& usr, const int datagram)
{
   Q_ASSERT(datagram < RFID_NUM_USER_DATAGRAMS && datagram >= 0);

   RFID_Tag* tag = new RFID_Tag;
   tag->usr[datagram] = usr;

   if(currentTag()) {
      if(currentTag()->usr[datagram] != usr && !currentTag()->usr[datagram].isEmpty())
         updateTagList(tag);
      else {
         updateTagData(&currentTag()->usr[datagram], usr);
         updateTagList(currentTag());
         free(tag);
      }
   }

   else
      updateTagList(tag);
}

/**
 * @brief RFID::onRfuFound
 * @param rfu reserved data
 *
 * Updates the @a rfu data of the current tag or registers a new tag based
 * on current tag information
 */
void RFID::onRfuFound(const QByteArray& rfu)
{
   RFID_Tag* tag = new RFID_Tag;
   tag->rfu = rfu;

   if(currentTag()) {
      if(currentTag()->rfu != rfu && !currentTag()->rfu.isEmpty())
         updateTagList(tag);
      else {
         updateTagData(&currentTag()->rfu, rfu);
         updateTagList(currentTag());
         free(tag);
      }
   }

   else
      updateTagList(tag);
}

//------------------------------------------------------------------------------
// Tag history management
//------------------------------------------------------------------------------

/**
 * @brief RFID::updateTagList
 * @param tag
 *
 * Registers the given @a tag and its data to the tag history list, and manages
 * the tag list so that data is not duplicated.
 *
 * @note the @a tag will not be complete, so this function is in charge of
 *       generating a tag list with complete information over time.
 */
void RFID::updateTagList(RFID_Tag* tag)
{
   Q_ASSERT(tag != Q_NULLPTR && reader());

   // Reset watchdog
   m_watchdog.start();

   // Do not add tag to list if it already exists
   bool tagFound = false;

   // Try to update existing tag (so we can "slowly" gather more information
   // about each RFID tag that is being scanned).
   for(int i = 0; i < tagCount(); ++i) {
      RFID_Tag* t = m_tags.at(i);
      if(t->epc == tag->epc || t->tid == tag->tid) {
         tagFound = true;
         updateTagData(&t->epc, tag->epc);
         updateTagData(&t->tid, tag->tid);
         updateTagData(&t->rfu, tag->rfu);

         for(int i = 0; i < RFID_NUM_USER_DATAGRAMS; ++i)
            updateTagData(&t->usr[i], tag->usr[i]);

         break;
      }
   }

   // Tag not found on list, register new tag..
   if(!tagFound) {
      m_tags.append(tag);
      emit tagCountChanged();
   }

   // Cleanup tag list to remove duplicates
   int tags = tagCount();
   for(int i = 0; i < tags; ++i) {
      RFID_Tag* ta = m_tags.at(i);
      for(int j = i + 1; j < tags; ++j) {
         RFID_Tag* tb = m_tags.at(j);
         if(ta == tb)
            m_tags.removeAt(j);
         else if(ta->tid == tb->tid)
            m_tags.removeAt(j);

         if(tags != tagCount()) {
            tags = tagCount();
            emit tagCountChanged();
         }
      }
   }

   // Change current tag
   if(currentTag() != tag) {
      reader()->setCurrentTag(tag);
      emit currentTagChanged();
   }
}

/**
 * @brief RFID::updateTagData
 * @param dest pointer to destination byte array
 * @param src  source byte array
 *
 * Compares the contents of @a dest and src and determines if the tag data
 * needs to be updated. In the eventual case of a tag update, the appropiate
 * signals are sent by this function.
 */
void RFID::updateTagData(QByteArray* dest, const QByteArray& src)
{
   if(!src.isEmpty() && *dest != src) {
      *dest = src;
      emit tagUpdated();
   }
}

