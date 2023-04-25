# ModBus library for openFFUcontrol
This is the ModBus library used in the open source filter fan unit control system openFFUcontrol.
It is used as a comunication driver to fans and control units (OCUs). The
library is based on Qt5 and interfaces non-blocking with signals and slots.

## Building and installing
First make sure to have Qt5 installed on your system.
Create a directory for the build
```
mkdir bin
```

Create the Makefile
```
cd bin
qmake ../src
```

Compile the library wih a number of concurrent jobs of your choice

```
make -j 8
```

Install the library as root user
```
sudo make install
```
