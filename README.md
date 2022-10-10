# Nuova

My solutions to the second esolang reverse engineering contest (Nuova).

## Introduction

The event was based on the language Nuova, defined by its implementation (in this repository as raw-nuova.c). The tasks were as follows:

1. Create a program that prints "Hi" without a linefeed.
2. Create a program that prints "Hello, World!" with a linefeed.
3. Make a `cat` program that does not terminate.
4. Make a `cat` program that terminates on EOF.
5. Make a `fizzbuzz` program that displays a specified amount of sequence items (as a decimal number, 0-10000).
6. Prove that the language is Turing-complete (either by creating an interpreter for a TC language in it or compiling a TC language into it). Assume that the registers and memory are unbounded and provide reasoning behind your proof.

This tasks would be trivial in any practical programming language, but completing fizzbuzz took me over 40 hours of work.

Nuova is a relatively simple language. Each instruction is executed one-by-one in order from start to end. The key difficulty with Nuova is that invalid instructions directly hash all three of the registers (`a`, `b`, and `c`), and the programmer only has full control over every second instruction, so your registers typically get hashed frequently.

## This Repository

- `raw-nuova.c`: the definitive Nuova interpreter. Run with `bin/prog` as the input via `make run-raw-nuova`
- `nuova.c`: a formatted version with debugging statements (prints to `logs/nuova.log`), memory limit, and runtime limit. Run with `bin/prog` as the input via `make run-nuova`
- `sources/*.s`: my solutions to the tasks, in an assembly-like format
- `compiler.c`, `compiler.h`: my primary compiler for generating Nuova code. Generate `bin/prog` (Nuova bytecode) for a particular program using `make run-compiler prog=cat-terminating`.
- `solver.py`: my first attempt, using Z3 (Z3 is no faster than exhaustive search since the hash has no useful patterns)

## Hash details

The hash itself is a three-round multiply-xorshift hash, named `triple32` from [Hash Prospector](https://github.com/skeeto/hash-prospector), which notes "this hash function is indistinguishable from a perfect PRF (e.g. a random permutation of all 32-bit integers)." Nuova names this function `P`. Fortunately, since `P` is collisionless, it is invertible. Also `P(0) = 0`, which has some niche use.

## Instructions

Program and data is stored on the same "unbounded" (bounded by 32 bit integer addressing) tape of unsigned 32-bit integers (call it `mem`). In each step, the memory at the instruction pointer (initially 0) is read like `instruction = mem[ip]`, and the instruction pointer is incremented. If that instruction is one of the 23 valid instructions, then the corresponding statement occurs (described below) and the next step begins. Otherwise, all three of the registers (`a`, `b`, and `c`) get hashed with `P`), and the next step begins.

Three instructions are load-immediate

```c
0x0F: a = mem[ip++];
0x1F: b = mem[ip++];
0x2F: c = mem[ip++];
```

Three instructions are jump-immediate. `0x3F` is unconditional. `0x4F` and `0x5F` are conditional, but note they also conditionally increment the instruction pointer (so a fall-through will typically end up hashing the registers)

```c
0x3F: ip = mem[ip++];
0x4F: if (a <= b) ip = mem[ip++];
0x5F: if (b >= c) ip = mem[ip++];
```

Three instructions are memset immediate. After setting the memory (in the cell immediately after the instruction), they increment the instruction pointer again

```c
0x00: mem[ip] = a; ip++;
0x10: mem[ip] = b; ip++;
0x20: mem[ip] = c; ip++;
```

Three instructions are random-access memset:

```c
0x30: mem[a] = b;
0x40: mem[a] = c;
0x50: mem[a] = mem[a]; // ??
```

Four instructions move registers through `a`:

```c
0x60: b = a;
0x70: c = a;
0x80: a = b;
0x90: a = c;
```

One instruction is an unconditional jump to register

```c
0xA0: ip = a;
```

