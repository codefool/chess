CC = g++
CFLAGS = -g -std=c++17
HEADERS = constants.h
OBJECTS = chessboard.o board.o piece.o util.o gameinfo.o db.o position.o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

a.out : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS)

board.o: board.cpp $(HEADERS)
chessboard.o: chessboard.cpp $(HEADERS)
piece.o: piece.cpp $(HEADERS)
util.o: util.cpp $(HEADERS)
gameinfo.o: gameinfo.cpp $(HEADERS)
db.o : db.cpp $(HEADERS)
position.o : position.cpp $(HEADERS)
