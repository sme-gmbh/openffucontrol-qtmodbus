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

#include "modbustelegram.h"

ModBusTelegram::ModBusTelegram()
{
    static quint64 id = 1;  // Start counting telegram id with 1. 0 is reserved for error
    this->m_id = id;
    id++;
    if (id == 0)            // Wrap around and avoid 0 - if that might ever happen with quint64... - just to be correct.
        id = 1;

    repeatCount = 1;
}

ModBusTelegram::ModBusTelegram(quint8 slaveAddress, quint8 functionCode, QByteArray data)
{
    static quint64 id = 1;  // Start counting telegram id with 1. 0 is reserved for error
    this->m_id = id;
    id++;
    if (id == 0)            // Wrap around and avoid 0 - if that might ever happen with quint64... - just to be correct.
        id = 1;

    this->slaveAddress = slaveAddress;
    this->functionCode = functionCode;
    this->data = data;
    repeatCount = 1;
}

bool ModBusTelegram::needsAnswer()
{
    if (this->slaveAddress == 0)
        return false;
    else
        return true;
}

quint64 ModBusTelegram::getID()
{
    return m_id;
}
