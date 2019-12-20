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
// MainWindow & UI includes
//------------------------------------------------------------------------------

#include "AppInfo.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

//------------------------------------------------------------------------------
// LibRFID includes
//------------------------------------------------------------------------------

#include <RFID.h>
#include <RFID_SerialManager.h>

//------------------------------------------------------------------------------
// Library includes
//------------------------------------------------------------------------------

#include <QFile>
#include <QTimer>
#include <QScreen>
#include <QScrollBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QSerialPort>
#include <QApplication>
#include <QDesktopServices>
#include <QStandardItemModel>

//------------------------------------------------------------------------------
// Custom utility functions
//------------------------------------------------------------------------------

/**
 * Displays the given @a data in Hexadecimal format and separates each byte
 * with a space (for easier reading)
 */
static QString ByteArrayToHex(const QByteArray& data)
{
   // Init empty string
   QString str;

   // Get unformatted HEX representation of data
   QString hex = QString::fromUtf8(data.toHex());

   // Separate HEX values into packets of two characters
   int i = 0;
   while(i < hex.length()) {
      str.append(hex.at(i));

      if(hex.length() > i + 1) {
         str.append(hex.at(i + 1));
         str.append(" ");
         i += 2;
      }

      else
         ++i;
   }

   // Remove extra spaces
   while(str.startsWith(" "))
      str.remove(0,1);
   while(str.endsWith(" "))
      str.chop(1);

   // Return obtained string
   return str;
}

static QByteArray HexToBinary(const QString& hex, bool* ok = Q_NULLPTR)
{
   // Init. variables
   QByteArray binary;
   QString data = hex;

   // Remove spaces
   data.remove(" ");

   // Assign true to validation pointer initially
   if(ok != Q_NULLPTR)
      *ok = true;

   // Convert hex string to binary (each byte is two chars long)...
   for(int i = 0; i < data.length(); i += 2) {
      // Get string for current byte
      QString str;
      str.append(data.at(i));
      str.append(data.at(i + 1));

      // Convert byte string to number
      quint8 byte = static_cast<quint8>(str.toUInt(ok, 16));
      if(ok != Q_NULLPTR) {
         if(*ok)
            binary.append(static_cast<char>(byte));
         else
            return QByteArray();
      }
   }

   // Return byte array
   return binary;
}

//------------------------------------------------------------------------------
// Constructor & destructor functions
//------------------------------------------------------------------------------

/**
 * Configures UI controls, connects signals/slots between UI controls and
 * libRFID, reads saved settings and loads application information to the
 * appropiate labels and controls
 */
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
   // Initialize UI objects
   ui = new Ui::MainWindow;
   ui->setupUi(this);

   // Configure UI data values & connect signals/slots
   configureUi();
   connectSlots();

   // Read settings
   readSettings();

   // Display current app info on the UI
   setAppInfo();
}

/**
 * Deletes UI controls from memory
 */
MainWindow::~MainWindow()
{
   delete ui;
}

//------------------------------------------------------------------------------
// UI configuration & signal setup functions
//------------------------------------------------------------------------------

/**
 * @brief MainWindow::setAppInfo
 *
 * Loads application information data and displays it on the UI of the
 * MainWindow automatically.
 */
void MainWindow::setAppInfo()
{
   // Construct app name & app version strings
   const QString appName = APP_NAME;
   const QString appVersion = tr("Version %1 %2")
                              .arg(APP_VERSION)
                              .arg(APP_CHANNEL);
   const QString appCopyright = tr("Copyright (c) %1 %2")
                                .arg(2019)
                                .arg(APP_ORGANIZATION);

   // Change values displayed on the UI
   ui->HT_AppName_Label->setText(appName);
   ui->HD_AppName_Label->setText(appName);
   ui->HT_AppOrg_Label->setText(appCopyright);
   ui->HT_AppVersion_Label->setText(appVersion);
   ui->HD_AppVersion_Label->setText(appVersion);

   // Set window title
   setWindowTitle(QString("%1 - %2").arg(appName).arg(appVersion));
}

/**
 * @brief MainWindow::configureUi
 *
 * Configures the UI elements and the information they display before the
 * MainWindow is shown to the user.
 */