Two instructions do arithmetic using `b` and `c` as arguments, with `a` as the result (I wish there was `a += b`):

```c
0xB0: a = b + c;
0xC0: a = b - c;
```

Two instructions do I/O on individual characters. Note `putchar` only cares about the lower 8 bits, and `getchar` returns `-1 = 0xFFFFFFFF` for EOF.

```c
0xD0: putchar(a);
0xE0: a = getchar();
```

There is one instruction to exit cleanly, like `exit(0);` in C, or `return 0;` in the main function in C.

```c
0xF0: exit(0);
```

We don't talk about `0x6F`.

Any other memory value (whenever you have garbage data) gets treated as `a = P(a); b = P(a); c = P(c);`

Note there is no instruction for reading from an arbitrary memory address.

## Startup memory

The Nuova interpreter takes a single binary file as input, but its operation depends on 32-bit integers in all registers. What gives?

For the most part, at program start, `mem[i] = P(i) ^ c`, where `c` is the `i`th byte of the input file. Since each byte is only one byte, and the result of the hash `P(i)` can be any 32-bit value, the programmer really only has control over the bottom byte of the startup memory. The top three bytes are all exactly the top three bytes of `P(i)`.

Unfortunately the instructions rely on the entire 32-bit value of `instruction = mem[ip]` being a correct value, so the top three bytes must be 0. To allow the Nuova programmer control over this, Nuova has a mechanism for setting 32-bit values. If the `i`th byte of the input file meets `c & 0xF = 0xF` (so the bottom nibble is set), then the next four bytes are read as a 32-bit integer `v`, to set `mem[i + 1] = P(i + 1) ^ v`. Thus the programmer has full control over `mem[i + 1]` by choosing the four bytes such that `v = P(i + 1) ^ k`, to get `mem[i + 1] = k`.

We don't talk about the last cell that gets set.

## Example (`a = 42`)

Suppose the input file is the sequence of bytes `0F 04 27 41 fc`.

1. Nuova starts by setting `mem[0] = P(0) ^ 0x0F`. Fortunately, `P(0) = 0`, so `mem[0] = 0x0F = 15`.
2. Since the byte `0x0F` has bottom nibble set, Nuova reads the next four bytes to set `mem[1] = P(1) ^ 0x042741fc`. I chose that value carefully, since `P(1) = 0x42741d6`, so `mem[1] = 0x042741fc ^ 0x42741d6 = 0x2a = 42`.

The end result of all this is that `mem[0] = 0x0F` and `mem[1] = 42`. So Nuova executes as follows:

1. Read `mem[0] = 0x0F` and conclude that it's time to do `a = mem[ip++]`
2. Read `mem[1] = 42` to set `a = 42`.
3. End with instruction pointer `ip = 2`

## Example (`putchar('h')`)

A consequence of Nuova's input is that the programmer has no control over a cell of the startup memory unless the previous cell is initialized with a byte whose lowest nibble is `0xF`. Hence the bytecode input really only has control over every second cell, so hashing is inevitable.

So to write a program whose effect is `putchar('h')`, we dont start with `a = 104 = 0x68 = 'h'` like in the previous section. We start with `a = 0xb7cd2d00` because we expect it to get hashed once.

Hence a full terminating program to print `h` looks like

```
0F B3EA6CD6 FF C0F0B597 FF E33DE5D1
```

I've grouped together four bytes when they get read as a set of four bytes. After the initial processing, the startup memory looks as follows:

```c
mem[0] = 0x0F;       // a = mem[1]; ip = 2;
mem[1] = 0xB7CD2D00;
mem[2] = 0xF1DFE816; // a = P(a); b = P(b); c = P(c);
mem[3] = 0xD0;       // putchar(a);
mem[4] = 0xD3A15F6A; // a = P(a); b = P(b); c = P(c);
mem[5] = 0xF0;       // exit(0);
```

Execution proceeds as:

