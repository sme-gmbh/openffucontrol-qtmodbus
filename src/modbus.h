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

#ifndef MODBUS_H
#define MODBUS_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QStringList>
#include <QTimer>
#include <QList>
#include <QMutex>

#include "modbustelegram.h"

class ModBus : public QObject
{
    Q_OBJECT
public:
    explicit ModBus(QObject *parent, QString interface, bool debug = false);
    ~ModBus();

    bool open(qint32 baudrate = QSerialPort::Baud9600);
    void close();

    // High level access
    quint64 sendRawRequest(quint8 slaveAddress, quint8 functionCode, QByteArray payload);
    QByteArray sendRawRequestBlocking(quint8 slaveAddress, quint8 functionCode, QByteArray payload);
    quint64 readRegisters(quint8 slaveAddress, quint16 dataStartAddress, quint8 count);
    quint64 writeRegisters(quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data);
    quint16 readRegister(quint8 slaveAddress, quint16 dataAddress);
    quint16 writeRegister(quint8 slaveAddress, quint16 dataAddress, quint16 data);
    quint64 readDiscreteInput(quint8 slaveAddress, quint16 dataAddress);
    quint64 writeCoil(quint8 slaveAddress, quint16 dataAddress, quint8 bit, bool data);

    // Low level access; writes to queue that is fed to the byte level access layer
    // Returns the assigned telegram id, which is unique
    quint64 writeTelegramToQueue(ModBusTelegram* telegram);

    quint64 rx_telegrams() const;
    quint64 crc_errors() const;

    QString exceptionToText(quint8 exceptionCode);
private:
    QString m_interface;
    bool m_debug;
    QSerialPort* m_port;
    QByteArray m_readBuffer;
    QTimer m_requestTimer;  // This timer controlles timeout of telegrams with answer and sending timeslots for telegrams without answer
    QTimer m_delayTxTimer;  // This timer delays switching to rs-485 tx after rs-485 rx (line clearance time)
    QTimer m_rxIdleTimer;   // This timer fires if receiver does not get any more bytes and telegram should be complete

    bool m_transactionPending;
    QMutex m_telegramQueueMutex;
    QList<ModBusTelegram*> m_telegramQueue;
    ModBusTelegram* m_currentTelegram;
    quint64 m_rx_telegrams;
    quint64 m_crc_errors;

    // Low level access; writes immediately to the bus
    quint64 writeTelegramNow(ModBusTelegram* telegram);
    void writeTelegramRawNow(quint8 slaveAddress, quint8 functionCode, QByteArray data);
    void tryToParseResponseRaw(QByteArray *buffer);
    void parseResponse(quint64 id, quint8 slaveAddress, quint8 functionCode, QByteArray data);
    quint16 checksum(QByteArray data);
    bool checksumOK(QByteArray data);

signals:
    void signal_responseRawComplete(quint64 telegramID, QByteArray data);
    void signal_responseRaw(quint64 telegramID, quint8 address, quint8 functionCode, QByteArray data);
    void signal_transactionFinished();
    void signal_transactionLost(quint64 id);

    // High level response signals
    void signal_exception(quint64 telegramID, quint8 exceptionCode);
    void signal_registersRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data);
    // Todo: Implement more high level response signals

public slots:

private slots:
    void slot_tryToSendNextTelegram();
    void slot_readyRead();
    void slot_requestTimer_fired();
    void slot_rxIdleTimer_fired();

};

#endif // MODBUS_H
