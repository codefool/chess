CC = g++
CFLAGS = -g -std=c++20
INCLUDES=#-I /usr/local/BerkeleyDB.18.1/include # -I /usr/include/mysql-cppconn-8
LIBS=-L/usr/lib/x86_64-linux-gnu -lpthread # -lmysqlcppconn8 -lmysqlcppconn
HEADERS = config.h constants.h csvtools.h dht.h dq.h
OBJECTS = board.o util.o gameinfo.o piece.o position.o worker.o csvtools.o md5.o dht.o dq.o

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

all: cg

cg : chessboard.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) chessboard.o $(OBJECTS) $(LIBS) -o cg

bang0 : bang0.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) bang0.o $(OBJECTS) $(LIBS) -o bang0

bang1 : bang1.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) bang1.o $(OBJECTS) $(LIBS) -o bang1

dht : dht_util.o dht.o md5.o util.o $(HEADERS)
	$(CC) $(CFLAGS) dht_util.o dht.o md5.o util.o $(LIBS) -o dht

dq : dq_util.o dq.o $(HEADERS) clean_dq
	$(CC) $(CFLAGS) dq_util.o dq.o $(LIBS) -o dq

clean-dq :
	rm -rf ~/tmp/testqueue

clean-cg :
	rm -rf ~/work/data

test : test.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) test.o $(OBJECTS) $(LIBS) -o test

fenutil : fenutil.o $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) fenutil.o $(OBJECTS) $(LIBS) -o fenutil

board.o: board.cpp $(HEADERS)
chessboard.o: chessboard.cpp $(HEADERS)
cg.o : cg.cpp $(HEADERS)
csvtools.o : csvtools.cpp $(HEADERS)
dht.o : dht.cpp dht.h md5.o $(HEADERS)
dq.o : dq.h $(HEADERS)
fenutil.o : fenutil.cpp $(HEADERS)
gameinfo.o: gameinfo.cpp $(HEADERS)
md5.o : md5.cpp md5.h
piece.o: piece.cpp $(HEADERS)
position.o : position.cpp $(HEADERS)
util.o: util.cpp $(HEADERS)
worker.o : worker.cpp $(HEADERS)
