CC = g++
CFLAGS = -g -std=c++17
HEADERS = constants.h gameinfo.h board.h pieces.h
OBJECTS = chessboard.o board.o piece.o king.o queen.o bishop.o knight.o rook.o pawn.o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

a.out : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS)

bishop.o: bishop.cpp pieces.h
board.o: board.cpp board.h pieces.h gameinfo.h
chessboard.o: chessboard.cpp gameinfo.h constants.h pieces.h board.h
king.o: king.cpp pieces.h
knight.o: knight.cpp pieces.h
pawn.o: pawn.cpp pieces.h
piece.o: piece.cpp pieces.h
queen.o: queen.cpp pieces.h
rook.o: rook.cpp pieces.h
