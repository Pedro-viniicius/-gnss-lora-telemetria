# Makefile dos testes de host do protocolo_comunicacao.
# Compila os mesmos fontes usados no firmware com g++ e roda a suite.

LIB_DIR := ../../firmware/bibliotecas/protocolo_comunicacao

CXX      ?= g++
CXXFLAGS := -std=c++11 -Wall -Wextra -Wpedantic -O2 -I$(LIB_DIR)

FONTES := test_protocolo.cpp \
          $(LIB_DIR)/protocolo.cpp \
          $(LIB_DIR)/crc16.cpp \
          $(LIB_DIR)/cobs.cpp

ALVO := test_protocolo

.PHONY: all test limpar

all: $(ALVO)

$(ALVO): $(FONTES)
	$(CXX) $(CXXFLAGS) $(FONTES) -o $(ALVO)

test: $(ALVO)
	./$(ALVO)

limpar:
	rm -f $(ALVO)
