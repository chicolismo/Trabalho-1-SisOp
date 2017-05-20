#
# Makefile dos TESTES
#

CC=gcc
LIB_DIR=../lib
INC_DIR=../include
BIN_DIR=../bin
SRC_DIR=../src

TST_FLS = $(wildcard *c)
TST_EXE = $(TST_FLS:.c=) = $(TST_FLS:.c=)

# Target para gerar todos os tests
all: aptos teste1 teste2 teste3 teste4 teste5 teste6 caso1 caso2 caso3 caso4 caso5

# Testes
aptos: warning aptos.c $(LIB_DIR)/libcthread.a
	$(CC) -o aptos aptos.c -L$(LIB_DIR) -lcthread -lm -Wall

teste1: Teste1.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste1 Teste1.c -L$(LIB_DIR) -lcthread -lm -Wall

teste2: Teste2.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste2 Teste2.c -L$(LIB_DIR) -lcthread -lm -Wall

teste3: Teste3.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste3 Teste3.c -L$(LIB_DIR) -lcthread -lm -Wall

teste4: Teste4.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste4 Teste4.c -L$(LIB_DIR) -lcthread -lm -Wall
	
teste5: Teste5.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste5 Teste5.c -L$(LIB_DIR) -lcthread -lm -Wall
	
teste6: Teste6.c $(LIB_DIR)/libcthread.a
	$(CC) -o teste6 Teste6.c -L$(LIB_DIR) -lcthread -lm -Wall
	
caso1: caso1.c $(LIB_DIR)/libcthread.a
	$(CC) -o caso1 caso1.c -L$(LIB_DIR) -lcthread -lm -Wall

caso2: caso2.c $(LIB_DIR)/libcthread.a
	$(CC) -o caso2 caso2.c -L$(LIB_DIR) -lcthread -lm -Wall
	
caso3: caso3.c $(LIB_DIR)/libcthread.a
	$(CC) -o caso3 caso3.c -L$(LIB_DIR) -lcthread -lm -Wall
	
caso4: caso4.c $(LIB_DIR)/libcthread.a
	$(CC) -o caso4 caso4.c -L$(LIB_DIR) -lcthread -lm -Wall
	
caso5: caso5.c $(LIB_DIR)/libcthread.a
	$(CC) -o caso5 caso5.c -L$(LIB_DIR) -lcthread -lm -Wall

	
# Mensagem
warning:
	@echo -n "\nATENÇÃO: Este teste usa funções privadas da biblioteca a fim de testa-las, desconsiderar este tipo de warning!\n\n"

# Clean
clean:
	rm -rf aptos cidentify teste1 teste2 teste3 teste4 teste5 teste6 caso1 caso2 caso3 caso4 caso5 *.o *~

# Print para testar makefile
print-%: ; @echo $* = $($*)
