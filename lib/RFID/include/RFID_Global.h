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

#ifndef RFID_GLOBAL_H
#define RFID_GLOBAL_H

#include <QList>
#include <QObject>
#include <QByteArray>

#define RFID_NUM_KEYS               2
#define RFID_RFU_LENGTH             8
#define RFID_EPC_LENGTH             12
#define RFID_TID_LENGTH             12
#define RFID_USER_LENGTH            64
#define RFID_NUM_USER_DATAGRAMS     4
#define RFID_CURRENT_TAG_TIMEOUT    1000
#define RFID_MAX_SHIT_TRESHOLD      250
#define RFID_MAX_BUFFER_SIZE        1024 * 16

typedef struct {
   QByteArray epc;
   QByteArray tid;
   QByteArray rfu;
   QByteArray keys[RFID_NUM_KEYS];
   QByteArray usr[RFID_NUM_USER_DATAGRAMS];
} RFID_Tag;

typedef QList<RFID_Tag*> RFID_TagList;

#endif
