p: 0xE0000
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
  
  a =
    .val &num_lines
  ms(a, b)

  c =
    .val 0xFFFFFFFF
  if (b >= c) ip =
    .val &exit

#define glue2(x,y) x##y
#define glue(x,y) glue2(x,y)
#define glue3(x,y,z) glue(glue(x,y),z)

#define INC_D(di, addr) \
.trash;
glue(inc_,di): addr;
  b =;
  glue(di,_a):;
    .val 0x0;
  c =;
    .val 0x01;
  a = b + c;
  c = a;
  a =;
    .val &glue(di,_a);
  ms(a, c);
  a =;
    .val &glue(di,_b);
  ms(a, c);
  b =;
    .val 0x9;
  if (b >= c) ip =;
    .val &inc_d_done

#define CLEAR_D(di) \
glue(clear_,di):;
  .zero_a;
  .trash;
  b = a;
  .trash;
  a =;
    .val &glue(di,_a);
  ms(a, b);
  a =;
    .val &glue(di,_b);
  ms(a, b)

INC_D(d0, 0xE0100) // if (++d0 <= 9) jmp inc x3
CLEAR_D(d0) // fallthrough; d0 = 0
INC_D(d1, 0xE0200) // if (++d1 <= 9) jmp inc x3
CLEAR_D(d1) // fallthrough; d1 = 0
INC_D(d2, 0xE0300) // if (++d2 <= 9) jmp inc x3
CLEAR_D(d2) // fallthrough; d2 = 0
INC_D(d3, 0xE0400) // if (++d3 <= 9) jmp inc x3

inc_d_done: 0xE0600
  .trash

@ // if (++x3 <= 2) jmp inc x5; else { print "Fizz"; x3=0; do_decimal=0; }
@ inc_x3:

@ // if (++x5 <= 4) jmp check_decimal; else { print "Buzz"; }
@ inc_x5:

@ // if (do_decimal <= 0) jmp endline
@ check_decimal:

@ // print(d3 d2 d1 d0) without leading 0s
@ print_decimal:

#define PRINT_D(di, addr, addr_plus_one) \
.trash;
glue(print_,di): addr;
  b =;
  glue(di,_b): addr_plus_one;
    .val 0x0;
  c =;
    .val '0';
  a = b + c;
  putchar(a)

PRINT_D(d3, 0xE0800, 0xE0801)
PRINT_D(d2, 0xE0900, 0xE0901)
PRINT_D(d1, 0xE0A00, 0xE0A01)
PRINT_D(d0, 0xE0B00, 0xE0B01)
  

// print "\n"; do_decimal = .trash; jmp dec_num_lines
.trash
endline: 0xE105D
  .putchar 0x0A
  // TODO do_decimal = .trash
  .trash
  ip =
    .val &dec_num_lines

.trash
exit: 0xF0000
  exit(0)

#define a_plus_equals_c \
  b = a;
  a = b + c

.trash
// cplus: do c = 10 * c + (a - '0')
cplus: 0xF00F0
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
  a =
    .val &num_lines
  ms(a, c)

get_next_char:
  ip =
    .val &getc

// golfing idea: store 5 as '5' instead, so wraparound '9' to '0'
// another idea: use cells (array) to avoid duplicating inc & print code.
