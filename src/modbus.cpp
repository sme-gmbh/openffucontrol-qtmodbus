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

#include <QSignalSpy>

#include "modbus.h"

ModBus::ModBus(QObject *parent, QString interface, bool debug) : QObject(parent)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::ModBus().\n");
        fflush(stdout);
    }
    m_interface = interface;
    m_debug = debug;
    m_port = new QSerialPort(interface, this);
    m_transactionPending = false;
    m_currentTelegram = NULL;
    m_rx_telegrams = 0;
    m_crc_errors = 0;

    m_requestedCount = 0;
    m_requestedDataAddress = 0;
    m_requestedDataStartAddress = 0;

    // This timer notifies about a telegram timeout if a unit does not answer
    m_requestTimer.setSingleShot(true);
    m_requestTimer.setInterval(5000);  // was 200
    connect(&m_requestTimer, SIGNAL(timeout()), this, SLOT(slot_requestTimer_fired()));

    // This timer delays tx after rx to wait for line clearance
    m_delayTxTimer.setSingleShot(true);
    m_delayTxTimer.setInterval(4);  // was 4
    connect(this, SIGNAL(signal_transactionFinished()), &m_delayTxTimer, SLOT(start()));
    connect(&m_delayTxTimer, SIGNAL(timeout()), this, SLOT(slot_tryToSendNextTelegram()));

    // This timer fires if receiver does not get any more bytes and telegram should be complete
    m_rxIdleTimer.setSingleShot(true);
    m_rxIdleTimer.setInterval(100); // was 100
    connect(&m_rxIdleTimer, SIGNAL(timeout()), this, SLOT(slot_rxIdleTimer_fired()));
}

ModBus::~ModBus()
{
    if (m_port->isOpen())
        this->close();
    delete m_port;

    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::~ModBus().\n");
        fflush(stdout);
    }
}

bool ModBus::open(qint32 baudrate)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::open().\n");
        fflush(stdout);
    }

    m_port->setBaudRate(baudrate);

    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::TwoStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);
    connect(m_port, SIGNAL(readyRead()), this, SLOT(slot_readyRead()));
    bool openOK = m_port->open(QIODevice::ReadWrite);
    m_port->setBreakEnabled(false);
    m_port->setTextModeEnabled(false);

//    QThread::msleep(10000);

    return openOK;
}

void ModBus::close()
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::close().\n");
        fflush(stdout);
    }
    if (m_port->isOpen())
        m_port->close();
}

quint64 ModBus::sendRawRequest(quint8 slaveAddress, quint8 functionCode, QByteArray payload)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::sendRawRequest(). +++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        fflush(stdout);
    }
    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

QByteArray ModBus::sendRawRequestBlocking(quint8 slaveAddress, quint8 functionCode, QByteArray payload)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::sendRawRequestBlocking(). +++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        fflush(stdout);
    }
    QSignalSpy spy(this, SIGNAL(signal_responseRawComplete(quint64, QByteArray)));

    writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
    QByteArray response;
    if (spy.wait(10000))
    {
        QList<QVariant> arguments = spy.takeFirst();
        response = arguments.at(1).toByteArray();
    }

    return response;
}

quint64 ModBus::readCoils(quint8 slaveAddress, quint16 dataStartAddress, quint16 count, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readCoils().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(dataStartAddress >> 8);
    payload += (unsigned char)(dataStartAddress & 0xff);
    payload += (unsigned char)(count >> 8);
    payload += (unsigned char)(count & 0xff);

    m_requestedCount = count;
    m_requestedDataStartAddress = dataStartAddress;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::readDiscreteInputs(quint8 slaveAddress, quint16 dataStartAddress, quint16 count, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readDiscreteInputs().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(dataStartAddress >> 8);
    payload += (unsigned char)(dataStartAddress & 0xff);
    payload += (unsigned char)(count >> 8);
    payload += (unsigned char)(count & 0xff);

    m_requestedCount = count;
    m_requestedDataStartAddress = dataStartAddress;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::readHoldingRegisters(quint8 slaveAddress, quint16 dataStartAddress, quint8 count, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readHoldingRegisters().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(dataStartAddress >> 8);
    payload += (unsigned char)(dataStartAddress & 0xff);
    payload += (unsigned char)(count >> 8);
    payload += (unsigned char)(count & 0xff);

    m_requestedCount = count;
    m_requestedDataStartAddress = dataStartAddress;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::readInputRegisters(quint8 slaveAddress, quint16 dataStartAddress, quint8 count, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readInputRegisters().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(dataStartAddress >> 8);
    payload += (unsigned char)(dataStartAddress & 0xff);
    payload += (unsigned char)(count >> 8);
    payload += (unsigned char)(count & 0xff);

    m_requestedCount = count;
    m_requestedDataStartAddress = dataStartAddress;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::writeSingleCoil(quint8 slaveAddress, quint16 dataAddress, bool on, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeSingleCoil().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(dataAddress >> 8);
    payload += (unsigned char)(dataAddress & 0xff);
    payload += (unsigned char)(on ? 0xff : 0x00);
    payload += (unsigned char)0x00;

    m_requestedDataAddress = dataAddress;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::writeSingleRegister(quint8 slaveAddress, quint16 dataAddress, quint16 data, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeSingleRegister().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(dataAddress >> 8);
    payload += (unsigned char)(dataAddress & 0xff);
    payload += (unsigned char)(data >> 8);
    payload += (unsigned char)(data & 0xff);

    m_requestedDataAddress = dataAddress;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::readExceptionStatus(quint8 slaveAddress, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readExceptionStatus().\n");
        fflush(stdout);
    }

    QByteArray payload;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::readDiagnosticCounter(quint8 slaveAddress, quint8 subFunctionCode, QByteArray data, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readDiagnosticCounter().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(subFunctionCode >> 8);
    payload += (unsigned char)(subFunctionCode & 0xff);
    payload += data;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::getCommEventCounter(quint8 slaveAddress, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::getCommEventCounter().\n");
        fflush(stdout);
    }

    QByteArray payload;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::getCommEventLog(quint8 slaveAddress, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::getCommEventLog().\n");
        fflush(stdout);
    }

    QByteArray payload;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::writeMultipleCoils(quint8 slaveAddress, quint16 dataStartAddress, QList<bool> on, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeMultipleCoils().\n");
        fflush(stdout);
    }

    QByteArray payload;

    quint16 count = on.count();
    m_requestedCount = count;
    m_requestedDataStartAddress = dataStartAddress;

    payload += (unsigned char)(dataStartAddress >> 8);
    payload += (unsigned char)(dataStartAddress & 0xff);
    payload += (unsigned char)(count >> 8);
    payload += (unsigned char)(count & 0xff);
    unsigned char bytes = count / 8;
    if ((count % 8) != 0) bytes += 1;
    payload += bytes;

    quint8 bit = 0;
    quint8 byte = 0;

    while (!on.isEmpty())
    {
        if (on.at(0))
            byte += 1 << bit;

        on.removeFirst();

        bit++;
        if (bit == 8)
        {
            bit = 0;
            payload += byte;
            byte = 0;
        }
    }

    if (bit != 0)
        payload += byte;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::writeMultipleRegisters(quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeMultipleRegisters().\n");
        fflush(stdout);
    }

    QByteArray payload;

    quint16 count = data.count();
    m_requestedCount = count;
    m_requestedDataStartAddress = dataStartAddress;

    payload += (unsigned char)(dataStartAddress >> 8);
    payload += (unsigned char)(dataStartAddress & 0xff);
    payload += (unsigned char)(count >> 8);
    payload += (unsigned char)(count & 0xff);
    unsigned char bytes = count * 2;
    payload += bytes;

    while (!data.isEmpty())
    {
        quint16 word = data.takeFirst();
        payload += (unsigned char)(word >> 8);
        payload += (unsigned char)(word & 0xff);
    }

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::reportSlaveID(quint8 slaveAddress, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::reportSlaveID().\n");
        fflush(stdout);
    }

    QByteArray payload;

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::maskWriteRegister(quint8 slaveAddress, quint16 dataAddress, quint16 andMask, quint16 orMask, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::maskWriteRegister().\n");
        fflush(stdout);
    }

    QByteArray payload;

    m_requestedDataAddress = dataAddress;

    payload += (unsigned char)(dataAddress >> 8);
    payload += (unsigned char)(dataAddress & 0xff);
    payload += (unsigned char)(andMask >> 8);
    payload += (unsigned char)(andMask & 0xff);
    payload += (unsigned char)(orMask >> 8);
    payload += (unsigned char)(orMask & 0xff);

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}

quint64 ModBus::readFIFOqueue(quint8 slaveAddress, quint16 fifoPointerAddress, quint8 functionCode)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::readFIFOqueue().\n");
        fflush(stdout);
    }

    QByteArray payload;

    payload += (unsigned char)(fifoPointerAddress >> 8);
    payload += (unsigned char)(fifoPointerAddress & 0xff);

    return writeTelegramToQueue(new ModBusTelegram(slaveAddress, functionCode, payload));
}


void ModBus::slot_tryToSendNextTelegram()
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::slot_tryToSendNextTelegram().\n");
        fflush(stdout);
    }
    m_telegramQueueMutex.lock();
    // Delete last telegram if it exists
    // If repeat counter is not zero, then repeat current telegram, otherwise take new
    // telegram from the queue
    if ((m_currentTelegram != NULL) && (m_currentTelegram->repeatCount == 0))
    {
        if (m_debug)
        {
            fprintf(stdout, "DEBUG ModBus::slot_tryToSendNextTelegram: Deleting Telegram.\n");
            fflush(stdout);
        }
        delete m_currentTelegram;
        m_currentTelegram = NULL;
    }

    if (m_currentTelegram == NULL)
    {
        if (m_telegramQueue.isEmpty())
        {
            if (m_debug)
            {
                fprintf(stdout, "DEBUG ModBus::slot_tryToSendNextTelegram: telegramQueue empty.\n");
                fflush(stdout);
            }
            m_transactionPending = false;
            m_telegramQueueMutex.unlock();
            return;
        }
        if (m_debug)
        {
            fprintf(stdout, "DEBUG ModBus::slot_tryToSendNextTelegram: Fetching new telegram from queue.\n");
            fflush(stdout);
        }
        m_currentTelegram = m_telegramQueue.takeFirst();
    }

    m_transactionPending = true;
    m_requestTimer.start();
    m_telegramQueueMutex.unlock();

    writeTelegramNow(m_currentTelegram);
}

quint64 ModBus::writeTelegramToQueue(ModBusTelegram *telegram)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeTelegramToQueue().\n");
        fflush(stdout);
    }
    quint64 telegramID = telegram->getID();
    m_telegramQueueMutex.lock();
    m_telegramQueue.append(telegram);

//    if (!m_requestTimer.isActive()) // If we inserted the first packet, we have to start the sending process
    if (!m_transactionPending) // If we inserted the first packet, we have to start the sending process
    {
        if (m_debug)
        {
            fprintf(stdout, "DEBUG ModBus::writeTelegramToQueue(): Actively starting send queue.\n");
            fflush(stdout);
        }
        m_telegramQueueMutex.unlock();
        slot_tryToSendNextTelegram();
    }
    else
    {
        if (m_debug)
        {
            fprintf(stdout, "DEBUG ModBus::writeTelegramToQueue(): Appended telegram to already running queue.\n");
            fflush(stdout);
        }
        m_telegramQueueMutex.unlock();
    }

    return telegramID;
}

