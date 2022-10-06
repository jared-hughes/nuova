#define glue2(x,y) x##y
#define glue(x,y) glue2(x,y)

#define MS_c(addr) \
  a =;
    .val addr;
  ms(a, c)

#define MS_b(addr) \
  a =;
    .val addr;
  ms(a, b)

p: 0x60000
  .zero_a
  .trash
  c = a

.trash
getc:
  a = getchar()
  b =
    .trash
  if (a <= b) ip =
    .val &cplus

// if (num_lines-- <= 0) jmp exit
.trash
dec_num_lines:
  b =
  num_lines:
    .trash
  c =
    .val 0x01
  a = b - c
  b = a
  
  // for some reason, MS_b(&num_lines) doesn't work here
  a =
    .val &num_lines
  ms(a, b)

  c =
    .val 0xFFFFFFFF
  if (b >= c) ip =
    .val &exit

#define INC_D(di, addr) \
.trash;
glue(inc_,di): addr;
  b =;
  glue(di,_a):;
    .val '0';
  c =;
    .val 0x01;
  a = b + c;
  c = a;
  MS_c(&glue(di,_a));
  MS_c(&glue(di,_b));
  b =;
    .trash;
  MS_b(&glue(di,_filled));

#define CHECK_9 \
  b =;
    .val '9';
  if (b >= c) ip =;
    .val &inc_x3

#define CLEAR_D(di) \
glue(clear_,di):;
  b =;
    .val '0';
  MS_b(&glue(di,_a));
  MS_b(&glue(di,_b));

INC_D(d0, 0x60100) // ++d0
CHECK_9 // if (d0 <= 9) jmp inc x3
CLEAR_D(d0) // fallthrough; d0 = 0

INC_D(d1, 0x60200) // ++d1
CHECK_9 // if (d1 <= 9) jmp inc x3
CLEAR_D(d1) // fallthrough; d1 = 0

INC_D(d2, 0x60300) // ++d2
CHECK_9 // if (d2 <= 9) jmp inc x3
CLEAR_D(d2) // fallthrough; d2 = 0

INC_D(d3, 0x60400) // if (++d3 <= 9) jmp inc x3

// if (++x3 <= 2) jmp inc_x5; else { print "Fizz"; x3=0; do_decimal=0; }
inc_x3: 0x60600
  b =
  x3:
    .val 0x0
  c =
    .val 0x1
  a = b + c
  c = a
  MS_c(&x3)
  b =
    .val 0x2
  if (b >= c) ip =
    .val &inc_x5
fizz:
  .zero_a
  .trash
  c = a
  .trash
  MS_c(&x3)
  .trash
  MS_c(&do_decimal)
  .putchar 'F'
  .putchar 'i'
  .putchar 'z'
  .putchar 'z'

// if (++x5 <= 4) jmp check_decimal; else { print "Buzz"; }
inc_x5: 0x60700
  b =
  x5:
    .val 0x0
  c =
    .val 0x1
  a = b + c
  c = a
  MS_c(&x5)
  b =
    .val 0x4
  if (b >= c) ip =
    .val &check_decimal
buzz:
  .zero_a
  .trash
  c = a
  .trash
  MS_c(&x5)
  .putchar 'B'
  .putchar 'u'
  .putchar 'z'
  .putchar 'z'
  ip =
    .val &endline

// if (do_decimal <= 0) jmp endline
check_decimal: 0x60800
  a =
  do_decimal: 0x60801
    .trash
  b =
    .val 0x0
  if (a <= b) ip =
    .val &endline

// print(d3 d2 d1 d0) without leading 0s

#define PRINT_D(di, no_print_label, addr) \
.trash;
glue(print_,di): addr;
  .zero_a;
  .trash;
  b = a;
  .trash;
glue(read_filled,di): addr + 9;
  a =;
  glue(di,_filled): addr + 10;
    .val 0x0;
  .trash;
  if (a <= b) ip =;
    .val &no_print_label;
  .trash;
  a =;
  glue(di,_b): addr + 16;
    .val '0';
  putchar(a)

PRINT_D(d3, print_d2, 0x60B00)
PRINT_D(d2, print_d1, 0x60C00)
PRINT_D(d1, print_d0, 0x60D00)
PRINT_D(d0, endline, 0x60E00)
  

// print "\n"; do_decimal = .trash; jmp dec_num_lines
.trash
endline: 0x6105D
  .putchar 0x0A
  .trash
  c =
    .trash
  MS_c(&do_decimal)
  .trash
  ip =
    .val &dec_num_lines

.trash
exit: 0x61100
  exit(0)

#define a_plus_equals_c \
  b = a;
  a = b + c

.trash
// cplus: do c = 10 * c + (a - '0')
cplus: 0x611F0
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  a_plus_equals_c
  
  b = a
  c =
    .val '0'
  a = b - c
  c = a

ms_num_lines:
  MS_c(&num_lines)

get_next_char:
  ip =
    .val &getc

// another idea: use cells (array) to avoid duplicating inc & print code.
