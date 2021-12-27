CFLAGS =  -g -std=c++17

DEFAULT = all

a.out : chessboard.cpp
	g++ $(CFLAGS) chessboard.cpp