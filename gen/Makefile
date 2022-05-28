SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := ../bin
LIB_DIR := ../lib

SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
INC := ../include

CC := g++
CFLAGS := -g -std=c++20 -I $(INC)
LDFLAGS :=
LDLIBS := -L/usr/lib/x86_64-linux-gnu -lpthread # -lmysqlcppconn8 -lmysqlcppconn

$(BIN_DIR)/dr : $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

dr : $(BIN_DIR)/dr

clean:
	rm $(OBJ_DIR)/*.o

clean-dq :
	rm -rf ~/tmp/testqueue

clean-dr :
	rm -rf ~/work/data
