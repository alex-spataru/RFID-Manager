#
# Copyright (c) 2019 Alex Spataru <https://github.com/alex-spataru>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

#-------------------------------------------------------------------------------
# Project configuration
#-------------------------------------------------------------------------------

TEMPLATE = app
TARGET = RFID-Manager

#-------------------------------------------------------------------------------
# Deploy options
#-------------------------------------------------------------------------------

win32* {
    RC_FILE = $$PWD/deploy/windows/resources/info.rc
}

#-------------------------------------------------------------------------------
# Qt modules
#-------------------------------------------------------------------------------

QT += xml
QT += svg
QT += core
QT += widgets

#-------------------------------------------------------------------------------
# Include libraries
#-------------------------------------------------------------------------------

include($$PWD/lib/RFID/RFID.pri)

#-------------------------------------------------------------------------------
# Import source code
#-------------------------------------------------------------------------------

DISTFILES += \
    $$PWD/LICENSE.md \
    $$PWD/README.md \
    $$PWD/util/scripts/astyle-format-code.bat

RESOURCES += \
    $$PWD/resources/resources.qrc

FORMS += \
    $$PWD/src/MainWindow.ui

HEADERS += \
    $$PWD/src/AppInfo.h \
    $$PWD/src/MainWindow.h

SOURCES += \
    $$PWD/src/MainWindow.cpp \
    $$PWD/src/main.cpp
