#TARGET = $(notdir $(CURDIR))
TARGET = dongle_app
CXX=$(CROSS_COMPILE)g++
C_FLAGS += -lpthread -lrt -g -Wl,--as-needed -std=c++11

objs := $(patsubst %c, %o, $(shell ls *.cpp))

INC += -I /usr/src/app/dongle_app/src/  -I /usr/include/

LIB +=  -L/usr/local/lib/   -L/usr/lib/  -L/usr/src/app/dongle_app/lib/ 

#-L./lib/libusb


all: $(TARGET)

$(TARGET): main.o hotplug.o socketwrap.o syninterface.o fifoqueue.o audioalsa.o serialdongle.o startdongleandalsa.o usartwrap.o

	$(CXX) -o $@ $^ $(INC) $(LIB) $(C_FLAGS)

.cpp.o:
	$(CXX) -c $(INC) $(C_FLAGS) $*.cpp -o $*.o 

.c.o:
	$(CC) -c $(INC) $(C_FLAGS) $*.c -o $*.o 

.PHONY : clean
clean:
	rm -f *.o $(TARGET)







