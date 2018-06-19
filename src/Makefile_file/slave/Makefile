#OBJS = add_int.o add_float.o
#all:$(OBJS)
#$(OBJS):%.o:%.c
#	$(CC) -c $< -o $(OBJSDIR)/$@ $(CFLAGS)

#clean:
#	$(RM) $(OBJS)
CC = gcc
CXX = g++

C_FLAGS += -lpthread -lrt -g -Wl,--as-needed -std=c++11

INC += -I $(OBJSDIR)/../src -I $(OBJSDIR)/../src/PublicInterface/SynInterface -I $(OBJSDIR)/../src -I $(OBJSDIR)/../src -I /usr/include/

LIB +=  -L/usr/local/lib/   -L/usr/lib/  

#所有的.c文件
CC_FILES = $(wildcard *.c)

CC_OBJS = $(patsubst %.c, %.o, $(CC_FILES))

#所有的.c++文件
CXX_FILES = $(wildcard *.cpp)
CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_FILES))

all:$(CC_OBJS) $(CXX_OBJS)
$(CC_OBJS):%.o:%.c
	$(CC) -c $< -o $(OBJSDIR)/$@ $(CFLAGS) $(INC) $(LIB) $(C_FLAGS)
$(CXX_OBJS):%.o:%.cpp
	$(CXX) -c $< -o $(OBJSDIR)/$@ $(CXXFLAGS) $(INC) $(LIB) $(C_FLAGS)
clean:
	$(RM) $(CC_OBJS)
	$(RM) $(CXX_OBJS)
