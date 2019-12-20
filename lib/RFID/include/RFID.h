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

#ifndef RFID_H
#define RFID_H

#include "RFID_Global.h"

#include <QTimer>

class RFID_Reader;
class RFID : public QObject
{
      Q_OBJECT

   signals:
      void tagUpdated();
      void readerChanged();
      void tagCountChanged();
      void currentTagChanged();

   public:
      static RFID* getInstance();

      int tagCount() const;
      bool readerAccessible() const;

      RFID_Reader* reader() const;
      RFID_Tag* currentTag();

      RFID_TagList rfidTags() const;
      QStringList rfidReaders() const;

      QByteArray getUserData(const RFID_Tag* tag) const;
      QString generateMemoryMap(const RFID_Tag* tag) const;

      bool writeEpc(const QByteArray& epc);
      bool writeRfu(const QByteArray& rfu);
      bool writeUserData(const QByteArray& userData);

   public slots:
      void clearHistory();
      void unloadReader();
      void setReader(const int index);
      void setReader(RFID_Reader* newReader);

      void lockTag();
      void killTag();
      void eraseTag();

   private slots:
      void scan();
      void resetCurrentTag();
      void onEpcFound(const QByteArray& epc);
      void onTidFound(const QByteArray& tid);
      void onUsrFound(const QByteArray& usr, const int datagram);
      void onRfuFound(const QByteArray& rfu);

   private:
      RFID();
      ~RFID();

      void updateTagList(RFID_Tag* tag);
      void updateTagData(QByteArray* dest, const QByteArray& src);

   private:
      QTimer m_watchdog;
      RFID_TagList m_tags;
      RFID_Reader* m_reader;
};

#endif