quint64 ModBus::rx_telegrams() const
{
    return m_rx_telegrams;
}

quint64 ModBus::crc_errors() const
{
    return m_crc_errors;
}

QString ModBus::exceptionToText(quint8 exceptionCode)
{
    switch (exceptionCode)
    {
        case 0x01: return QString("E_ILLEGAL_FUNCTION");
        break;
        case 0x02: return QString("E_ILLEGAL_DATA_ADDRESS");
        break;
        case 0x03: return QString("E_ILLEGAL_DATA_VALUE");
        break;
        case 0x04: return QString("E_SERVER_DEVICE_FAILURE");
        break;
        case 0x05: return QString("E_ACKNOWLEDGE");
        break;
        case 0x06: return QString("E_SERVER_DEVICE_BUSY");
        break;
        case 0x08: return QString("E_MEMORY_PARITY_ERROR");
        break;
        case 0x0a: return QString("E_GATEWAY_PATH_UNAVAILABLE");
        break;
        case 0x0b: return QString("E_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND");
        break;
        default: return QString("E_UNKNOWN");
    }
}

quint64 ModBus::writeTelegramNow(ModBusTelegram *telegram)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeTelegramNow().\n");
        fflush(stdout);
    }
    quint8 slaveAddress = telegram->slaveAddress;
    quint8 functionCode = telegram->functionCode;
    QByteArray data = telegram->data;
    telegram->repeatCount--;

    writeTelegramRawNow(slaveAddress, functionCode, data);
    return telegram->getID();
}

void ModBus::writeTelegramRawNow(quint8 slaveAddress, quint8 functionCode, QByteArray data)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::writeTelegramRawNow().\n");
        fflush(stdout);
    }
    QByteArray out;

    out.append(slaveAddress);
    out.append(functionCode);
    out.append(data);

    quint16 cs = checksum(out);
    out.append(cs & 0xFF);
    out.append(cs >> 8);

    if (m_port->isOpen())
    {
        if (m_debug)
        {
            fprintf(stdout, "ModBus::writeTelegramRawNow: Writing: %s\n", out.toHex().data());
            fflush(stdout);
        }
        m_port->write(out);
        m_port->flush();
    }
}

