INC = -I ./
CC = g++
CCFLAG = -Wextra -g -O2 -Wno-deprecated
TARGET = libtinyxml.a
AR_ = ar rcs
FLAG = $(CCFLAG)

all:$(TARGET)
OBJECTS = tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o

tinystr.o : tinystr.cpp tinystr.h
	$(CC) -c $(FLAG) tinystr.cpp -o tinystr.o $(INC)

tinyxml.o : tinyxml.cpp tinyxml.h
	$(CC) -c $(FLAG) tinyxml.cpp -o tinyxml.o $(INC)

tinyxmlerror.o : tinyxmlerror.cpp
	$(CC) -c $(FLAG) tinyxmlerror.cpp -o tinyxmlerror.o $(INC)

tinyxmlparser.o : tinyxmlparser.cpp
	$(CC) -c $(FLAG) tinyxmlparser.cpp -o tinyxmlparser.o $(INC)

$(TARGET) : $(OBJECTS)
	$(AR_) $(TARGET) $(OBJECTS)

clean:
	rm $(OBJECTS)
	rm $(TARGET)
