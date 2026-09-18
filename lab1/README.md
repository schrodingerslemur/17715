# Lab1: Cache Side Channel Attacks

- **Course:** 17-435/17-715
- **Due Date:** Sep 24, 2026
- **Last Updated Date:** Sep 8, 2026

## Collaboration and AI Policy

Our full Academic Honesty policy can be found on [our website](https://www.andrew.cmu.edu/course/17-715/2026f/syllabus/). As a reminder, all labs should be completed individually. You may discuss the lab at a high level with a classmate, but you may not work on code together or share any of your code. You may use AI tools as a tutor, but you may not upload lab materials to external AI services or use them to solve assignments for you. We may ask you to explain or walk us through your lab submissions; if you cannot explain the concepts behind your submitted work, you will receive no credit for that assignment.

## Getting Started

Log in to our lab machine `coffeelakerf.lan.local.cmu.edu` via SSH by running `ssh username@coffeelakerf.lan.local.cmu.edu`. The username is your Andrew ID, and we have sent you the password through [Canvas](https://canvas.cmu.edu/). After you have access to the lab machine, you can put your public key at `~/.ssh/authorized_keys` to log in with your private key.

Please note that the lab machine is only accessible through CMU's network, and you will need to connect to [CMU's VPN](https://www.cmu.edu/computing/services/endpoint/network-access/vpn/index.html) if you want to access it off-campus.

> [!WARNING]
> We will delete your account and cleanup the home directories after the submission deadline. Please do **NOT** store important files on the lab machine.

## Introduction

In this lab, you will complete the following tasks:

- Solve three CTF (capture-the-flag) puzzles using cache-based side channels.
- Build a covert channel to send and receive arbitrary messages using cache-based side channels.

In this lab, you will learn how to interact and manipulate fine-grained cache states in real hardware. Real, commercial hardware is a black box to us. To be able to mount a cache attack, we need to leverage our computer architecture knowledge to infer how a cache behaves for a sequence of instructions. Making the attacker's life more difficult, real-world caches are far more complex than the toy example caches that we learned in the classroom. After completing this lab, you will hopefully get a glimpse of the complexity of these hardware features.

### Getting Prepared Before You Start

You will program in C throughout this lab. C is a low-level language that gives you more control over the hardware compared to high-level languages. Programs written in C can be directly compiled into machine code, and directly executed on the hardware without other abstraction layers. When working on microarchitectural attacks, having a high degree of control over the exact instructions being executed is essential.

### Setting Up Your Environment

You will run your attacks using one physical CPU core on the lab machine. Each physical CPU core is a pair of SMT (Simultaneous Multi-Threading) cores, or logical cores. These two logical cores map to the same physical core and share multiple hardware resources associated with that core, such as private L1 and L2 caches.

Our lab machine will allocate a dedicated CPU core for you to test your attack without the interference from other users. Note that for the lab machine, logical cores $i$ and $i+8$ are mapped to a physical core (e.g., logical cores $(0, 8)$ are paired and mapped to a physical core, and $(1, 9)$ and $(2, 10)$ are also paired as physical cores).

There are 8 physical cores on the lab machine. When you build and run your code using `make`, the make file will request a dedicated core and pin your program on that core. After a request, you will be able to use the CPU core for 30 minutes. However, when all CPU cores are all allocated, you will enter a queue to wait for the next available core. For more information about this queue and the allocation rules, please refer to `CPU-CLIENT.md`.

> [!WARNING]
> Currently, these allocation rules are only enforced by the make files, meaning that you can still manually pin your processes to other CPU cores not allocated to you. However, we may change the allocation policy and deploy system-wide enforcement rules in the future.

## Lab Machine's Microarchitecture

Before we begin, let's take a look at the lab machine's microarchitecture. The simplest way to gather hardware information is to use existing public system interfaces and documentation. Here is a list of commands that can be used to determine machine architecture information on Linux.

- `lscpu`: Provides information on the type of processor and some summary information about the architecture in the machine.
- `less /proc/cpuinfo`: Provides detailed information about each logical processor in the machine. (Type `q` to exit.)
- `getconf -a | grep CACHE`: Displays the system configuration related to the cache. This will provide detailed information about how the cache is structured. The numbers that are reported using this command use Bytes (B) as the unit.

> [!TIP]
> Fill in the blanks in the following table using the information you gathered about the cache configuration of the lab machine. You should be able to directly obtain the information for the first 3 blank columns using commands above. You will need to derive the number of sets using what you have learned about set-associative caches in the class. Note that as some levels of cache are private to a physical core, some commands may output the size per-core, while others may print out the size in total.

| Cache          | Cache Line Size | Total Size | Number of Ways (Associativity) | Number of Sets |
|----------------|-----------------|------------|--------------------------------|----------------|
| L1-Data        |        64       |  32768     |                8               |         64     |
| L1-Instruction |       64        |    32768   |               1                |                |
| L2             |       64        |   262144   |              4                 |                |
| L3             |       64        | 16777216   |             16                  |                |

## Part 1: Capture the Flag with Flush+Reload (20%)

From now on, we are entering attack time. In this part of the lab, you will be attempting to extract secrets from a victim program. You will get a taste of solving a Capture-the-Flag (CTF) puzzle.

### Get to Know the Victim

We provide you with a victim program in `Part1-FlushReload/victim.c`, whose pseudocode is listed below. The victim program uses `mmap` to map a file into its own virtual address space to create a buffer. It then generates a random integer as the flag and uses the flag to index into the buffer. Your task is to learn the flag value by monitoring the victim's memory accesses using a Flush+Reload attack.

```c
// Allocate a large memory buffer
char *buf = get_buffer();

// Set the flag to random integer in the range [0, 1024)
int flag = random(0, 1024);
printf(flag);

// Main loop
while (true) {
  value = load(buf + flag * 128);
}
```

### The Attack Setup and Your Plan

We have set up the attack framework that enables your attacker program to share a memory region with the victim. It uses a technique called memory-mapped file, where two virtual addresses (one from your program's address space and the other from the victim's address space) are mapped to a same physical address, which contains a copy of a file on the hard drive.

Your attack should implement standard Flush+Reload. We are providing you with the attack skeleton and several practical tips.

- **Flush:** Flush a cache line that might be accessed by the victim to DRAM using `clflush`. We defined a `clflush` function in `common/common.h`, and you can call this function to flush an address. Be careful with the aforementioned cache line granularity issue. Note that cache line size != integer size.
- **Wait:** Wait a few hundred cycles for the victim to perform a flag-dependent memory load operations. Don't use the system-provided `sleep` function to do this, since this function will trigger a system call, potentially destroying cache states.
- **Reload:** Re-access the cache line in the Flush step and measure the access latency to each of them.
- **Repeat** these steps for all cache lines Use the threshold derived from Part 1 to decode the flag value.

> [!TIP]
> **Beware of the hardware prefetcher!**
>
> Modern processors can predict future memory accesses and prefetch data into the cache before it is used. Hardware prefetching is an effective performance optimization technique that is widely deployed in real-world processors. This feature can confuse your attack code. For example, regardless of what the flag value is, some Flush+Reload attack implementation may consistently observe a cache miss for the first reload operation, and cache hits for the rest of the reload operations, because the first load miss triggers hardware prefetching for the later addresses.
>
> Usually, the hardware prefetcher makes address prediction based on simple patterns, such as a linear, fixed-stride access pattern within a page. Therefore, you can bypass the prefetching effects by introducing randomness to your address access pattern.
>
> The prefetchers are enabled on the lab machines. Make sure you have avoided aforementioned simple access patterns in your code.
>
> We suggest perform flush and reload a single line at a time, rather than flushing all the lines and reloading all the lines.

### Your Task

Complete the code in `Part1-FlushReload/attacker.c` to successfully extract the secret values from `Part1-FlushReload/victim`.

To test your attack, you should first compile your code using `make`, which will compile the attacker, the victim, and generate a file (`shared_file`) for the shared buffer. Then use tmux, screen, or simply two SSH connections, and run `make run_victim` in one terminal and `make run_attacker` in another terminal. Make sure you are NOT executing `./victim` or `./attacker` directly because they will not run the programs on your assigned core.

### Grading

Your code should be able to reliably capture the flag. Due to system noise, we will grade this part by executing your code multiple times. Full credit will be awarded if your code works at least 8 out of 10 runs. Each run should not take longer than 1 minute.

## Part 2: Capture the Flag with Prime+Probe (30%)

We will now solve a more challenging CTF puzzle, leaking the flag using a Prime+Probe attack. In this setup, the attacker and the victim no longer share memory, and thus Flush+Reload will not work. Instead, to make the attack work, you need to carefully manipulate cache states and trigger cache set-conflicts.

### Get to Know the Victim

We provide you with a victim program in `Part2-PrimeProbe/victim.c`, whose pseudocode is listed below. The victim program generates a random offset and adds this offset to the buffer it allocated. Then it computes the flag value using this offset. After that, it enters an infinite loop that keeps accessing the buffer at four different addresses.

```c
// Allocate a large memory buffer
char *buf = get_buffer();

// Compute a radom offset, add this offset to the buffer, and compute the flag
int offset = rand() % 0x10000;
buf += offset;
int flag = offset >> 6;
printf(flag);

// Main loop
while (true) {
    for (int i = 0; i < 4; i++) {
        // Access the buffer
        (*(char *)(buf + i * 0x10000))++;
    }
}
```

### Using Huge Pages

When the victim calls `get_buffer()` in the above pseudocode, it allocates a 2 MB huge page instead of a typical 4 KB page.

While the default page size is 4 kilobytes, Linux supports huge pages, which allow programs to allocate contiguous physical memory larger than the standard 4 KB size. Our victim here creates a 2 MB page, ensuring $2^{21}$ bytes of contiguous physical addresses. You can use the `mmap` system call as follows to allocate a 2MB page. You can use the command `man mmap` to understand the semantics for each argument used by this function.

```c
size_t BUFF_SIZE = 1 << 21;
void *buf= mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                -1, 0);

if (buf == (void*) - 1) {
  perror("mmap() error\n");
  exit(EXIT_FAILURE);
}

*((char *)buf) = 1; // dummy write to trigger page allocation
```

You can see if your huge page is being allocated or not by watching the status of `/proc/meminfo`. If you run `cat /proc/meminfo | grep HugePages_`, you should see the number of `HugePages_Free` decrease by 1 when your code is using one.

### Which Level of Cache Should You Target?

Before the attack, you need to first figure out which level of cache you want to target.

Similar to Part 1, the addresses you are dealing with in your C code are virtual addresses while physical addresses (which you will not have access to within your code) are used when indexing into caches. This fact can be more problematic in this part because you might need to do more careful calculation on the addresses.

> [!TIP]
> Answering the following questions may help you figure out which level of cache to attack:
> 
> - Why is the offset computed as `rand() % 0x10000`?
> - Why is the flag computed as `offset >> 6`?
> - The victim accesses these addresses: `buf + i * 0x10000`. Do they map to the same L1 cache set? How about L2 or L3?

### Implementing the Attack: Prime+Probe

We outline the attack procedure below and provide a few tips. The most important rule is, do not try to implement everything then test. Modern processors often contain optimizations that make them behave differently from the simplified architectures taught in class. This lab requires experimentation to find working approaches and values. You should not expect your solution to work on the first attempt, so be sure to incrementally build up your solution and verify that each step is working before proceeding.

- **Eviction addresses collection:** You need to find a group of eviction addresses for each cache set, so that when these eviction addresses are accessed, they can fully occupy a given cache set. This step requires a clear understanding of the cache addressing scheme. We highly suggest you calculate twice, code once. Trust us, sitting down to think through cache addressing before coding will save you time.
- **Prime:** For each cache set, access its corresponding eviction addresses to place these addresses in the cache and fully occupy the cache set. Again, be careful with the mismatch of the size of an integer and a cache line. Repeatedly accessing the same cache line will only bring one line into the cache, far from being able to monitor the whole cache set.
- **Wait:** Similar to the Flush+Reload attack, you need to wait for a while before you probe. Do not use system call functions, such as `sleep`.
- **Probe:** For each cache set, re-access the eviction addresses for each cache set and measure their access latency. You can use simple statistic analysis (e.g., median, average, maximum, or median/average/max after removing outliers) to decode the flag.

### Your Task

Complete the code in `Part2-PrimeProbe/attacker.c` to successfully extract the secret values from `Part2-PrimeProbe/victim`.

Similar to Part 1, you need to first compile your code using `make`, which will compile the attacker and the victim. Then run `make run_victim` in one terminal and `make run_attacker` in another terminal. Make sure you are NOT executing `./victim` or `./attacker` directly because they will not run the programs on your assigned core.

### Grading

Your code should be able to reliably capture the flag. Due to system noise, we will grade this part by executing your code multiple times. Full credit will be awarded if your code works at least 8 out of 10 runs. Each run should not take longer than 1 minute.

### Practical Coding Tips

If the receiver needs to measure the latency of multiple memory accesses, you should pay attention to the following features that can introduce substantial noise to your communication channel.

**Randomizing the access pattern during probe:** Similar to the Flush+Reload attack, accessing addresses with a fixed-stride pattern can trigger hardware prefetching and introduce confusing measurement results. The problem is that you do not know when you observe a cache hit, the line was always located inside the cache or it was brought into the cache by the prefetcher. Randomizing the access patterns may improve the accuracy of your attack.

**Reduce memory traffic in the attacker program:** While you are priming and probing the cache, the memory footprint of your own attacker program will create noise to the cache, which will make the victim's signal more difficult to recover. To avoid this, try to reduce the number of cache lines that your attacker program accesses. This can be done through various ways such as utilizing the registers instead of the stack, run Prime+Probe in one tight loop and prevent function calls, etc.

**Beware of the cache inclusion policy:** The lab machine's L2 cache is *non-inclusive* of L1. That is, a cache line may exist only in the L1 cache, only in the L2 cache, or exist in both L1 and L2. When you are priming and probing the cache lines, think about which level of cache they may reside in, and how can it affect their access latency.

## Part 3: Decrypt the SPECK cipher (30%)

Now that you know how to launch cache side channel attacks with Flush+Reload and Prime+Probe, let's attack something (slightly) more realistic. In this part, you need to attack a victim running the [SPECK](https://eprint.iacr.org/2013/404) block cipher and recover the plaintext from a ciphertext.

### Get to Know the Victim

We provide you with a victim program in `Part3-Speck/victim.c`, whose pseudocode is listed below. The victim first generates a random plaintext and picks a random encryption function out of 64 possible encryption functions. Here, all 64 encryption functions are computing the same SPECK encryption, but they are encrypting with different keys. It then encrypts the plaintext for 1 million times and prints the ciphertext. After that, it repeatedly redo the encryption to double check the encryption result.

Your goal is to figure out which encryption function is used, and then recover the plaintext.

```c
// Generate a random plaintext
block pt = random_plaintext();

// Pick a random encryption function
int func_idx = rand() % 64;
fn encrypt = speck_encrypt_functions[func_idx];

// Encrypt the plaintext 1M times to make it *extra* secure!
block ref = pt;
for (long i = 0; i < 1000000; i++) ref = encrypt(ref);
printf(ref);

// Double check the encryption
while(1) {
    block check = pt;
    for (long i = 0; i < 1000000; i++) check = encrypt(check);
    if (check != ref) printf("ERROR!");
}
```

### The SPECK Block Cipher

The SPECK block cipher is a lightweight block cipher with a 32-bit block (stored in two 16-bit variables `x` and `y`) and a 64-bit key. Its implementation in C is listed below. In the victim's program, there are 64 encryption functions (`speck_encrypt_functions`), each of them looks very similar to the `speck_encrypt` function below. The difference between them is the `round_key` they use. You can see the hardcoded round keys in `speck_rounds.c`. These round keys are derived from the keys hardcoded in `speck.h` (the `speck_keys` array).

Note that you do **NOT** have to understand the details of this algorithm to solve this challenge. The goal of this challenge is to decode the signals in the cache side channel instead of doing cryptanalysis on this block cipher.

```c
#define ROR16(x, r) ((uint16_t)(((x) >> (r)) | ((x) << (16 - (r)))))
#define ROL16(x, r) ((uint16_t)(((x) << (r)) | ((x) >> (16 - (r)))))

void speck_encrypt(uint16_t *x, uint16_t *y)
{
    for (unsigned i = 0; i < SPECK_ROUNDS; i++) {
        *x = ROR16(*x, 7);
        *x = *x + *y;
        *x = *x ^ round_key[i];

        *y = ROL16(*y, 2);
        *y = *y ^ *x;
    }
}
```

### The Signal in the Cache Side Channel

When the victim executes `encrypt`, it will trigger a series of encryption functions defined in `speck_rounds.c`. For example, if the encryption function picked by the victim is `speck_g8_r0`, when it calls `encrypt`, it will recursively call `speck_g8_r0`, `speck_g8_r1`, `speck_g8_r2`, `speck_g8_r3`, ..., and finally `speck_g8_r21`. That is, instead of running the encryption rounds in a loop, the victim defined each round as a separate function, and it executes these 22 round functions sequentially in one encryption.

Even though these round functions have no memory access instructions, executing them will still create memory traffic. This is because the code a program executes is also stored in the memory, and the CPU needs to read the instructions from the memory in order to execute them. Therefore, the execution trace of the victim program will leave certain signals in the cache, and your attack will need to decode these signals to solve this challenge.

**Inspect the victim's layout in the memory**

We can inspect the victim program's layout to see what memory access pattern will be generated when the victim executes the round functions. Here, we use the `nm` command to see the function address in a program. The output of `nm -n ./victim | grep speck_ | head` is listed as follows:

```
0000000000002000 t speck_g0_r0
0000000000002040 t speck_g1_r0
0000000000002080 t speck_g2_r0
00000000000020c0 t speck_g3_r0
0000000000002100 t speck_g4_r0
0000000000002140 t speck_g5_r0
0000000000002180 t speck_g6_r0
00000000000021c0 t speck_g7_r0
0000000000002200 t speck_g8_r0
0000000000002240 t speck_g9_r0
```

This shows that `speck_g0_r0` is located at address `0x2000`, and the next function (`speck_g1_r0`) is `0x40` or 64 bytes after it.

We can also find all the round functions that will be executed by the victim. For example, here is the output of `nm -n ./victim | grep speck_g8_`, which lists the addresses of the functions that are called when the victim picks `speck_g8_r0`:

```
0000000000002200 t speck_g8_r0
0000000000003200 t speck_g8_r1
0000000000004200 t speck_g8_r2
0000000000005200 t speck_g8_r3
0000000000006200 t speck_g8_r4
0000000000007200 t speck_g8_r5
0000000000008200 t speck_g8_r6
0000000000009200 t speck_g8_r7
000000000000a200 t speck_g8_r8
000000000000b200 t speck_g8_r9
000000000000c200 t speck_g8_r10
000000000000d200 t speck_g8_r11
000000000000e200 t speck_g8_r12
000000000000f200 t speck_g8_r13
0000000000010200 t speck_g8_r14
0000000000011200 t speck_g8_r15
0000000000012200 t speck_g8_r16
0000000000013200 t speck_g8_r17
0000000000014200 t speck_g8_r18
0000000000015200 t speck_g8_r19
0000000000016200 t speck_g8_r20
0000000000017200 t speck_g8_r21
```

Therefore, in one encryption, the victim will access the following addresses: `0x2200`, `0x3200`, `0x4200`, ..., `0x17200`.

> [!TIP]
> Answering the following questions may help you figure out how to exploit this access pattern:
>
> - For the 22 round functions, do they map to the same L1 cache set? How about L2 or L3?
> - The L1 cache is split into two halves: the L1d cache for data and the L1i cache for instructions. Which half should you target?
> - How to flush, prime, or probe the L1i cache? How to measure its latency?
> - Is it possible to differentiate the access latency between an L1 hit and an L2 hit? How about multiple L1 hits versus multiple L2 hits?
>

### Decrypt the Ciphertext

Once you figured out the encrypt function the victim is using, you can use the `speck_decrypt` function in `speck.h` to decrypt the ciphertext. You do **NOT** have to implement the decryption algorithm yourself. The decryption code is provided for you in `Part3-Speck/attacker.c`.

### Your Task

Complete the code in `Part3-Speck/attacker.c` to successfully decrypt the ciphertext from `Part3-Speck/victim`.

To test your attack, you should first compile your code using `make`, which will compile the attacker and the victim. Then use tmux, screen, or simply two SSH connections, and run `make run_victim` in one terminal and `make run_attacker` in another terminal. Make sure you are NOT executing `./victim` or `./attacker` directly because they will not run the programs on your assigned core.

### Grading

Your code should be able to reliably decrypt the ciphertext. Due to system noise, we will grade this part by executing your code multiple times. Full credit will be awarded if your code works at least 8 out of 10 runs. Each run should not take longer than 1 minute.

## Part 4: Dead Drop – An Evil Chat Client (20%)

If you find leaking an integer is not exciting enough, you can level it up to build a covert channel to send and receive arbitrary messages, like an evil chat client that can stealthily communicate without being monitored by privileged software, such as the OS. In this part, you can decide to build a chat client using Flush+Reload, Prime+Probe, or even some other fancy side channels. There are only very few requirements.

- The sender and receiver must be different processes.
- The sender and receiver should not set up a shared writable address space between them, nor call any obviously-useful-for-chat functions such as Unix sockets.
- You can use some convenience code that stays within the spirit of the lab. Obviously, you may not use pre-packaged code from online for building covert channels (e.g., mastik).

### Expected Behavior

The Dead Drop client should behave in the following way. Using tmux, screen, or simply two SSH connections, we can have two different terminals running on the same machine and run the following commands:

```
Terminal B: $ make run_receiver    // you start the receiver process in a terminal
Terminal B: Receiver now listening.
Terminal A: $ make run_sender      // you start the sender in another terminal
Terminal A: Please type a message.

Terminal A: Hello, world!          // you type a message and hit enter in the sender's terminal
Terminal B: Hello, world!          // receiver should generate the same message as you entered on the sender's side
```

Note that you should support messages containing arbitrary number of characters. For example, the message "Hello, world!" above contains 13 characters and is typed by user together. Then all 13 characters appear on the receiver side. To achieve this, your sender needs to signal the receiver that "the next character is coming" in some way. Partial credits will be awarded for solutions which only support a fixed number of characters in a message.

### Grading

You need to submit your code to your assigned GitHub repository. You code needs to first successfully send and receive the "Hello, world!" message to get 80% of the credits and then messages with arbitrary length and contents to get the remaining 20%. We do not accept code that directly prints the "Hello, world!" message in the receiver. You will receive full credit if your chat client can correctly decode the test messages at least 6 out of 10 runs. Each run should not take longer than 1 minute.

## Submission

You need to submit your code as a `.zip` file on Canvas. We provided a `Makefile` for you to pack your code. Please run `make submission` under the lab root folder to execute it.

You can modify the provided `Makefile`s in each part, in case you want to change the compiler flags or add additional source or header files for your attacker. However, you need to make sure that our lab machine can build your code without errors. We may deduct some points if our lab machine cannot compile your code.

When we grade your code, we will run your `Makefile`s to build your attackers, but we will use our own victims. Therefore, we will ignore your modification to the victim code, and thus your attacks should target the victims we provided to you.

## Acknowledgments

This lab assignment is adapted from the [Cache Side Channel Attacks Lab](https://shd.mit.edu/2026/labs/cache.html) for 6.5950/6.5951 at MIT. The original Dead Drop lab (Part 5 of this lab) was developed by Christopher Fletcher for CS 598CLF at UIUC. The starting code and lab handout are both heavily adapted from his work.