1. `ip = 0`. Instruction `0x0F` is read at `mem[0]`, so `a = mem[ip++]` is executed. Then `a = 0xB7CD2D00`.
2. `ip = 2`. Since `0xF1DFE816` is not a valid instruction, all the registers get hashed, so `a = P(0xB7CD2D00) = 0x68 = 'h'`. We were able to force this by selecting `mem[1] = inverseP(0x68)`.
3. `ip = 3`. Instruction `0xD0` is read, so `putchar(a)`. We get `h` printed to stdout!
4. `ip = 4`. Hash all the registers.
5. `ip = 5`. Instruction `0xF0` is read, so `exit(0);`, terminating cleanly.

## Task 1 (`printf("hi")`)

The previous example (`putchar('h')`) is so short because `P(0) = 0`, so the bytecode has full control over the first two memory locations. Since we want to follow up with `putchar('i')`, it might seem like a tall task because it's hard in general to get memory to initialize such that `mem[i] = 0x0F` and `mem[i + 1] = inverseP('h')`. This was used earlier at `i = 0` to start with `a = inverseP('h')` in order to `putchar(P(mem[i + 1])) = putchar(P(inverseP('h'))) = putchar('h')`.

But now we want to `putchar(P(mem[i + 1]))` while only having control over the bottom byte of `mem[i + 1]`. But this isn't too bad because we only care about the bottom byte of `P(mem[i + 1])` since `putchar(x) ≡ putchar(x & 0xF)`.

Exhaustive search gives three options here:

```py
>>> [hex(c) for c in range(256) if P(c ^ P(6)) & 0xFF == ord('i')]
['0x9f', '0xae', '0xec']
```

Using `0x9F` would mess up input by treating the next four bytes as an integer, so let's choose `0xAE` and write the rest of the hi program around it:

```
0F B3EA6CD6 FF C0F0B597
FF E33DE52E AE FF 42CF8F4F
FF 082C8EEA
```

This leads the the following startup memory:

```c
// putchar('h')
// 0F B3EA6CD6 FF C0F0B597
mem[0] = 0x0F;       // a =
mem[1] = 0xB7CD2D00; //   0xB7CD2D00
mem[2] = 0xF1DFE816; // a = P(a); b = P(b); c = P(c);
mem[3] = 0xD0;       // putchar(a);
// putchar('i')
// FF E33DE52E AE FF 42CF8F4F
mem[4] = 0xD3A15F6A; // a = P(a); b = P(b); c = P(c);
mem[5] = 0x0F;       // a =
mem[6] = 0xA6385F3F; //   0xA6385F3F
mem[7] = 0x4F25D266; // a = P(a); b = P(b); c = P(c);
mem[8] = 0xD0;       // putchar(a);
// exit(0)
// FF 082C8EEA
mem[9] = 0x8ED47961; // a = P(a); b = P(b); c = P(c);
mem[10] = 0xF0;      // exit(0);
```

The execution starts as before with `putchar('h')`. Then:

1. `ip = 4`: hash all the registers
2. `ip = 5`: Instruction `0x0F`, so `a = mem[ip++] = 0xA6385F3F`
3. `ip = 7`: hash all the registers, so `a = 0x91CEC869`
4. `ip = 8`: Instruction `0xD0`, so `putchar(a)`. This is equivalent to `putchar(0x69)`, which emits an `i` character (byte) to stdout.

Then execution wraps up with the same `exit(0)` approach as the previous example.

## Task 2 (`printf("Hello, World!\n");`)

The `"Hello, World\n"` task is pretty similar to `"hi"` except there's more characters to print. For all the letters after `H`, use the same 11-byte, 5-cell pattern:

1. (cell `i`) Trash input byte `0xFF` to control the next cell
2. (cell `i+1`) Four bytes to force `0x0F` (`a = mem[ip++]`) in the memory
3. (cell `i+2`) Careful byte `c` such that `P(c ^ P(i + 2)) & 0xFF == 'e'` where `'e'` is the byte you're trying to print and varies from `'e'` to `'\n'`. Note we must avoid `c & 0xF == 0xF` to avoid interpreting the following four bytes as a new cell.
4. (cell `i+3`) Trash input byte `0xFF` to control the next cell
5. (cell `i+4`) Four bytes to fource `0xD0` (`putchar(a)`) in the memory

Note this isn't possible for all starting indices `i`: there are only `256 - 16 = 240` options for `c` in step 3, and there are some collisions, so you might have to increment `i` until a working `c` is possible to choose.

[[TODO put full binary of Hello World here]]

## The compiler

Before we get to more complicated stuff, we're going to need some more notation. To help write out the solutions, I made an assembler to convert some human-readable code into Nuova bytecode.

For example, the "hi" program above can be written as

```c
  .putchar 'h'
  .putchar 'i'
```

Ok I cheated a bit there because I made putchar a builtin directive for the assembler because it needs to look for the right index. Using other features, the same program can be written as

```c
start: 0
// putchar('h')
  a =
    .val 0xB7CD2D00
  .trash
  putchar(a)
// putchar('i');
  .trash
  a =
    .val 0xA6385F3F
  .trash
  putchar(a)
// exit(0);
  .trash
  exit(0)
```

There's a lot to unpack here. The code starts with `start: 0`, which defines a label called "start" which is absolute-positioned at cell 0.

The next non-comment line is `a =`, which is a mnemonic for the `0x0F` instruction `a = mem[ip++]`. The full list of 23 mnemonics follows. Mnemonics must be written exactly as given; no spaces removed.

<details>
  <summary> <b>Full list of mnemonics </b> </summary>

- 0x0F: `a =`
- 0x1F: `b =`
- 0x2F: `c =`
- 0x3F: `ip =`
- 0x4F: `if (a <= b) ip =`
- 0x5F: `if (b >= c) ip =`
- 0x6F: `a = sse(...)`
- 0x00: `ms(ip, a)`
- 0x10: `ms(ip, b)`
- 0x20: `ms(ip, c)`
- 0x30: `ms(a, b)`
- 0x40: `ms(a, c)`
- 0x50: `ms(a, mg(a))`
- 0x60: `b = a`
- 0x70: `c = a`
- 0x80: `a = b`
- 0x90: `a = c`
- 0xA0: `ip = a`
- 0xB0: `a = b + c`
- 0xC0: `a = b - c`
- 0xD0: `putchar(a)`
- 0xE0: `a = getchar()`
- 0xF0: `exit(0)`
</details>

Next is `.val 0xB7CD2D00` which is a literal value.

After that is `.trash`, which specifies a literal `0xFF` byte in the input bytecode. This allows the next cell to be forced exactly using four bytes.

The source file continues with `putchar(a)`, another mnemonic. Once the assembler reaches the end, it ends up emitting the same bytecode that we handwrote:

```
0F B3EA6CD6 FF C0F0B597
FF E33DE52E AE FF 42CF8F4F
FF 082C8EEA
```

## Force A

We got lucky in "hi" and "hello world" because you don't need much control on a putchar value. But for all the harder tasks, we're going to need some way to have full control over all 4 bytes of a register.

One way we accomplish this is by combining registers we have low control over to get a register with high control. For example, if we can vary one byte in the input file to get 240 choices for `b`, and vary another byte to get 240 choices for `c`, then running `a = b + c` would give about `240 * 240 = 57600` choices for `a` (slightly less due to collisions). This is slightly less than 2 bytes of control. To get close to a full 4 bytes of control, we run `b = a`, vary a byte to get 240 choices for `c`, then again run `a = b + c` to get a little less than 3 bytes of control. Repeating one more time gives almost 4 bytes of control, which allows for a successful exchaustive search for most start indices.

In the assembly language, this would be written approximately

```c
// example: start at 1
start_pos: 1
  .trash
  b =
    .val ?? // 240 choices here
  .trash
  c =
    .val ?? // 240 choices here
  .trash
  a = b + c
  .trash
  b = a
  .trash
  c =
    .val ?? // 240 choices here
  .trash
  a = b + c
  .trash
  b = a
  .trash
  c =
    .val ?? // 240 choices here
  .trash
  a = b + c
```

The `??` are stand-ins for actual values.

This exhaustive search ends up covering about 3/4 of the possible 32-bit output values. If the search fails, then increment the start index and try again. This is forceA and can be written via the directive `.forceA value`.

(The exhaustive search is a little slow since it needs to search a few billion options on average, so the assembler caches the results in `cache/tsfav`)

## Example: infinite loop

To get an infinite loop, we need to set the `ip` back to an earlier position. We can uselessly get an empty infinite loop via

```c
start: 0
  ip =
    .val 0
```

To get an infinite loop with a body, we can use the `ip = a` (jump-to-register) instruction:

```c
loop_start: 0x10
  // loop body would go here
loop_end: 0x50
  .trash
  .forceA `0x10
  .trash
  ip = a
```

Here, the `` ` `` denotes `inverseP`, so `` `0x10 `` is the same as the value `0x1CDB46F2` since `P(0x1CDB46F2) == 0x10`. The first `.trash` is just to deal with an assembler bug, and the second `.trash` allows for four bytes to force the value of `ip = a`.

When the program execution finishes the instructions generated by the `.forceA` directive, we have `a = 0x1CDB46F2`. The `.trash` instruction then hashes the registers to get `a = 0x10`. Then `ip = a` unconditionally jumps back to `loop_start`.

To familiarize you with another assembler syntax, `&label` refers to the cell index of `label`. Also, values support addition (at most one) or subtraction (at most one), so the above loop can be written as

```c
loop_start: 0x10
  // loop body would go here
loop_end: &loop_start + 0x40
  .trash
  .forceA `&loop_start
  .trash
  ip = a
```

## `initMemset`

We're working towards a cat program, which requires `a = getchar();` and `putchar(a);` in consecutive instructions. This is difficult in general. The way to do this is by setting the memory to force instructions.

For example, it would be nice for something like this to work:

```c
start: 0x10
  // 0xD0 = "putchar(a)"
  .forceA `0xD0
  b = a
  .forceA `&putchar
  .trash
  ms(a, b)

getchar: 0x50
  a = getchar()
putchar: &getchar + 1
  .trash
```

The wishful thinking here is that at the point of the `ms(a, b)` instruction, we'll have `a = &putchar = 0x51`, and `b = 0xD0` (the opcode for `putchar(a)`). The instruction `ms(a, b)` aka `mem[a] = b` would overwrite the final `.trash` to put a `putchar(a)` in the cell after `a = getchar()`.

But unfortunately this doesn't work out. The `.forceA` directive overwrites all three registers, so it is not useful twice in a row.

Instead, we take advantage of the hashing capabilities. We just want to use `ms(a, b)` eventually with the right `a` and `b`. So if `b` isn't the right value now, we can just wait until it gets hashed to be the right value. So the general plan is

````````````c
  // whatever number of inverses is needed
  .forceA ```````````&putchar
  .trash
  b =
    .val ?? // 240 choices here
  .trash
  .trash
  // however many hashes are needed (up to 16 million ish)
  .trash
  .trash
  ms(a, b)
````````````

Since we have about 8 bits of control over the initial value of `b`, the "however many hashes are needed" might take up to 24 bits worth of waiting, so 2^24 = 16 million. This is a bit too long for our purposes, but we have another axis of control: when this whole sequence starts.

````````````c
  .trash
  .trash
  // however much padding is needed (up to 4000 ish)
  .trash
  .trash
  // whatever number of inverses is needed
  .forceA ```````````&putchar
  .trash
  b =
    .val ?? // 240 choices here
  .trash
  .trash
  // however many hashes are needed (up to 4000 ish)
  .trash
  .trash
  ms(a, b)
