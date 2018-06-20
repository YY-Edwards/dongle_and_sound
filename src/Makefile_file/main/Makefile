CXX = g++
CC = gcc
CXXFLAGS = -g -Wall -O2
TARGET = dongle_app
export OBJSDIR = $(shell pwd)/objs
export ASDF = $(shell pwd)/bina
$(TARGET):$(OBJSDIR) $(ASDF)
	$(MAKE) -C src/PublicInterface/FifoQueue
	$(MAKE) -C src/PublicInterface/Hotplug
	$(CXX) -o $(ASDF)/$(TARGET) $(OBJSDIR)/*.o
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
