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

#ifndef OPENFFUCONTROLMODBUS_H
#define OPENFFUCONTROLMODBUS_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QStringList>
#include <QTimer>
#include <QList>
#include <QMutex>

#include "modbus_global.h"
#include "modbustelegram.h"

class MODBUSSHARED_EXPORT ModBus : public QObject
{
    Q_OBJECT
public:
    explicit ModBus(QObject *parent, QString interface, bool debug = false);
    ~ModBus();

    bool open(qint32 baudrate = QSerialPort::Baud9600);
    void close();

    void setDelayTxTimer(quint32 milliseconds);

    // High level access
    quint64 sendRawRequest(quint8 slaveAddress, quint8 functionCode, QByteArray payload);
    QByteArray sendRawRequestBlocking(quint8 slaveAddress, quint8 functionCode, QByteArray payload);

    quint64 readCoils(quint8 slaveAddress, quint16 dataStartAddress, quint16 count = 1, quint8 functionCode = 0x01);
    quint64 readDiscreteInputs(quint8 slaveAddress, quint16 dataStartAddress, quint16 count = 1, quint8 functionCode = 0x02);
    quint64 readHoldingRegisters(quint8 slaveAddress, quint16 dataStartAddress, quint8 count = 1, quint8 functionCode = 0x03);
    quint64 readInputRegisters(quint8 slaveAddress, quint16 dataStartAddress, quint8 count = 1, quint8 functionCode = 0x04);

    quint64 writeSingleCoil(quint8 slaveAddress, quint16 dataAddress, bool on, quint8 functionCode = 0x05);
    quint64 writeSingleRegister(quint8 slaveAddress, quint16 dataAddress, quint16 data, quint8 functionCode = 0x06);

    quint64 readExceptionStatus(quint8 slaveAddress, quint8 functionCode = 0x07);
    quint64 readDiagnosticCounter(quint8 slaveAddress, quint8 subFunctionCode, QByteArray data, quint8 functionCode = 0x08);

    quint64 getCommEventCounter(quint8 slaveAddress, quint8 functionCode = 0x0b);
    quint64 getCommEventLog(quint8 slaveAddress, quint8 functionCode = 0x0c);

    quint64 writeMultipleCoils(quint8 slaveAddress, quint16 dataStartAddress, QList<bool> on, quint8 functionCode = 0x0f);
    quint64 writeMultipleRegisters(quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data, quint8 functionCode = 0x10);

    quint64 reportSlaveID(quint8 slaveAddress, quint8 functionCode = 0x11);

//    quint64 readFileRecord(quint8 slaveAddress, quint8 functionCode = 0x14);
//    quint64 writeFileRecord(quint8 slaveAddress, quint8 functionCode = 0x15);

    quint64 maskWriteRegister(quint8 slaveAddress, quint16 dataAddress, quint16 andMask, quint16 orMask, quint8 functionCode = 0x16);
//    quint64 readWriteMultipleRegisters(quint8 slaveAddress, quint8 functionCode = 0x17);
    quint64 readFIFOqueue(quint8 slaveAddress, quint16 fifoPointerAddress, quint8 functionCode = 0x18);
//    quint64 encapsulatedInterfaceTransport(quint8 slaveAddress, quint8 functionCode = 0x2b);

//    quint64 canOpenGeneralReferenceRequestAndResponsePDU(quint8 slaveAddress, quint8 functionCode = 0x2b);
//    quint64 readDeviceIdentification(quint8 slaveAddress, quint8 functionCode = 0x2b);

    int getSizeOfTelegramQueue(bool highPriorityQueue = false);
    void clearTelegramQueue(bool highPriorityQueue = false);

    // Low level access; writes to queue that is fed to the byte level access layer
    // Returns the assigned telegram id, which is unique
    quint64 writeTelegramToQueue(ModBusTelegram* telegram, bool highPriority = false);

    int getTelegramRepeatCount() const;
    void setTelegramRepeatCount(int telegramRepeatCount);

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
    QList<ModBusTelegram*> m_telegramQueue_standardPriority;
    QList<ModBusTelegram*> m_telegramQueue_highPriority;
    ModBusTelegram* m_currentTelegram;
    int m_telegramRepeatCount;
    quint64 m_rx_telegrams;
    quint64 m_crc_errors;

    // Low level access; writes immediately to the bus
    quint64 writeTelegramNow(ModBusTelegram* telegram);
    void writeTelegramRawNow(quint8 slaveAddress, quint8 functionCode, QByteArray data);
    void tryToParseResponseRaw(QByteArray *buffer);
    void parseResponse(quint64 telegramID, quint8 slaveAddress, quint8 functionCode, QByteArray payload);
    quint16 checksum(QByteArray data);
    bool checksumOK(QByteArray data);

signals:
    void signal_responseRawComplete(quint64 telegramID, QByteArray data);
    void signal_responseRaw(quint64 telegramID, quint8 address, quint8 functionCode, QByteArray data);
    void signal_transactionFinished();
    void signal_transactionLost(quint64 id);

    // High level response signals
    void signal_exception(quint64 telegramID, quint8 exceptionCode);

    void signal_coilsRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<bool> on);
    void signal_discreteInputsRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<bool> on);
    void signal_holdingRegistersRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data);
    void signal_inputRegistersRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data);

    void signal_exceptionStatusRead(quint64 telegramID, quint8 slaveAddress, quint16 data);
    void signal_diagnosticCounterRead(quint64 telegramID, quint8 slaveAddress, quint8 subFunctionCode, quint16 data);
    void signal_commEventCounterRead(quint64 telegramID, quint8 slaveAddress, quint16 data);
    void signal_commEventLogRead(quint64 telegramID, quint8 slaveAddress, QList<quint16> data);

    void signal_slaveIdRead(quint64 telegramID, quint8 slaveAddress, quint8 data);

public slots:

private slots:
    void slot_tryToSendNextTelegram();
    void slot_readyRead();
    void slot_requestTimer_fired();
    void slot_rxIdleTimer_fired();

};

#endif // OPENFFUCONTROLMODBUS_H
