CC = g++
CFLAGS = -g -std=c++17
INCLUDES=-I /usr/include/mysql-cppconn-8
LIBS=-L/usr/lib/x86_64-linux-gnu -lmysqlcppconn8 -lmysqlcppconn -lpthread
HEADERS = constants.h
OBJECTS = board.o piece.o util.o gameinfo.o position.o worker.o #db.o

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

all: a.out bang0 #cg

a.out : chessboard.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) chessboard.o $(OBJECTS) $(LIBS)

cg : cg.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) cg.o $(OBJECTS) $(LIBS) -o cg

bang0 : bang0.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) bang0.o $(OBJECTS) $(LIBS) -o bang0

board.o: board.cpp $(HEADERS)
chessboard.o: chessboard.cpp $(HEADERS)
cg.o : cg.cpp $(HEADERS)
piece.o: piece.cpp $(HEADERS)
util.o: util.cpp $(HEADERS)
gameinfo.o: gameinfo.cpp $(HEADERS)
# db.o : db.cpp db.h $(HEADERS)
position.o : position.cpp $(HEADERS)
worker.o : worker.cpp $(HEADERS)
