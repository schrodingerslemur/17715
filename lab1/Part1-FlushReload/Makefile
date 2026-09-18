include ../global.mk

.DEFAULT_GOAL := all

TARGETS=attacker victim
UTILS=util.o

all: $(TARGETS) shared_file

shared_file: gen_file.py
	python3 gen_file.py

%.o: %.c util.h $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETS): %: %.o $(UTILS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

run_victim: victim shared_file
	taskset -c $(SENDER_CPU) timeout --foreground $(TIMEOUT) ./victim

run_attacker: attacker shared_file
	taskset -c $(RECEIVER_CPU) timeout --foreground $(TIMEOUT) ./attacker

.PHONY:	clean

clean:
	$(RM) *.o $(COMMON_OBJ) $(TARGETS) shared_file
