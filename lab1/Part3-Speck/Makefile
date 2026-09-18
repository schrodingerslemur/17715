include ../global.mk

.DEFAULT_GOAL := all

TARGETS=attacker victim

all: $(TARGETS)

%.o: %.c $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build the attacker with optimization O2
attacker.o: attacker.c speck.h
attacker.o: CFLAGS += -O2

# -O1: keeps each round in one 64-byte line
# -fno-toplevel-reorder: preserve the round-major layout
# -fcf-protection=none: avoid an endbr64 that would inflate the line
# -fno-ipa-icf: keep rounds that share a key distinct
SPECK_CFLAGS=-O1 -fno-toplevel-reorder -fcf-protection=none -fno-ipa-icf

speck_rounds.o: speck_rounds.c speck.h
	$(CC) $(SPECK_CFLAGS) -c speck_rounds.c -o speck_rounds.o

victim.o: victim.c speck.h

victim: victim.o speck_rounds.o $(COMMON_OBJ)

$(TARGETS): %: %.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

run_victim:
	taskset -c $(SENDER_CPU) timeout --foreground $(TIMEOUT) ./victim

run_attacker: attacker
	taskset -c $(RECEIVER_CPU) timeout --foreground $(TIMEOUT) ./attacker

.PHONY:	clean

clean:
	$(RM) *.o $(COMMON_OBJ) $(TARGETS)
