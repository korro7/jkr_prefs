#-----------------------------------------
# Makefile for compiling UDL for EDEM
# oleh.baran@dem-solutions.com
#-----------------------------------------

SHELL=/bin/sh
.IGNORE:

SRCDIR=.

PLATF = `uname -i`
SDIR = /opt/DEMSolutions/EDEM_2.3/src
PDIR = ../plugins

BNAME = JKR
SRCF = C$(BNAME).cpp $(BNAME).cpp CCohesionList.cpp
OBJ  = $(SRCF:.cpp=.o)
DLL  = $(BNAME)_$(PLATF).so

# common flags
CC   = g++
CFLAGS=  -O2 -I$(SDIR)/Api/ContactModels \
             -I$(SDIR)/Api/Core \
             -I$(SDIR)/Misc \

dll: $(OBJ)
	$(CC) $(CFLAGS) -fPIC -shared -o $(DLL) $(notdir $(OBJ))

# compile object files for each cpp file
.cpp.o:
	$(CC) $(CFLAGS) -fPIC -c $*.cpp

install:
	cp -f $(DLL) $(PDIR)/
	cp -f $(DLL) $(HOME)/EDEM_2.3.1/ContactModels/$(BNAME).so
	cp -f *_prefs.txt $(HOME)/EDEM_2.3.1/ContactModels/

clean:
	rm *.so *.o
