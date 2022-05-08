CC = g++
CFLAGS = -g -std=c++20
INCLUDES=#-I /usr/local/BerkeleyDB.18.1/include # -I /usr/include/mysql-cppconn-8
LIBS=-L/usr/lib/x86_64-linux-gnu -lpthread # -lmysqlcppconn8 -lmysqlcppconn
HEADERS = dht.h
OBJECTS = dht.o md5.o

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

dht_test : dht_test.o dht.o
	$(CC) $(CFLAGS) dht_test.o $(OBJECTS) $(LIBS) -o dht_test

dht.o : dht.cpp dht.h md5.o
dht_test.o : dht_test.cpp dht.h
md5.o : md5.cpp md5.h

clean:
	rm *.o
