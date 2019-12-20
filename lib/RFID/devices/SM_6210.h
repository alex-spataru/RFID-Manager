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

#ifndef UHF_SM_6210_DRIVER_H
#define UHF_SM_6210_DRIVER_H

#include "RFID_Reader.h"

class SM_6210 : public RFID_Reader
{
      Q_OBJECT

   public: SM_6210();
      ~SM_6210();

      void scan();
      bool loaded();
      void readEpc();
      void readTid();
      void readRfu();
      void readUsr();
      bool killTag();
      bool lockTag();
      bool eraseTag();
      bool writeEpc(const QByteArray& epc);
      bool writeRfu(const QByteArray& rfu);
      bool writeUserData(const QByteArray& userData);

   private slots:
      void onDataReceived(const QByteArray& data);

   private:
      bool readAckPacket();
      bool readEpcPacket();
      bool readRfuPacket();
      bool readUsrPacket();
      bool readStdResult();
      bool readStdResponse();
      bool readTagIdPacket();
      bool readEpcPacketFromScan();

      bool writeData(const QByteArray& data,
                     const quint8 label[2],
                     const quint8 startAddress,
                     const quint8 length);

      QByteArray readInformationPacket(const quint8 label[2],
                                       bool* ok = nullptr,
                                       int* start = nullptr,
                                       int* length = nullptr,
                                       const bool singleTag = false,
                                       const bool verifyChecksum = true);

   private:
      qint8 m_selector;
      quint8 m_shitCount;
      quint8 m_userStartAddress;
};

#endif
