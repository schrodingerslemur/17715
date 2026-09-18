# cpu.mk -- pick the CPU cores the lab code runs on.
#
# Include this from your Makefile:   include ../common/cpu.mk
# It sets two variables for you:
#
#   SENDER_CPU     the logical CPU the sender runs on
#   RECEIVER_CPU   its SMT sibling, where the receiver runs
#
# There are two modes, chosen by whether the broker client is installed:
#
#   broker mode  (the lab server)  ask the CPU broker for a core, because the
#                                  machine is shared with the whole class
#   manual mode  (your own laptop) use the cores you name yourself
#
#   make                          # server: ask the broker
#   make SENDER_CPU=2             # laptop: use CPU 2 and its SMT sibling
#   make SENDER_CPU=2 RECEIVER_CPU=6      # ... or name both
#   export SENDER_CPU=2; make     # ... for a whole shell session
#   make CPU_MODE=manual SENDER_CPU=2     # force manual mode on the server
#
# On the server the broker's answer always wins: SENDER_CPU set by hand is
# ignored there unless you ask for manual mode explicitly.

CPU_CLIENT ?= /usr/local/bin/cpu-client
CPU_MODE ?= auto

# The broker client only exists on the shared server, so its presence is what
# distinguishes "shared machine, take a turn" from "my machine, do as I like".
ifeq ($(CPU_MODE),auto)
  ifneq ($(wildcard $(CPU_CLIENT)),)
    CPU_MODE := broker
  else
    CPU_MODE := manual
  endif
endif

# Targets that do not run anything on a core, and so need neither a core from
# the broker nor a SENDER_CPU of your own.  `make clean` works while you are
# sitting in the queue, and on a laptop with nothing configured.
CPU_FREE_GOALS := clean distclean help cpu-status cpu-release cpu-wait
CPU_SKIP := $(if $(strip $(filter-out $(CPU_FREE_GOALS),$(or $(MAKECMDGOALS),all))),,1)

# --------------------------------------------------------------------------
# broker mode: the shared lab server
# --------------------------------------------------------------------------
ifeq ($(CPU_MODE),broker)

# Running `make` asks the broker for a core.  If one is free you get it for 30
# minutes and SENDER_CPU is set automatically.  If every core is busy you are
# put in a queue, the wait estimate is printed, and the build stops.  Re-run
# `make` when your turn comes, or run `cpu-client wait` to block until then.
#
# Asking twice is harmless: while your lease is alive the broker just re-prints
# the same core, so `make` inside a lease never queues you again.

# `override` so that in broker mode the broker's answer always wins, even
# against `make SENDER_CPU=...` on the command line.  (Setting SENDER_CPU that
# way selects manual mode instead, which is the supported way to choose your
# own cores.)
ifdef CPU_SKIP
  # Only "free" goals were requested: skip the broker entirely.
  override SENDER_CPU := 0
else
  # stdout carries just the core number; the broker's messages (assignment
  # details, or your queue position and wait time) go to stderr and appear
  # on your terminal.
  override SENDER_CPU := $(shell $(CPU_CLIENT) acquire)
endif

ifeq ($(strip $(SENDER_CPU)),)
$(error No CPU core was assigned -- see the cpu-broker message above. If you were queued, wait for your turn and run make again, or run `$(CPU_CLIENT) wait` to block until a core is free)
endif
ifneq ($(shell echo '$(strip $(SENDER_CPU))' | grep -qE '^[0-7]$$' && echo ok),ok)
$(error SENDER_CPU must be an integer between 0 and 7 (got '$(SENDER_CPU)'))
endif

# On the lab server the two SMT siblings of physical core N are logical CPUs
# N and N+8.  The broker refuses to start if that is ever untrue.
override RECEIVER_CPU := $(shell expr $(strip $(SENDER_CPU)) + 8)

# --------------------------------------------------------------------------
# manual mode: your own machine
# --------------------------------------------------------------------------
else ifeq ($(CPU_MODE),manual)

ifdef CPU_SKIP

# Nothing will run on a core, so do not insist on a valid pair.
SENDER_CPU := 0
RECEIVER_CPU := 1

else

CPU_NLOGICAL := $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || \
                        sysctl -n hw.logicalcpu 2>/dev/null || echo 0)

