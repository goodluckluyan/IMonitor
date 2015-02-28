

CC=gcc
CXX=g++
INC_PATH = src
CFLAGS += -g -MMD 
CFLAGS +=  -I$(INC_PATH) -I/usr/include/mysql -I/usr/include/sqlpg/ -I/usr/local/xerces-c-3.1.1-x86-linux-gcc-3.4/include
LDFLAGS +=  -lpthread  -L/usr/lib/mysql -lmysqlclient -lgsoap++ -L/usr/local/xerces-c-3.1.1-x86-linux-gcc-3.4/lib  -lxerces-c
TARGET= ./bin/imonitor
OUTPUT_PATH = ./obj


SUBDIR =database log para contentcheck jobs threadManage timeTask utility webservice .

#…Ë÷√VPATH
EMPTY = 
SPACE = $(EMPTY)$(EMPTY) 
VPATH = $(subst $(SPACE), : ,$(strip $(foreach n,$(SUBDIR), $(INC_PATH)/$(n)))) : $(OUTPUT_PATH)

CXX_SOURCES = $(notdir $(foreach n, $(SUBDIR), $(wildcard $(INC_PATH)/$(n)/*.cpp)))
CXX_OBJECTS = $(patsubst  %.cpp,  %.o, $(CXX_SOURCES))
DEP_FILES = $(patsubst  %.cpp,  $(OUTPUT_PATH)/%.d, $(CXX_SOURCES))
	
$(TARGET):$(CXX_OBJECTS) 
	$(CXX) $(LDFLAGS) -o $@ $(foreach n, $(CXX_OBJECTS), $(OUTPUT_PATH)/$(n))
	#******************************************************************************#
	#                          Bulid successful !                                  #
	#******************************************************************************#

%.o:%.cpp
	$(CXX) -c $(CFLAGS) -MT $@ -MF $(OUTPUT_PATH)/$(notdir $(patsubst  %.cpp, %.d,  $<)) -o $(OUTPUT_PATH)/$@ $< 
	
-include $(DEP_FILES)


test:
	@echo $(VPATH)
	
mkdir:
	mkdir -p $(dir $(TARGET))
	mkdir -p $(OUTPUT_PATH)
	
rmdir:
	rm -rf $(dir $(TARGET))
	rm -rf $(OUTPUT_PATH)

clean:
	rm -f $(OUTPUT_PATH)/*
