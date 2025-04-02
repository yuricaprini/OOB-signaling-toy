CC = gcc
CFLAGS = -Wall -pedantic -Iinclude

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = test

# Creazione delle cartelle se non esistono
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Oggetti
object_sup  = $(OBJ_DIR)/OOB_supervisor.o $(OBJ_DIR)/compipe.o $(OBJ_DIR)/genlist.o
object_serv = $(OBJ_DIR)/OOB_server.o $(OBJ_DIR)/comsock.o $(OBJ_DIR)/compipe.o $(OBJ_DIR)/internals.o $(OBJ_DIR)/endian.o
object_cli  = $(OBJ_DIR)/OOB_client.o $(OBJ_DIR)/comsock.o $(OBJ_DIR)/internals.o $(OBJ_DIR)/endian.o

.PHONY: all clean test

all: $(BIN_DIR)/OOB_client $(BIN_DIR)/OOB_server $(BIN_DIR)/OOB_supervisor

clean:
	killall OOB_server OOB_client OOB_supervisor || true
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/* *.log OOB-server-*

test:
	killall OOB_server OOB_client OOB_supervisor || true
	rm -rf *.log OOB-server-* $(BIN_DIR)/*
	make
	(cd $(BIN_DIR) && ../$(TEST_DIR)/test.sh)

# Eseguibili
$(BIN_DIR)/OOB_client: $(object_cli)
	$(CC) $(CFLAGS) $(object_cli) -o $@

$(BIN_DIR)/OOB_server: $(object_serv)
	$(CC) $(CFLAGS) $(object_serv) -pthread -o $@

$(BIN_DIR)/OOB_supervisor: $(object_sup)
	$(CC) $(CFLAGS) $(object_sup) -o $@

# Regole di compilazione generiche
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/OOB_supervisor.o: $(SRC_DIR)/OOB_supervisor.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/OOB_server.o: $(SRC_DIR)/OOB_server.c
	$(CC) $(CFLAGS) -pthread -c $< -o $@

$(OBJ_DIR)/OOB_client.o: $(SRC_DIR)/OOB_client.c
	$(CC) $(CFLAGS) -c $< -o $@
