# Nuova

My solutions to the second esolang reverse engineering contest (Nuova).

## Introduction

The event was based on the language Nuova, defined by its implementation (in this repository as raw-nuova.c). The tasks were as follows:

1. Create a program that prints "Hi" without a linefeed.
2. Make a `cat` program that does not terminate.
3. Create a program that prints "Hello, World!" with a linefeed.
4. Make a `cat` program that terminates on EOF.
5. Make a `fizzbuzz` program that displays a specified amount of sequence items (as a decimal number, 0-10000).
6. Prove that the language is Turing-complete (either by creating an interpreter for a TC language in it or compiling a TC language into it). Assume that the registers and memory are unbounded and provide reasoning behind your proof.

This tasks would be trivial in any practical programming language, but completing fizzbuzz took me over 40 hours of work.

Nuova is a relatively simple language. Each instruction is executed one-by-one in order from start to end. The key difficulty with Nuova is that invalid instructions directly hash all three of the registers (`a`, `b`, and `c`), and the programmer only has full control over every second instruction, so your registers typically get hashed frequently.

## This Repository

- `raw-nuova.c`: the definitive Nuova interpreter. Run with `bin/prog` as the input via `make run-raw-nuova`
- `nuova.c`: a formatted version with debugging statements (prints to `logs/nuova.log`), memory limit, and runtime limit. Run with `bin/prog` as the input via `make run-nuova`
- `sources/*.s`: my solutions to the tasks, in an assembly-like format
- `compiler.c`, `compiler.h`: my primary compiler for generating Nuova code. Generate `bin/cat-terminating` (Nuova bytecode) using `make run-compiler prog=cat-terminating`.
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

```s
  .putchar 'h'
  .putchar 'i'
```

Ok I cheated a bit there because I made putchar a builtin directive for the assembler because it needs to look for the right index. Using other features, the same program can be written as

```s
start: 0
# putchar('h')
  a =
    .val 0xB7CD2D00
  .trash
  putchar(a)
# putchar('i');
  .trash
  a =
    .val 0xA6385F3F
  .trash
  putchar(a)
# exit(0);
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