void MainWindow::configureUi()
{
   // Display first tab
   ui->MW_TabWidget->setCurrentIndex(0);

   // Fill hardware config combos
   updateBaudRates();
   updateRfidReaders();

   // Generate monospace font
   QFont monospace = qApp->font();
#if defined(Q_OS_WIN)
   monospace.setPointSize(10);
   monospace.setFamily("Consolas");
#elif defined(Q_OS_LINUX)
   monospace.setPointSize(11);
   monospace.setFamily("DejaVu Sans Mono");
#elif defined(Q_OS_OSX)
   monospace.setPointSize(12);
   monospace.setFamily("Menlo");
#else
   monospace.setPointSize(10);
   monospace.setFamily("Monospace");
#endif

   // Use monospace fonts on console text editors
   ui->TH_TableView->setFont(monospace);
   ui->TM_EPC_LineEdit->setFont(monospace);
   ui->TM_RFU_LineEdit->setFont(monospace);
   ui->TM_TagID_LineEdit->setFont(monospace);
   ui->TM_UserData_TextEdit->setFont(monospace);
   ui->TM_MemoryDump_TextEdit->setFont(monospace);

   // Enable disable controls accordingly
   updateTagManagementControls();

   // Set status LED color and text
   updateStatus();

   // Set tag history headers
   updateTagsTable();

   // Set fixed window size, move window to top-left corner
   resize(minimumSize());
   move(100, 20);
}

/**
 * @brief MainWindow::connectSlots
 *
 * Connects the signals/slots between the UI elements and the appication logic
 */
void MainWindow::connectSlots()
{
   // Connect serial manager signals/slots
   RFID_SerialManager* srmg = RFID_SerialManager::getInstance();
   connect(srmg, &RFID_SerialManager::availableDevicesChanged,
           this, &MainWindow::updateSerialDevices);
   connect(srmg, &RFID_SerialManager::connectionStatusChanged,
           this, &MainWindow::onPortConnectionChanged);

   // Connect reader selection to RFID bridge
   connect(ui->HC_Readers_Combo,
           SIGNAL(currentIndexChanged(int)),
           RFID::getInstance(),
           SLOT(setReader(int)));

   // Change baud rate when the baud rate combo's index is changed
   connect(ui->HC_BaudRate_Combo,
           SIGNAL(currentIndexChanged(int)),
           srmg, SLOT(setBaudRate(int)));

   // Disconnect current device if user selects another device while being
   // connected to a serial port
   connect(ui->HC_SerialPort_Combo,
           SIGNAL(currentIndexChanged(int)),
           this, SLOT(updateDevice(int)));

   // Hardware configuration tab signals/slots
   connect(ui->HC_Connect_Button,
           &QPushButton::clicked,
           this, &MainWindow::connectDevice);

   // Update tag management controls automatically
   connect(RFID::getInstance(),
           &RFID::currentTagChanged,
           this, &MainWindow::updateTagManagementControls);
   connect(RFID::getInstance(),
           &RFID::tagUpdated,
           this, &MainWindow::updateTagManagementControls);

   // Update RFID history table automatically
   connect(RFID::getInstance(),
           &RFID::tagCountChanged,
           this, &MainWindow::updateTagsTable);
   connect(RFID::getInstance(),
           &RFID::currentTagChanged,
           this, &MainWindow::updateTagsTable);
   connect(RFID::getInstance(),
           &RFID::tagUpdated,
           this, &MainWindow::updateTagsTable);

   // Table controls
   connect(ui->TH_Clear_Button,
           &QPushButton::clicked,
           RFID::getInstance(),
           &RFID::clearHistory);
   connect(ui->TH_Export_Button,
           &QPushButton::clicked,
           this, &MainWindow::exportTagsTable);

   // Connect tag management signals/slots
   connect(ui->TM_Kill_Button,
           &QPushButton::clicked,
           this, &MainWindow::killTag);
   connect(ui->TM_Block_Button,
           &QPushButton::clicked,
           this, &MainWindow::lockTag);
   connect(ui->TM_Format_Button,
           &QPushButton::clicked,
           this, &MainWindow::eraseTag);

   // Connect RFID tag writing signals/slots
   connect(ui->TM_WriteRfu_Button,
           &QPushButton::clicked,
           this, &MainWindow::writeRfuData);
   connect(ui->TM_WriteEpc_Button,
           &QPushButton::clicked,
           this, &MainWindow::writeEpcData);
   connect(ui->TM_WriteUserData_Button,
           &QPushButton::clicked,
           this, &MainWindow::writeUserData);
}

