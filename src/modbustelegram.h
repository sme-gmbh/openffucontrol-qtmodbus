/**********************************************************************
** openFFUcontrol-qtmodbus - a library for openFFUcontrol communication
** Copyright (C) 2023 Smart Micro Engineering GmbH, Peter Diener
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
** You should have received a copy of the GNU General Public License
** along with this program. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/

#ifndef OPENFFUCONTROLMODBUSTELEGRAM_H
#define OPENFFUCONTROLMODBUSTELEGRAM_H

#include <QByteArray>

class ModBusTelegram
{
public:
    ModBusTelegram();
    ModBusTelegram(quint8 slaveAddress, quint8 functionCode, QByteArray data);

    typedef enum {
        E_ILLEGAL_FUNCTION = 0x01,
        E_ILLEGAL_DATA_ADDRESS = 0x02,
        E_ILLEGAL_DATA_VALUE = 0x03,
        E_SERVER_DEVICE_FAILURE = 0x04,
        E_ACKNOWLEDGE = 0x05,
        E_SERVER_DEVICE_BUSY = 0x06,
        E_MEMORY_PARITY_ERROR = 0x08,
        E_GATEWAY_PATH_UNAVAILABLE = 0x0a,
        E_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND = 0x0b
    } ExeptionCode;

    quint8 slaveAddress;
    quint8 functionCode;
    quint16 requestedDataStartAddress;
    quint16 requestedCount;
    QByteArray data;

    int repeatCount;    // Set to different value if that telegram is important and should be autorepeated

    bool needsAnswer();

    quint64 getID();

private:
    quint64 m_id; // Telegram id is unique accross all telegrams per bus
};

#endif // OPENFFUCONTROLMODBUSTELEGRAM_H
