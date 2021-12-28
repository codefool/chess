CC = g++
CFLAGS = -g -std=c++17
HEADERS = constants.h gameinfo.h board.h pieces.h
OBJECTS = chessboard.o board.o piece.o king.o queen.o bishop.o knight.o rook.o pawn.o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

a.out : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS)