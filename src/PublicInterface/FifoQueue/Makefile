#OBJS = add_int.o add_float.o
#all:$(OBJS)
#$(OBJS):%.o:%.c
#	$(CC) -c $< -o $(OBJSDIR)/$@ $(CFLAGS)

#clean:
#	$(RM) $(OBJS)
CC = gcc
CXX = g++

#需要的头文件路径
INC += \
-I $(OBJSDIR)/../src \
-I $(OBJSDIR)/../src/PublicInterface/SynInterface \
-I $(OBJSDIR)/../src/PublicInterface/Logger \
-I /usr/include/
 
CXXFLAGS += -std=c++11 -g

#所有的.c文件
CC_FILES = $(wildcard *.c)

CC_OBJS = $(patsubst %.c, %.o, $(CC_FILES))

#所有的.c++文件
CXX_FILES = $(wildcard *.cpp)
CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_FILES))

all:$(CC_OBJS) $(CXX_OBJS)
$(CC_OBJS):%.o:%.c
	$(CC) -c $< -o $(OBJSDIR)/$@  $(INC) $(CCFLAGS)
$(CXX_OBJS):%.o:%.cpp
	$(CXX) -c $< -o $(OBJSDIR)/$@  $(INC)  $(CXXFLAGS)
clean:
	$(RM) $(CC_OBJS)
	$(RM) $(CXX_OBJS)
