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

#include "RFID_SerialManager.h"

#include <QTimer>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>

/**
 * Pointer to the only instance of the @c RFID_SerialManager class
 */
static RFID_SerialManager* INSTANCE = Q_NULLPTR;

//------------------------------------------------------------------------------
// Constructor, destructor & single instance access functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_SerialManager::RFID_SerialManager
 * Begins the serial port device polling process and initialzes internal
 * variables to appropiate values
 */
RFID_SerialManager::RFID_SerialManager()
{
   // Init. internal variables
   m_currentDevice = Q_NULLPTR;
   m_availableDevices = QStringList(tr("Please wait..."));

   // Begin serial device polling process
   QTimer::singleShot(1000, this, &RFID_SerialManager::updateDevices);
}

/**
 * @brief RFID_SerialManager::~RFID_SerialManager
 * Disconnects from current device (if any) before destroying the class
 */
RFID_SerialManager::~RFID_SerialManager()
{
   disconnectDevice(true);
}

/**
 * @brief RFID_SerialManager::getInstance
 * @returns the one and only instance of this class
 */
RFID_SerialManager* RFID_SerialManager::getInstance()
{
   if(INSTANCE == Q_NULLPTR)
      INSTANCE = new RFID_SerialManager;

   return INSTANCE;
}

//------------------------------------------------------------------------------
// Serial status access functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_SerialManager::baudRate
 * @return the baud rate used by the current device, 0 if no device is connected
 */
int RFID_SerialManager::baudRate() const
{
   if(connected())
      return currentDevice()->baudRate();

   return 0;
}

/**
 * @brief RFID_SerialManager::connected
 * @return @c true if the current device is open, @c false if the current device
 *         is not available or the program is not connected to any device
 */
bool RFID_SerialManager::connected() const
{
   if(m_currentDevice != Q_NULLPTR)
      return m_currentDevice->isOpen();

   return false;
}

/**
 * @brief RFID_SerialManager::currentDevice
 * @return the pointer to the current device
 */
QSerialPort* RFID_SerialManager::currentDevice() const
{
   return m_currentDevice;
}

//------------------------------------------------------------------------------
// Serial devices & baud rates access functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_SerialManager::availableDevices
 * @returns a string list with the port names and descriptions for each
 *          serial port device available to the computer
 */
QStringList RFID_SerialManager::availableDevices() const
{
   return m_availableDevices;
}

/**
 * @brief RFID_SerialManager::avialableBaudRates
 * @returns a string list with the baud rates available to the system
 */
QStringList RFID_SerialManager::availableBaudRates() const
{
   QStringList list;

   QList<qint32> baudRates = QSerialPortInfo::standardBaudRates();
   for(int i = 0; i < baudRates.count(); ++i)
      list.append(QString("%1").arg(baudRates.at(i)));

   return list;
}

/**
 * @brief RFID_SerialManager::writeData
 * @param data
 *
 * Writes the given @a data to the current serial device and returns the number
 * of bytes sent through the serial port.
 */
qint64 RFID_SerialManager::writeData(const QByteArray& data)
{
   if(connected()) {
      qint64 bytes = currentDevice()->write(data);
      emit dataSent(data.chopped(data.length() - static_cast<int>(bytes)));
      return bytes;
   }

   return -1;
}

//------------------------------------------------------------------------------
// Device configuration functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_SerialManager::disconnectDevice
 *
 * Disconnects the current serial device (if any) and removes signal/slot
 * connections between the device driver and the serial manager.
 *
 * @param silent if set to @c false, a message box will be displayed to notify
 *               the user that the device has been disconnected
 */