void ModBus::tryToParseResponseRaw(QByteArray *buffer)
{
    if (buffer->size() < 4)
        return;

    if (m_debug)
    {
        fprintf(stdout, "ModBus::tryToParseResponseRaw: Reading: %s\n", buffer->toHex().data());
        fflush(stdout);
    }

    if (m_currentTelegram == nullptr)
    {
        if (m_debug)
        {
            fprintf(stdout, "ModBus::tryToParseResponseRaw: Response does not belong to a request. Parse abort.\n");
            fflush(stdout);
        }
        buffer->clear();
        return;
    }

    quint8 address = buffer->at(0);
    quint8 functionCode = buffer->at(1) & 0x7F;
    bool exception = buffer->at(1) & 0x80;

    // Check address match here in case we are Modbus server
    // if(server)
    // {...}

    if (exception)
    {
        if (buffer->size() < 5)
        {
            if (m_debug)
            {
                fprintf(stdout, "ModBus::tryToParseResponseRaw: Buffer size below 5 byte: %s\n", buffer->toHex().data());
                fflush(stdout);
            }
            return;
        }

        quint8 exceptionCode = buffer->at(2);
        if (!checksumOK(*buffer))
        {
            buffer->clear();
            m_crc_errors++;
            if (m_debug)
            {
                fprintf(stdout, "ModBus::tryToParseResponseRaw: Received exception and have CRC error.\n");
                fflush(stdout);
            }
            return;
        }

        m_requestTimer.stop();
        m_rx_telegrams++;

        if (m_debug)
        {
            fprintf(stdout, "ModBus::tryToParseResponseRaw: Got exception, sending it upstream.\n");
            fflush(stdout);
        }

        // Parse exception here and send signal!
        emit signal_exception(m_currentTelegram->getID(), exceptionCode);
        emit signal_responseRawComplete(m_currentTelegram->getID(), *buffer);
        emit signal_transactionFinished();
        buffer->clear();
        return;
    }

    // Try to check length here (todo!)
    if (buffer->length() > 255)
    {
        buffer->clear();
        if (m_debug)
        {
            fprintf(stdout, "ModBus::tryToParseResponseRaw: Buffer overflow error.\n");
            fflush(stdout);
        }
        return;
    }

    if (!checksumOK(*buffer))
    {
        buffer->clear();
        m_crc_errors++;
        if (m_debug)
        {
            fprintf(stdout, "ModBus::tryToParseResponseRaw: CRC error.\n");
            fflush(stdout);
        }
        return;
    }

    // If we reach this code, checksum is ok

    m_requestTimer.stop();
    m_rx_telegrams++;
    QByteArray data = buffer->mid(2, buffer->length() - 4); // Fill data with PDU

    if (m_debug)
    {
        fprintf(stdout, "ModBus::tryToParseResponseRaw: Ok, sending data upstream.\n");
        fflush(stdout);
    }

    emit signal_responseRawComplete(m_currentTelegram->getID(), *buffer);
    emit signal_responseRaw(m_currentTelegram->getID(), address, functionCode, data);
    parseResponse(m_currentTelegram->getID(), address, functionCode, data);
    emit signal_transactionFinished();

    buffer->clear();
}

