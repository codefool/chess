CC = g++
CFLAGS = -g -std=c++17
HEADERS = pieces.h constants.h gameinfo.h board.h
OBJECTS = chessboard.o pieces.o board.o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

a.out : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS)