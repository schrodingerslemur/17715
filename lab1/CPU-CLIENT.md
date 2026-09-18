# Getting a CPU core

The lab server has 8 physical cores shared by the whole class. You get **one
core for 30 minutes** at a time. Your Makefile requests it for you, so normally
you just run `make`.

## Normal case

```
$ make
cpu-broker: Assigned to you: physical core 3 (logical CPUs 3,11) for the next 30 min.
gcc -O0 ... -o sender sender.c
```

`SENDER_CPU` is set to your core (3 here) and `RECEIVER_CPU` to its SMT sibling
(`SENDER_CPU + 8`, so 11). Run `make` as often as you like during those 30
minutes — you keep the same core, and you are never queued twice.

## When the server is full

```
$ make
cpu-broker: all cores are busy. You are queued at position 2 of 5.
cpu-broker: estimated wait is about 24 min.
cpu-broker: when your turn comes the core is held for you for 10 min -- run
`make` again within that window, or run `cpu-client wait` to block until it is ready.
Makefile:14: *** No CPU core was assigned -- see the cpu-broker message above.  Stop.
```

The build stops; you are in the queue. Two ways to pick your core up:

- run `make` again when your turn comes, or
- run `cpu-client wait`, which blocks and returns as soon as a core is yours.

**You must claim within 10 minutes.** When you reach the front of the queue a
core is reserved for you for 10 minutes. If you don't claim it in that window
you lose your place and have to queue again, so don't queue and walk away —
use `cpu-client wait`.

Ctrl-C during `cpu-client wait` is safe: you keep your place in the queue.

## Commands

```bash
cpu-client status    # who holds what, how long is the queue, where are you in it
cpu-client wait      # queue up and block until a core is yours
cpu-client acquire   # what make runs; prints your core number
cpu-client release   # hand your core back early -- please do this when you finish
cpu-client whoami    # the account the broker sees you as
```

`make clean`, `make cpu-status`, `make cpu-release` and `make cpu-wait` do
**not** request a core, so they work while you are queued.

## Testing on your own machine

There is no broker on your laptop, so `cpu.mk` switches to **manual mode** and
you name the cores yourself:

```bash
make SENDER_CPU=2                 # picks CPU 2 and works out its SMT sibling
make SENDER_CPU=2 RECEIVER_CPU=6  # or name both
export SENDER_CPU=2; make         # for a whole shell session
```

It prints which pair it is using and checks the numbers exist on your machine.
The sibling is read from the kernel when possible, otherwise it assumes the
usual layout (sibling of CPU *n* is *n* + half your logical CPU count). If your
machine has SMT/hyper-threading turned off, `make` says so — the assignment
needs two threads on one physical core.

Setting `SENDER_CPU` is what selects manual mode, so on the lab server it is
ignored: `make` there always uses the core the broker gave you. Nothing is
reserved for you in manual mode, so don't use it to get around the queue.

## Rules

- One core per student. You cannot hold two, or sit in the queue twice.
- Your identity comes from your SSH login — the broker asks the kernel who you
  are, so you cannot request a core as somebody else.
- After 30 minutes your core is released and given to whoever is next. Your
  program is **not** killed, but you are no longer entitled to that core: stop
  it and re-queue, or you are stealing someone else's turn.
- Do not set `SENDER_CPU` by hand or run on a core you were not given. Every
  assignment, queue entry and release is logged with your username.
- Releasing early with `cpu-client release` when you finish a run costs you
  nothing and shortens everyone else's wait.

## When something looks wrong

- `cpu-broker: the CPU broker is not running` — the service is down; tell the
  course staff.
- `cpu-broker: <you> is not a member of the 'students' group` — your account
  isn't set up for the lab; tell the course staff.
- `make` says a core was not assigned but you think you hold one — run
  `cpu-client status` to see what the broker actually thinks.