````````````

Now, exhaustive search through about 4000 starting positions, 240 initial `b` values, and 4000 hash counts gives about 2^32 options, which is enough to control `b` at the point of `ms(a, b)`.

(This is slow implemented naively, so the assembler accelerates it by using a hash set to store a bunch of inverses of the goal `b` value).

This is represented in assembly by putting two mnemonics next to each other without `.trash` in between. So getchar-putchar is represented by

```c
  a = getchar()
  putchar(a)
```

The first mnemonic will be inserted by `ms_simple`, the easy process where the bytecode has a `0xFF` byte, then four bytes to control the instruction cell. The second mnemonic will be inserted by `initMemset`, the complicated 4000-240-4000 approach above.

## Task 3: `cat` (non-terminating)

Using `initMemset`, non-terminating cat is easy to write

```c
// cat non-terminating
p: 0x1500
  a = getchar() // ms_simple
  putchar(a)    // initMemset

  .trash
  .forceA `&p
  .trash
  ip = a
```

The assembler puts the `initMemset` block somewhere before `0x1500`, which gives about 5000 total cells of freedom. This works out even though it's less than 4000+240+4000.

The program ends with the same infinite loop as [the infinite loop example](#example-infinite-loop).

### Golfing non-terminating cat

To save 47 bytes, the loop at the end can be swapped out for `ip = 0` using

```c
  b = a
  .trash
  c = a
  .trash
  a = b - c
  .trash
  ip = a
```

This is much shorter than a general `.forceA`. It relies on `P(0) = 0`. The assembler also defines a directive to zero `a` which corresponds to exactly the above code:

```c
  .zero_a
  .trash
  ip = a
```

## Task 4: `cat` (terminating)

Terminating `cat` is similar to non-terminating `cat`. It just needs to check for EOF using a conditional jump.

```c
p: 0x4A00
  a = getchar()
  if (a <= b) ip =
    .val &not_eof

  .trash
  exit(0)

.trash
not_eof: &p + 8
  putchar(a)

  .forceA `&p
  .trash
  ip = a
```

Since EOF is the maximum 32-bit integer `0xFFFFFFFF`, pretty much all integers compare less than it, so we don't really need to initialize `b`, just ensure that it is not in a cycle including `0xFFFFFFFF`.

## Example: global variables

Nuova does not have a random-access memory get instruction, so memory must be read where it is used.

For a demonstration, let's reproduce the following program that prints out all the ascii characters in a cycle without terminating, starting at `!` and cycling due to overflow.

```c
start: 0x10000
  a =
    .val '!'
loop_begin:
  putchar(a)
  // increment register a
  c =
    .val 1
  b = a
  a = b + c
  ip =
    .val &loop_begin
```

This works correctly, but it requires an unbroken chain where `a` never gets overwritten. For Fizz Buzz, we will need to store more variables, so we store variables in the actual tape memory.

### Method 1: read-immediate

Let's demonstrate by storing the current character in memory at address `0x10001` (labelled with `x` below).

```c
loop_begin: 0x10000
  a =
  x:
    .val '!'
  putchar(a)
  // b ← a + 1
  c =
    .val 1
  b = a
  a = b + c
  b = a
  // set the value of x to be a
  a =
    .val &x
  ms(a, b)
  // loop back to start
  // overwriting register a is ok
  .trash
  a =
    .val `&loop_begin
  .trash
  ip = a
