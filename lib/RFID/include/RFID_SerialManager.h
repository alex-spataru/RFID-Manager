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

#ifndef RFID_SERIAL_MANAGER_H
#define RFID_SERIAL_MANAGER_H

#include <QObject>
#include <QStringList>

class QSerialPort;
class RFID_SerialManager : public QObject
{
      Q_OBJECT

   signals:
      void baudRateChanged();
      void availableDevicesChanged();
      void connectionStatusChanged();
      void bytesSent(const qint64 bytes);
      void dataSent(const QByteArray& data);
      void dataReceived(const QByteArray& data);

   public:
      static RFID_SerialManager* getInstance();

      int baudRate() const;
      bool connected() const;

      QSerialPort* currentDevice() const;
      QStringList availableDevices() const;
      QStringList availableBaudRates() const;
      qint64 writeData(const QByteArray& data);

   private:
      explicit RFID_SerialManager();
      ~RFID_SerialManager();

   public slots:
      void setDevice(int deviceIndex);
      void disconnectDevice(bool silent);
      void setBaudRate(int baudRateIndex);

   private slots:
      void onReadyRead();
      void updateDevices();

   private:
      qint32 m_baudRate;
      QSerialPort* m_currentDevice;
      QStringList m_availableDevices;
};

#endif