//------------------------------------------------------------------------------
// Application state loading & saving functions
//------------------------------------------------------------------------------

void MainWindow::readSettings()
{

}

void MainWindow::saveSettings()
{

}

//------------------------------------------------------------------------------
// Hardware configuration tab functions
//------------------------------------------------------------------------------

/**
 * @brief MainWindow::updateStatus
 *
 * Updates the status LED and text based on what the RFID reader driver reports
 * to the program. If the RFID reader driver is ready, then the MainWindow will
 * show a green LED and a "RFID Reader Ready" message on the status area.
 *
 * Otherwise, the MainWindow will show a red LED and "Waiting for RFID Reader"
 * message on the status area.
 */
void MainWindow::updateStatus()
{
   if(RFID::getInstance()->readerAccessible()) {
      ui->HD_RfidStatus_Icon->setEnabled(true);
      ui->HD_RfidStatus_Label->setEnabled(true);
      ui->HD_RfidStatus_Label->setText(tr("RFID Reader Ready"));
   }

   else {
      ui->HD_RfidStatus_Icon->setEnabled(false);
      ui->HD_RfidStatus_Label->setEnabled(false);
      ui->HD_RfidStatus_Label->setText(tr("Waiting for RFID Reader"));
   }

   QTimer::singleShot(1000, this, &MainWindow::updateStatus);
}

/**
 * @brief MainWindow::connectDevice
 *
 * This function is called when the user clicks on the connect/disconnect
 * button in the Hardware Configuration tab. This function toggles the
 * connection state with the current serial device selected by the user.
 */
void MainWindow::connectDevice()
{
   RFID_SerialManager* sm = RFID_SerialManager::getInstance();

   if(!sm->connected()) {
      RFID::getInstance()->clearHistory();
      sm->setBaudRate(ui->HC_BaudRate_Combo->currentIndex());
      sm->setDevice(ui->HC_SerialPort_Combo->currentIndex());
   }

   else
      sm->disconnectDevice(false);
}

/**
 * @brief MainWindow::updateBaudRates
 * Queries for available baud rates from the system and displays them on the
 * baud rate selector combobox.
 */
void MainWindow::updateBaudRates()
{
   // Fill baud rates combobox
   ui->HC_BaudRate_Combo->clear();
   QStringList list = RFID_SerialManager::getInstance()->availableBaudRates();
   for(int i = 0; i < list.count(); ++i)
      ui->HC_BaudRate_Combo->addItem(list.at(i));

   // Select 9600 baud rate index
   ui->HC_BaudRate_Combo->setCurrentText("9600");
}

/**
 * @brief MainWindow::updateRfidReaders
 * Updates the items displayed on the RFID reader combobox.
 */
void MainWindow::updateRfidReaders()
{
   ui->HC_Readers_Combo->clear();
   foreach(QString reader, RFID::getInstance()->rfidReaders())
      ui->HC_Readers_Combo->addItem(reader);

   RFID::getInstance()->setReader(0);
}

/**
 * @brief MainWindow::updateSerialDevices
 *
 * Updates the elements displayed on the serial device selector combobox.
 */
void MainWindow::updateSerialDevices()
{
   // Update devices combobox
   ui->HC_SerialPort_Combo->clear();
   QStringList devices = RFID_SerialManager::getInstance()->availableDevices();
   for(int i = 0; i < devices.count(); ++i)
      ui->HC_SerialPort_Combo->addItem(devices.at(i));

   // If there are no devices, add dummy text
   if(devices.count() == 0)
      ui->HC_SerialPort_Combo->addItem(tr("No serial devices found"));

   // Enable/disable connect button based on number of devices
   ui->HC_Connect_Button->setEnabled(devices.count() > 0);
}

/**
 * @brief MainWindow::onPortConnectionChanged
 *
 * Called when the serial port connection status is changed. This function
 * updates the user interface components, such as the HEX console and the
 * connect button text and check state.
 */