```

This means that the value of `x` can only be read at the beginning of the loop. If you need to read it in two locations, the program has to also write the value to those two locations, see [Method 1 with several reads](#method-1-with-several-reads).

### Method 1 with several reads

The fizzbuzz program linked here at `fizzbuzz.s` uses the method of writing the value to those multiple locations. For example, fizzbuzz needs to read the ones digit `d0` in two places: once for incrementing the counter, and once for printing out the counter. So it stores two copies of the value at different addresses `d0_a` and `d0_b`.

When `d0`, it reads from `d0_a` and writes to `d0_a` and `d0_b`:

```c
// increment the ones digit d0
inc_d0: 0x35100
  // read the current value from the address labelled d0_a
  b =
  d0_a:
    .val '0'
  // add one
  c =
  .val 0x01
  a = b + c
  c = a
  // set d0_a to the new value of d0
  a =
    .val &d0_a
  ms(a, c)
  // set d0_b to the new value of d0
  a =
    .val &d0_b
  ms(a, c)
```

When printing `d0`, it reads from `d0_b`:

```c
a =
  d0_b: 0x35E10
    .val '0'
  putchar(a)
```

When zeroing `d0`, it writes to `d0_a` and `d0_b`:

```c
  b =
    .val '0'
  a =
    .val &d0_a
  ms(a, b)
  a =
    .val &d0_b
  ms(a, b)
```

### Macros for Method 1

For Method 1, it helps to define a macro to simplify the memory operations. The `make run-compiler` uses `gcc -E` to expand macros. Unfortunately, it replaces newlines with spaces, so as a hack, write `;` instead of `\` when you want the macro to continue onto several lines (`make run-compiler` replaces `;` with `;\`, then runs `gcc -E`, then replaces `;` with newlines again).

```c
#define MS_b(addr) \
  a =;
    .val addr;
  ms(a, b)
```

Then the previous code block can be written as

```c
  b =
    .val '0'
  MS_b(&d0_a)
  MS_b(&d0_b)
```

### Method 2: read and callback

Another way to implement global variables is by storing their value in one place and reading them through callbacks.

```c
// skip ahead past the x_b_eq declaration
skip_decls: 0xE0000
  ip =
    .val &loop_begin

// declare variable x
// it gets accessed by jumping to x_b_eq with the callback in register a
x_b_eq: 0xE1000
  b =
  x: 0xE1001
    .val '!'
  ip = a

loop_begin: 0xF0000
  // setup callback got_x
  a =
    .val &got_x
  // jump to x_b_eq
  ip =
    .val &x_b_eq
  // x_b_eq will jump back to got_x with the value of x in register b
got_x: 0xF0010
  a = b
  putchar(a)
  // b ← a + 1
  c =
    .val 1
  a = b + c
  b = a
  // Set the value of x to be b
  a =
    .val &x
  ms(a, b)
  // loop back to start
  // overwriting register a is ok
  .trash
  ip =
    .val &loop_begin
```

This method extends to reading in several places, as long as the callback values are set correctly.

### Macros for Method 2

For Method 2, it helps to define some macros to simplify the memory operations.

```c
#define glue2(x,y) x##y
#define glue(x,y) glue2(x,y)

#define global_declare(var, value, pos) \
  glue(var, _b_eq): pos;
  b =;
  var: pos + 1;
    value;
  ip = a

#define b_eq_global(var, return_idx) \
  a =;
    .val &return_idx;
  ip =;
    .val &var - 1

#define global_eq_b(var) \
  a =;
    .val &var;
  ms(a, b)
```

Then the ASCII loop code can be written as

```c
skip_decls: 0xE0000
  ip =
    .val &loop_begin

global_declare(x, .val '!', 0xE1000)

loop_begin: 0xF0000
  b_eq_global(x, got_x)
got_x: 0xF0010
  a = b
  putchar(a)
  // b ← a + 1
  c =
    .val 1
  b = a
  a = b + c
  b = a
  // put b into x
  global_eq_b(x)
  // loop back to start
  // overwriting register a is ok
  .trash
  ip =
    .val &loop_begin
```

## Task 5: Fizz Buzz

Since Fizz Buzz only needs to go up to four digits numbers (10000 is buzz), it's reasonable to just store the digits in four separate memory locations instead of an aray.
