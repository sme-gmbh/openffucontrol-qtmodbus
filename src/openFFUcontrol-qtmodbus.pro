#**********************************************************************
#* openFFUcontrol-qtmodbus - a library for openFFUcontrol communication
#* Copyright (C) 2023 Smart Micro Engineering GmbH, Peter Diener
#* This program is free software: you can redistribute it and/or modify
#* it under the terms of the GNU General Public License as published by
#* the Free Software Foundation, either version 3 of the License, or
#* (at your option) any later version.
#* This program is distributed in the hope that it will be useful,
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#* GNU General Public License for more details.
#* You should have received a copy of the GNU General Public License
#* along with this program. If not, see <http://www.gnu.org/licenses/>.
#*********************************************************************/

QT       -= core
QT       += serialport testlib

CONFIG += c++11

TARGET = openffucontrol-qtmodbus
TEMPLATE = lib

OBJECTS_DIR = .obj/
MOC_DIR = .moc/
RCC_DIR = .rcc/

DEFINES += OPENFFUCONTROL_QTMODBUS_LIBRARY

SOURCES += \
    modbus.cpp \
    modbustelegram.cpp

HEADERS += \
    modbus.h \
    modbus_global.h \
    modbustelegram.h

linux-g++: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-g++-32: QMAKE_TARGET.arch = x86
linux-g++-64: QMAKE_TARGET.arch = x86_64
linux-cross: QMAKE_TARGET.arch = x86
win32-cross-32: QMAKE_TARGET.arch = x86
win32-cross: QMAKE_TARGET.arch = x86_64
win32-g++: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
win32-msvc*: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-raspi: QMAKE_TARGET.arch = armv6l
linux-armv6l: QMAKE_TARGET.arch = armv6l
linux-armv7l: QMAKE_TARGET.arch = armv7l
linux-arm*: QMAKE_TARGET.arch = armv6l
linux-aarch64*: QMAKE_TARGET.arch = aarch64

unix {
    equals(QMAKE_TARGET.arch , x86_64): {
        message("Configured for x86_64")
        target.path = /usr/lib64
    }

    equals(QMAKE_TARGET.arch , x86): {
        message("Configured for x86")
        target.path = /usr/lib
    }

    equals(QMAKE_TARGET.arch , armv6l): {
        message("Configured for armv6l")
        target.path = /usr/lib
    }

    equals(QMAKE_TARGET.arch , armv7l): {
        message("Configured for armv7l")
        target.path = /usr/lib
    }

    headers.path = "/usr/include/libopenffucontrol-qtmodbus"
    headers.files = $$PWD/*.h

    INSTALLS += target headers
}