ifeq ($(strip $(SENDER_CPU)),)
$(error No CPU broker on this machine, so set the cores yourself: `make SENDER_CPU=<n>` (this machine has $(CPU_NLOGICAL) logical CPUs). Add RECEIVER_CPU=<n> too if the sibling cannot be detected)
endif
ifneq ($(shell echo '$(strip $(SENDER_CPU))' | grep -qE '^[0-9]+$$' && echo ok),ok)
$(error SENDER_CPU must be a non-negative integer (got '$(SENDER_CPU)'))
endif

# Work out the SMT sibling unless you named it yourself.  Prefer what the
# kernel reports; the sibling list is either "3,11" or "0-1" depending on the
# kernel, and awk handles both.  Fall back to the usual Linux layout, where
# the sibling of CPU n is n + (logical CPUs / 2).
ifeq ($(strip $(RECEIVER_CPU)),)
  CPU_SIBFILE := /sys/devices/system/cpu/cpu$(strip $(SENDER_CPU))/topology/thread_siblings_list
  RECEIVER_CPU := $(shell awk -F'[,-]' -v s=$(strip $(SENDER_CPU)) \
      '{ for (i = 1; i <= NF; i++) if ($$i + 0 != s) { print $$i + 0; exit } }' \
      $(CPU_SIBFILE) 2>/dev/null)
  ifeq ($(strip $(RECEIVER_CPU)),)
    RECEIVER_CPU := $(shell expr $(strip $(SENDER_CPU)) + $(CPU_NLOGICAL) / 2 2>/dev/null)
  endif
endif

ifeq ($(strip $(RECEIVER_CPU)),)
$(error Could not work out the SMT sibling of CPU $(SENDER_CPU). Name it yourself: `make SENDER_CPU=$(SENDER_CPU) RECEIVER_CPU=<n>`)
endif
ifeq ($(strip $(RECEIVER_CPU)),$(strip $(SENDER_CPU)))
$(error RECEIVER_CPU came out equal to SENDER_CPU ($(SENDER_CPU)) -- SMT/hyper-threading looks disabled on this machine. Enable it, or name a second CPU yourself with RECEIVER_CPU=<n> (the results will not show an SMT side channel))
endif
ifneq ($(shell echo '$(strip $(RECEIVER_CPU))' | grep -qE '^[0-9]+$$' && echo ok),ok)
$(error RECEIVER_CPU must be a non-negative integer (got '$(RECEIVER_CPU)'))
endif
# Only range-check when we actually managed to count the CPUs.
ifneq ($(shell [ '$(CPU_NLOGICAL)' -gt 0 ] 2>/dev/null && echo yes),)
ifeq ($(shell [ $(strip $(SENDER_CPU)) -lt $(CPU_NLOGICAL) ] && [ $(strip $(RECEIVER_CPU)) -lt $(CPU_NLOGICAL) ] && echo ok),)
$(error SENDER_CPU=$(strip $(SENDER_CPU)) / RECEIVER_CPU=$(strip $(RECEIVER_CPU)) is out of range: this machine has $(CPU_NLOGICAL) logical CPUs, numbered 0-$(shell expr $(CPU_NLOGICAL) - 1))
endif
endif

$(info cpu.mk: manual mode -- sender on CPU $(strip $(SENDER_CPU)), receiver on CPU $(strip $(RECEIVER_CPU)). Nothing is reserved for you; do not use this mode on the shared lab server.)

endif

else
$(error CPU_MODE must be auto, broker or manual (got '$(CPU_MODE)'))
endif

# These are included from the top of the lab Makefiles, so without care the
# first one below would become the *default goal* -- plain `make` would print
# the broker status instead of building, and would take a core to do it (the
# skip list above only sees the goals you typed, and you typed none).  So
# remember the default goal on the way in and put it back afterwards.  Empty
# means "no target seen yet", which lets the next target the including
# Makefile defines become the default, exactly as if cpu.mk were not here.
CPU_SAVED_DEFAULT_GOAL := $(.DEFAULT_GOAL)

.PHONY: cpu-status cpu-release cpu-wait
cpu-status:
	@$(CPU_CLIENT) status
cpu-release:
	@$(CPU_CLIENT) release
cpu-wait:
	@$(CPU_CLIENT) wait

.DEFAULT_GOAL := $(CPU_SAVED_DEFAULT_GOAL)
