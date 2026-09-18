include ../global.mk

.DEFAULT_GOAL := all

TARGETS=receiver sender
UTILS=util.o

all: $(TARGETS)

%.o: %.c util.h $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETS): %: %.o $(UTILS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

run_sender: sender
	taskset -c $(SENDER_CPU) timeout --foreground $(TIMEOUT) ./sender

run_receiver: receiver
	taskset -c $(RECEIVER_CPU) timeout --foreground $(TIMEOUT) ./receiver

.PHONY:	clean

clean:
	$(RM) *.o $(COMMON_OBJ) $(TARGETS)