void MainWindow::onPortConnectionChanged()
{
   // Serial port connected, allow user to manually disconnect from device
   if(RFID_SerialManager::getInstance()->connected()) {
      ui->HC_Connect_Button->setChecked(true);
      ui->HC_Connect_Button->setText(tr("Disconnect"));
   }

   // Serial port disconnected, allow user to connected to another device
   else {
      ui->HC_Connect_Button->setChecked(false);
      ui->HC_Connect_Button->setText(tr("Connect"));
   }
}

/**
 * Enables or disables the tag management controls depending if the RFID reader
 * has access to an RFID chip or not.
 *
 * Also, the function will read current RFID tag data and display it on the
 * appropiate user interface controls.
 */
void MainWindow::updateTagManagementControls()
{
   QString epc, tid, rfu, usr, mem;
   RFID_Tag* tag = RFID::getInstance()->currentTag();
   ui->MW_TagManagement_Tab->setEnabled(tag != Q_NULLPTR);

   if(tag) {
      epc = ByteArrayToHex(tag->epc);
      tid = ByteArrayToHex(tag->tid);
      rfu = ByteArrayToHex(tag->rfu);
      usr = ByteArrayToHex(RFID::getInstance()->getUserData(tag));
      mem = RFID::getInstance()->generateMemoryMap(tag);
   }

   ui->TM_EPC_LineEdit->setText(epc);
   ui->TM_RFU_LineEdit->setText(rfu);
   ui->TM_TagID_LineEdit->setText(tid);
   ui->TM_UserData_TextEdit->setPlainText(usr);
   ui->TM_MemoryDump_TextEdit->setPlainText(mem);
}

/**
 * @brief MainWindow::updateDevice
 * @param index unused
 *
 * This function disconnects the current serial device when the user selects
 * a different serial device from the serial device combobox.
 */
void MainWindow::updateDevice(const int index)
{
   (void) index;
   RFID_SerialManager::getInstance()->disconnectDevice(false);
}

/**
 * @brief MainWindow::exportTagsTable
 *
 * Exports the tag history table to a CSV file for further reference
 */
void MainWindow::exportTagsTable()
{
   // Used to ask user if he/she wants to open file after writing
   bool ok = true;

   // Ask user for file save location
   QString l = QFileDialog::getSaveFileName(this,
                                            tr("Export Tag Data"),
                                            QDir::homePath(),
                                            tr("Comma Separated Values (*.csv);"));

   // Check if location is valid
   if(l.isEmpty())
      return;

   // Try to open file for writing
   QFile file(l);
   if(file.open(QFile::WriteOnly)) {
      // Get table model
      QStandardItemModel* model = static_cast<QStandardItemModel*>(ui->TH_TableView->model());

      // Get row & column count
      int rows = model->rowCount();
      int columns = model->columnCount();

      // Create CSV data string
      QString csv = tr("Tag ID,EPC,User Data,Reserved Data\n");
      for(int i = 0; i < rows; i++) {
         for(int j = 0; j < columns; j++) {
            csv += model->data(model->index(i,j)).toString();
            csv += ",";
         }

         csv += "\n";
      }

      // Write data to file
      if(file.write(csv.toUtf8()) != csv.toUtf8().length()) {
         ok = false;
         QMessageBox::critical(this,
                               tr("File write error"),
                               tr("Could not write table data to \"%1\"").arg(l));
      }

      // Close file
      file.close();
   }

   // File open error
   else {
      ok = false;
      QMessageBox::critical(this,
                            tr("File open error"),
                            tr("Cannot open \"%1\" for writting!").arg(l));
   }

   // Notify user and ask to open file using system apps
   if(ok) {
      int ans = QMessageBox::question(this,
                                      tr("Information"),
                                      tr("The tag data table was successfully"
                                         " exported, do you want to open it?"),
                                      QMessageBox::Yes | QMessageBox::No);

      if(ans == QMessageBox::Yes)
         QDesktopServices::openUrl(QUrl::fromLocalFile(l));
   }
}

/**
 * @brief MainWindow::onTagCountChanged
 *
 * Generates the RFID history table when the tag count is changed
 */