void RFID_SerialManager::disconnectDevice(bool silent)
{
   if(m_currentDevice) {
      const QString portName = m_currentDevice->portName();

      disconnect(m_currentDevice, &QSerialPort::readyRead,
                 this, &RFID_SerialManager::onReadyRead);
      disconnect(m_currentDevice, &QSerialPort::bytesWritten,
                 this, &RFID_SerialManager::bytesSent);

      m_currentDevice->close();
      m_currentDevice->deleteLater();
      m_currentDevice = Q_NULLPTR;

      if(!silent) {
         QMessageBox::warning(Q_NULLPTR,
                              tr("Information"),
                              tr("Disconnected from device at %1")
                              .arg(portName));
      }

      emit connectionStatusChanged();
   }
}

/**
 * @brief RFID_SerialManager::setDevice
 *
 * Tries to establish a connection with the serial device at the given
 * @a deviceIndex from the @c QStringList returned by the
 * @c availableDevices() function.
 *
 * @note If an existing serial device is connected through this class,
 *       then that device will be removed before establishing the new
 *       serial connection.
 */
void RFID_SerialManager::setDevice(int deviceIndex)
{
   Q_ASSERT(deviceIndex >= 0);
   Q_ASSERT(deviceIndex < availableDevices().count());

   // Disconnect current device
   if(m_currentDevice)
      disconnectDevice(true);

   // Change device pointer
   QSerialPortInfo info = QSerialPortInfo::availablePorts().at(deviceIndex);
   m_currentDevice = new QSerialPort(info, this);

   // Set device options
   m_currentDevice->setBaudRate(m_baudRate);

   // Try to open device
   if(m_currentDevice->open(QIODevice::ReadWrite)) {
      connect(m_currentDevice, &QSerialPort::readyRead,
              this, &RFID_SerialManager::onReadyRead);
      connect(m_currentDevice, &QSerialPort::bytesWritten,
              this, &RFID_SerialManager::bytesSent);
      QMessageBox::information(Q_NULLPTR,
                               tr("Information"),
                               tr("Connected with %1 successfully")
                               .arg(currentDevice()->portName()));
      emit connectionStatusChanged();
   } else {
      QMessageBox::critical(Q_NULLPTR,
                            tr("Warning"),
                            tr("Failed to communicate with %1")
                            .arg(currentDevice()->portName()));
      disconnectDevice(true);
   }
}

/**
 * @brief RFID_SerialManager::setBaudRate
 *
 * Changes the baud rate of the serial port, if the port is connected, the
 * changes are reflected immediately.
 */
void RFID_SerialManager::setBaudRate(int baudRateIndex)
{
   Q_ASSERT(baudRateIndex >= 0);
   Q_ASSERT(baudRateIndex < availableBaudRates().count());

   m_baudRate = QSerialPortInfo::standardBaudRates().at(baudRateIndex);

   if(currentDevice() != Q_NULLPTR)
      currentDevice()->setBaudRate(m_baudRate);

   emit baudRateChanged();
}

/**
 * @brief RFID_SerialManager::onReadyRead
 *
 * Reads the incoming data from the current device and generates a signal
 * with that data...
 */
void RFID_SerialManager::onReadyRead()
{
   if(connected()) {
      const QByteArray data = currentDevice()->readAll();
      emit dataReceived(data);
   }
}

//------------------------------------------------------------------------------
// Device discovery functions
//------------------------------------------------------------------------------

/**
 * @brief RFID_SerialManager::updateDevices
 *
 * Probes for available serial devices every second. If the available serial
 * devices are different from the last time this function was called, then this
 * function shall emit a signal to notify the rest of the application of the
 * changes...
 */
void RFID_SerialManager::updateDevices()
{
   // Probe for available serial devices
   QStringList devices;
   foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
      if(!port.description().isEmpty())
         devices.append(QString("%1 (%2)")
                        .arg(port.description())
                        .arg(port.portName()));
   }

   // Compare current available devices with previous available devices
   if(devices != availableDevices()) {
      m_availableDevices = devices;
      emit availableDevicesChanged();
   }

   // Call this function again in one second
   QTimer::singleShot(1000, this, &RFID_SerialManager::updateDevices);
}
