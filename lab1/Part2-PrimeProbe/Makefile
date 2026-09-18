include ../global.mk

.DEFAULT_GOAL := all

TARGETS=attacker victim

all: $(TARGETS)

%.o: %.c $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build the attacker with optimization O2
attacker.o: CFLAGS += -O2

$(TARGETS): %: %.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

run_victim:
	taskset -c $(SENDER_CPU) timeout --foreground $(TIMEOUT) ./victim

run_attacker: attacker
	taskset -c $(RECEIVER_CPU) timeout --foreground $(TIMEOUT) ./attacker

.PHONY:	clean

clean:
	$(RM) *.o $(COMMON_OBJ) $(TARGETS)