void MainWindow::updateTagsTable()
{
   // Get pointer to RFID manager instance
   RFID* rfid = RFID::getInstance();

   // Generate curated tag list (only accept tags that have at least
   // tag Id and EPC present)
   RFID_TagList list;
   for(int i = 0; i < rfid->tagCount(); ++i) {
      RFID_Tag* tag = rfid->rfidTags().at(i);
      if(!tag->epc.isEmpty() && !tag->tid.isEmpty())
         list.append(tag);
   }

   // Display tag count
   ui->TH_TagCount_LCD->display(list.count());

   // Create table model
   QTableView* table = ui->TH_TableView;
   QStandardItemModel* model = new QStandardItemModel(
      list.count(), 4, table);

   // Set table headers
   QStringList labels = {tr("Tag ID"), tr("EPC"), tr("User Data"), tr("RFU")};
   model->setHorizontalHeaderLabels(labels);

   // Stretch column headers
   table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

   // Get current tag EPC & TID data (to highlight it on the table)
   QString ct_epc;
   QString ct_tid;
   if(rfid->currentTag()) {
      ct_epc = ByteArrayToHex(rfid->currentTag()->epc);
      ct_tid = ByteArrayToHex(rfid->currentTag()->tid);
   }

   // Add data to table
   for(int i = 0; i < model->rowCount(); ++i) {
      // Get pointer to RFID tag data structure
      RFID_Tag* tag = list.at(i);

      // Get tag data in Hex format
      QString epcStr = ByteArrayToHex(tag->epc);
      QString tidStr = ByteArrayToHex(tag->tid);
      QString rfuStr = ByteArrayToHex(tag->rfu);
      QString usrStr = ByteArrayToHex(rfid->getUserData(tag));

      // Add data to model
      model->setItem(i, 0, new QStandardItem(tidStr));
      model->setItem(i, 1, new QStandardItem(epcStr));
      model->setItem(i, 2, new QStandardItem(usrStr));
      model->setItem(i, 3, new QStandardItem(rfuStr));

      // Highlight current tag
      if(epcStr == ct_epc && tidStr == ct_tid) {
         model->item(i, 0)->setBackground(QBrush(Qt::darkGreen));
         model->item(i, 1)->setBackground(QBrush(Qt::darkGreen));
         model->item(i, 2)->setBackground(QBrush(Qt::darkGreen));
         model->item(i, 3)->setBackground(QBrush(Qt::darkGreen));
      }
   }

   // Set table model
   table->setModel(model);
}

//------------------------------------------------------------------------------
// RFID tag management functions
//------------------------------------------------------------------------------

/**
 * @brief MainWindow::killTag
 *
 * Instructs libRFID to try to kill the current tag, message boxes and user
 * notifications will be handled automatically by the @a RFID class.
 */
void MainWindow::killTag()
{
   RFID::getInstance()->killTag();
}

/**
 * @brief MainWindow::lockTag
 *
 * Instructs libRFID to try to lock the current tag, message boxes and user
 * notifications will be handled automatically by the @a RFID class.
 */
void MainWindow::lockTag()
{
   RFID::getInstance()->lockTag();
}

/**
 * @brief MainWindow::eraseTag
 *
 * Instructs libRFID to try to erase the current tag, message boxes and user
 * notifications will be handled automatically by the @a RFID class.
 */
void MainWindow::eraseTag()
{
   RFID::getInstance()->eraseTag();
}

void MainWindow::writeEpcData()
{
   // Get EPC binary data
   bool epcOk;
   QByteArray epc = HexToBinary(ui->TM_EPC_LineEdit->text(), &epcOk);

   // Validate EPC conversion
   if(!epcOk) {
      QMessageBox::critical(this,
                            tr("Encoding error"),
                            tr("Cannot encode EPC data into binary data"));
      return;
   }

   // If EPC data length is less than EPC requirement, add zeroes
   while(epc.length() < RFID_EPC_LENGTH)
      epc.append(static_cast<char>(0x00));

   // If EPC data length is too large, complain
   if(epc.length() > RFID_EPC_LENGTH) {
      QMessageBox::critical(this,
                            tr("Encoding error"),
                            tr("EPC data cannot be larger than %1 bytes")
                            .arg(RFID_EPC_LENGTH));
      return;
   }

   // Write EPC and RFU data
   epcOk = RFID::getInstance()->writeEpc(epc);

   // Show success messagebox
   if(epcOk)
      QMessageBox::information(this,
                               tr("RFID Manager"),
                               tr("RFID Tag data updated successfully!"));

   // Show EPC writting error
   else
      QMessageBox::critical(this,
                            tr("Tag Update Error"),
                            tr("An error has ocurred while trying to update "
                               "the tag's EPC data"));
}