void ModBus::parseResponse(quint64 telegramID, quint8 slaveAddress, quint8 functionCode, QByteArray payload)
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::parseResponse() fc%i.\n", functionCode);
        fflush(stdout);
    }

    switch (functionCode)
    {
    case 1:
    case 2:
    {
        quint8 bytes;
        quint16 dataStartAddress = m_requestedDataStartAddress;
        QList<bool> on;

        if (payload.length() < 1)
        {
            fprintf(stdout, "DEBUG ModBus::parseResponse: fc%i data length < 1.\n", functionCode);
            fflush(stdout);
            break;
        }

        bytes = payload.at(0);

        if (payload.length() != (bytes + 1))
        {
            fprintf(stdout, "DEBUG ModBus::parseResponse: fc%i data length != bytecount + 1.\n", functionCode);
            fflush(stdout);
            break;
        }

        quint16 bit = 0;
        quint8 byte = 0;

        while (payload.length() > (byte + 1))
        {
            if (payload.at(byte + 1) & (1 << bit))
                on.append(true);
            else
                on.append(false);

            if (on.length() == m_requestedCount)
                break;

            bit++;
            if (bit == 8)
            {
                bit = 0;
                byte++;
            }
        }

        if (functionCode == 1)
            emit signal_coilsRead(telegramID, slaveAddress, dataStartAddress, on);
        else if (functionCode == 2)
            emit signal_discreteInputsRead(telegramID, slaveAddress, dataStartAddress, on);
        break;
    }
    case 3:
    case 4:
    {
        quint8 bytes;
        quint16 dataStartAddress = m_requestedDataStartAddress;
        QList<quint16> data;

        if (payload.length() < 1)
        {
            fprintf(stdout, "DEBUG ModBus::parseResponse: fc%i data length < 1.\n", functionCode);
            fflush(stdout);
            break;
        }

        bytes = payload.at(0);

        if (payload.length() != (bytes + 1))
        {
            fprintf(stdout, "DEBUG ModBus::parseResponse: fc%i data length != bytecount + 1.\n", functionCode);
            fflush(stdout);
            break;
        }

        if (payload.length() != (m_requestedCount * 2 + 1))
        {
            fprintf(stdout, "DEBUG ModBus::parseResponse: fc%i requested length mismatch with resonse length.\n", functionCode);
            fflush(stdout);
            break;
        }

        for (quint16 i = 0; i < m_requestedCount; i++)
        {
            quint16 word = 0;
            word += payload.at(i*2 + 1) << 8;
            word += payload.at(i*2 + 2);
            data.append(word);
        }

        if (functionCode == 3)
            emit signal_holdingRegistersRead(telegramID, slaveAddress, dataStartAddress, data);
        else if (functionCode == 4)
            emit signal_inputRegistersRead(telegramID, slaveAddress, dataStartAddress, data);
        break;
    }
    case 5: // Single coil written
        break;
    case 6: // Single holding register written
        break;
    case 7:
        break;
    case 8:
        break;
    case 9:
        break;
    case 10:
        break;
    case 11:
        break;
    case 12:
        break;
    case 13:
        break;
    case 14:
        break;
    case 15: // Multiple coils written
        break;
    case 16: // Multiple holding registers written
        break;
    default:
        break;

        // Todo: Implementation of the following signals
//        emit signal_exceptionStatusRead(quint64 telegramID, quint8 slaveAddress, quint16 data);
//        emit signal_diagnosticCounterRead(quint64 telegramID, quint8 slaveAddress, quint8 subFunctionCode, quint16 data);
//        emit signal_commEventCounterRead(quint64 telegramID, quint8 slaveAddress, quint16 data);
//        emit signal_commEventLogRead(quint64 telegramID, quint8 slaveAddress, QList<quint16> data);
//        emit signal_slaveIdRead(quint64 telegramID, quint8 slaveAddress, quint8 data);

    }
}

quint16 ModBus::checksum(QByteArray data)
{
    quint16 crc = 0xffff;
    quint16 i;

    for (i=0;i < data.length(); i++)
    {
        const uint16_t polynom = 0xA001;

        bool c;
        uint8_t crc_hi, crc_low;

        crc_hi = crc >> 8;
        crc_low = (crc & 0xFF) ^ (uint8_t)data.at(i);
        crc = (crc_hi << 8) | crc_low;

        for (quint8 j=0; j<=7; j++)
        {
            c = (crc & 0x001);
            crc = (crc >> 1);
            if (c) crc ^= polynom;
        }
    }

    return crc;
}

bool ModBus::checksumOK(QByteArray data)
{
    quint16 crc = 0;
    crc = (uint8_t)data.at(data.length() - 2);
    crc |= ((uint8_t)data.at(data.length() - 1)) << 8;

    data.remove(data.length() - 2, 2);

    quint16 crc_calculated = checksum(data);

    if ((crc_calculated != crc) && m_debug)
    {
        fprintf(stdout, "ModBus::checksumOK: Read crc:       %#06x\nModBus::checksumOK: Calculated crc: %#06x\n", crc, crc_calculated);
        fflush(stdout);
    }

    return (crc_calculated == crc);
}

void ModBus::slot_readyRead()
{
    while (!m_port->atEnd())
    {
        char c;
        m_port->getChar(&c);
        m_readBuffer.append(c);
        m_rxIdleTimer.start();  // Start resets the timer even if it has not finished in order to run the full time again
    }
}

void ModBus::slot_requestTimer_fired()
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::slot_requestTimer_fired().\n");
        fflush(stdout);
    }
    if (m_currentTelegram->needsAnswer())
    {
        emit signal_transactionLost(m_currentTelegram->getID());
    }
//    else
//    {
        emit signal_transactionFinished();
//    }
}

void ModBus::slot_rxIdleTimer_fired()
{
    if (m_debug)
    {
        fprintf(stdout, "DEBUG ModBus::slot_rxIdleTimer_fired().\n");
        fflush(stdout);
    }
    tryToParseResponseRaw(&m_readBuffer);
}
