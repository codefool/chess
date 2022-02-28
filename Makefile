CC = g++
CFLAGS = -g -std=c++17
INCLUDES=-I /usr/include/mysql-cppconn-8
LIBS=-L/usr/lib/x86_64-linux-gnu -lmysqlcppconn8 -lmysqlcppconn
HEADERS = constants.h
OBJECTS = chessboard.o board.o piece.o util.o gameinfo.o db.o position.o

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

a.out : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS)

board.o: board.cpp $(HEADERS)
chessboard.o: chessboard.cpp $(HEADERS)
piece.o: piece.cpp $(HEADERS)
util.o: util.cpp $(HEADERS)
gameinfo.o: gameinfo.cpp $(HEADERS)
db.o : db.cpp db.h $(HEADERS)
position.o : position.cpp $(HEADERS)
