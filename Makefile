CC = g++
CFLAGS = -g -std=c++20
INCLUDES=-I /usr/include/mysql-cppconn-8
LIBS=-L/usr/lib/x86_64-linux-gnu -lmysqlcppconn8 -lmysqlcppconn -lpthread
HEADERS = constants.h csvtools.h
OBJECTS = board.o piece.o util.o gameinfo.o position.o worker.o csvtools.o #db.o

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

all: cg bang0 bang1 test

cg : chessboard.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) chessboard.o $(OBJECTS) $(LIBS) -o cg

bang0 : bang0.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) bang0.o $(OBJECTS) $(LIBS) -o bang0

bang1 : bang1.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) bang1.o $(OBJECTS) $(LIBS) -o bang1

bang1 : test.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) test.o $(OBJECTS) $(LIBS) -o test

fenutil : fenutil.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) fenutil.o $(OBJECTS) $(LIBS) -o fenutil

board.o: board.cpp $(HEADERS)
chessboard.o: chessboard.cpp $(HEADERS)
cg.o : cg.cpp $(HEADERS)
csvtools.o : csvtools.cpp $(HEADERS)
piece.o: piece.cpp $(HEADERS)
util.o: util.cpp $(HEADERS)
gameinfo.o: gameinfo.cpp $(HEADERS)
# db.o : db.cpp db.h $(HEADERS)
position.o : position.cpp $(HEADERS)
worker.o : worker.cpp $(HEADERS)
fenutil.o : fenutil.cpp $(HEADERS)