void MainWindow::writeRfuData()
{
   // Get RFU binary data
   bool rfuOk;
   QByteArray rfu = HexToBinary(ui->TM_RFU_LineEdit->text(), &rfuOk);

   // Get user data fields

   // Validate RFU conversion
   if(!rfuOk) {
      QMessageBox::critical(this,
                            tr("Encoding error"),
                            tr("Cannot encode RFU data into binary data"));
      return;
   }

   // If RFU data length is less than RFU requirement, add zeroes
   while(rfu.length() < RFID_RFU_LENGTH)
      rfu.append(static_cast<char>(0x00));

   // If RFU data length is too large, complain
   if(rfu.length() > RFID_RFU_LENGTH) {
      QMessageBox::critical(this,
                            tr("Encoding error"),
                            tr("RFU data cannot be larger than %1 bytes")
                            .arg(RFID_RFU_LENGTH));
      return;
   }

   // Write RFU data
   rfuOk = RFID::getInstance()->writeRfu(rfu);

   // Show success messagebox
   if(rfuOk)
      QMessageBox::information(this,
                               tr("RFID Manager"),
                               tr("RFID Tag data updated successfully!"));

   // Show RFU writting error
   else
      QMessageBox::critical(this,
                            tr("Tag Update Error"),
                            tr("An error has ocurred while trying to update "
                               "the tag's RFU data"));
}

void MainWindow::writeUserData()
{
   // Get user binary data
   bool userOk;
   QByteArray user = HexToBinary(ui->TM_UserData_TextEdit->toPlainText(),
                                 &userOk);

   // Validate user conversion
   if(!userOk) {
      QMessageBox::critical(this,
                            tr("Encoding error"),
                            tr("Cannot encode RFU data into binary data"));
      return;
   }

   // If RFU data length is less than RFU requirement, add zeroes
   while(user.length() < RFID_USER_LENGTH)
      user.append(static_cast<char>(0x00));

   // If RFU data length is too large, complain
   if(user.length() > RFID_USER_LENGTH) {
      QMessageBox::critical(this,
                            tr("Encoding error"),
                            tr("RFU data cannot be larger than %1 bytes")
                            .arg(RFID_USER_LENGTH));
      return;
   }

   // Write user data
   userOk = RFID::getInstance()->writeUserData(user);

   // Show success messagebox
   if(userOk)
      QMessageBox::information(this,
                               tr("RFID Manager"),
                               tr("RFID Tag data updated successfully!"));

   // Show RFU writting error
   else
      QMessageBox::critical(this,
                            tr("Tag Update Error"),
                            tr("An error has ocurred while trying to update "
                               "the tag's RFU data"));
}

/**
 * @brief MainWindow::writeRfidData
 *
 * Instructs libRFID to try to write the new EPC, RFU and UserData given by
 * the user through the user interface.
 */
void MainWindow::writeRfidData()
{

}

/**
 * @brief MainWindow::copyMemoryDump
 *
 * Copies the memory dump text into the system clipboard.
 */
void MainWindow::copyMemoryDump()
{
   ui->TM_MemoryDump_TextEdit->copy();
}

/**
 * @brief MainWindow::savesMemoryDump
 *
 * Saves the memory dump text into a text file in a location and file name
 * specified by the user.
 */
void MainWindow::saveMemoryDump()
{

}

//------------------------------------------------------------------------------
// Help tab functions
//------------------------------------------------------------------------------

/**
 * @brief MainWindow::openWebsite
 *
 * Opens the project's website through the system's web browser
 */
void MainWindow::openWebsite()
{

}

/**
 * @brief MainWindow::reportError
 *
 * Opens the project's issues website through the system's web browser
 */
void MainWindow::reportError()
{

}
