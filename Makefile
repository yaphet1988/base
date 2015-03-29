########################################################################
# eg. make MODE=STATIC/DYNAMIC
# STATIC---.a   DYNAMIC---.so
########################################################################
CC = g++
INC = -I tinyxml
CCFLAG = -Wall -O2 -Wno-deprecated

OUT_LIB_DIR = output/lib
OUT_INC_DIR = output/include
OUT_OBJ_DIR = output/obj

ifeq ($(MODE),DYNAMIC)
	TARGET = $(OUT_LIB_DIR)/libbase.so
	AR_ = g++ -fPIC -shared -o
	FLAG = $(CCFLAG) -fPIC
else
	TARGET = $(OUT_LIB_DIR)/libbase.a
	AR_ = ar rcs
	FLAG = $(CCFLAG)
endif

all:$(TARGET)
OBJS = $(patsubst %.cpp, $(OUT_OBJ_DIR)/%.o, $(wildcard *.cpp))

$(OUT_OBJ_DIR)/%.o : %.cpp %.h
	$(CC) -c $(FLAG) $< -o $@ $(INC)

OBJS_TINY = ./tinyxml/*.o

$(TARGET) : $(OBJS) $(OBJS_TINY) $(LIB)
	$(AR_) $(TARGET) $(OBJS_TINY) $(OBJS)
	cp *.h $(OUT_INC_DIR)
#	cp ilink.h $(OUT_INC_DIR)
#	cp ilinkhandler.h $(OUT_INC_DIR)
#	cp tcplink.h $(OUT_INC_DIR)
#	cp udplink.h $(OUT_INC_DIR)
#	cp env.h $(OUT_INC_DIR)
#	cp uni.h $(OUT_INC_DIR)
#	cp logger.h $(OUT_INC_DIR)
#	cp encrypt.h $(OUT_INC_DIR)
#	cp fdtimer.h $(OUT_INC_DIR)
#	cp xmlloader.h $(OUT_INC_DIR)
#	cp packet.h $(OUT_HELP_DIR)
#	cp singleton.h $(OUT_HELP_DIR)
#	cp macroformat.h $(OUT_HELP_DIR)
#	cp consistenhash.h $(OUT_HELP_DIR)

xml:
	cd tinyxml; make clean; make

.PHONY: clean

clean:
	rm $(OBJS)
	rm $(TARGET)
	rm $(OUT_INC_DIR)/*.h
