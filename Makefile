CXX = g++
CC = gcc
CXXFLAGS += -g -Wall -O2 -lpthread -lrt -Wl,--as-needed -std=c++11
TARGET = dongle_app
export OBJSDIR = $(shell pwd)/objs
export ASDF = $(shell pwd)/bina


#需要的库文件路径
LIB +=  \
-L/usr/local/lib/   \
-L/usr/lib/  

$(TARGET):$(OBJSDIR) $(ASDF)

	$(MAKE) -C src/PublicInterface/FifoQueue
	$(MAKE) -C src/PublicInterface/Logger
	$(MAKE) -C src/PublicInterface/Hotplug
	$(MAKE) -C src/PublicInterface/SocketWrap
	$(MAKE) -C src/PublicInterface/SynInterface
	
	$(MAKE) -C src/StartDongleAndAlsa/AudioAlsa
	$(MAKE) -C src/StartDongleAndAlsa/SerialDongle
	$(MAKE) -C src/StartDongleAndAlsa/SerialDongle/Usart
	$(MAKE) -C src/StartDongleAndAlsa
	
	$(MAKE) -C src
	$(CXX) -o $(ASDF)/$(TARGET) $(OBJSDIR)/*.o $(LIB) $(CXXFLAGS)
$(OBJSDIR):
	mkdir -p $@
$(ASDF):
	mkdir -p $@

clean:
	-$(RM) $(TARGET)
	-$(RM) $(foreach dir, $(OBJSDIR), $(wildcard $(dir)/*.o))
	-$(RM) -rf objs
	-$(RM) -rf bin
info:
	@echo "OBJSDIR:$(OBJSDIR)"
