include ../global.mk

.DEFAULT_GOAL := all
TARGETS=main solution
all: $(TARGETS)

run: $(TARGETS)
	@taskset -c $(SENDER_CPU) timeout --foreground $(TIMEOUT) ./main

run_solution: $(TARGETS)
	@taskset -c $(SENDER_CPU) timeout --foreground $(TIMEOUT) ./solution

%.o: %.c $(COMMON_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETS): %: %.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o $(TARGETS)
