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

#ifndef RFID_READER_H
#define RFID_READER_H

#include "RFID_Global.h"

/**
 * @brief The RFID_Reader class
 *
 * Definition for basic interface driver between the RFID Bridge object and
 * device-specific implementations for RFID readers.
 *
 * @note All virtual functions must be implemented for correct operation of
 *       the RFID reader and the rest of the RFID Manager software.
 */
class RFID_Reader : public QObject
{
      Q_OBJECT

   signals:
      void tidFound(const QByteArray& tid);
      void epcFound(const QByteArray& epc);
      void rfuFound(const QByteArray& rfu);
      void usrFound(const QByteArray& usr, const int datagram);

   public:
      RFID_Reader()
      {
         m_currentTag = Q_NULLPTR;
      }

      inline RFID_Tag* currentTag() const
      {
         return m_currentTag;
      }

      inline void setCurrentTag(RFID_Tag* tag)
      {
         m_currentTag = tag;
      }

      virtual void scan() = 0;
      virtual bool loaded() = 0;
      virtual void readEpc() = 0;
      virtual void readTid() = 0;
      virtual void readRfu() = 0;
      virtual void readUsr() = 0;
      virtual bool killTag() = 0;
      virtual bool lockTag() = 0;
      virtual bool eraseTag() = 0;
      virtual bool writeRfu(const QByteArray& rfu) = 0;
      virtual bool writeEpc(const QByteArray& epc) = 0;
      virtual bool writeUserData(const QByteArray& userData) = 0;

   private:
      RFID_Tag* m_currentTag;
};

#endif
