CC = g++
CFLAGS = -g -std=c++17
HEADERS = constants.h gameinfo.h board.h piece.h
OBJECTS = chessboard.o board.o piece.o util.o gameinfo.o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

a.out : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS)

board.o: board.cpp board.h piece.h gameinfo.h
chessboard.o: chessboard.cpp gameinfo.h constants.h piece.h board.h
piece.o: piece.cpp piece.h
util.o: util.cpp constants.h
gameinfo.o: gameinfo.cpp gameinfo.h
