CC=gcc
COMMON_DIR=../common
CFLAGS=-O0 -I /usr/local -I$(COMMON_DIR)
TIMEOUT=60s

# pick the CPU cores the lab code runs on
include $(COMMON_DIR)/cpu.mk

# Shared code lives in ../common and is linked into each part.
COMMON_OBJ=$(COMMON_DIR)/common.o

$(COMMON_DIR)/common.o: $(COMMON_DIR)/common.c $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@
