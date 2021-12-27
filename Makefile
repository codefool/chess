CFLAGS =  -g -std=c++17

DEFAULT = all

a.out : chessboard.cpp pieces.h constants.h
	g++ $(CFLAGS) chessboard.cpp