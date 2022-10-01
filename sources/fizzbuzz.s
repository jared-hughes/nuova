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

// if (++d0 <= 9) jmp inc x3
.trash
inc_d0: 0xE0100
  b =
  d0_a:
    .val 0x0
  c =
    .val 0x01
  a = b + c
  c = a
  a =
    .val &d0_a
  ms(a, c)
  a =
    .val &d0_b
  ms(a, c)
  b =
    .val 0x9
  if (b >= c) ip =
    .val &inc_d_done
// fallthrough to clear this digit and increment next
// d0 = 0
clear_d0:
  .zero_a
  .trash
  b = a
  .trash
  a =
    .val &d0_a
  ms(a, b)
  a =
    .val &d0_b
  ms(a, b)

inc_d_done: 0xE0200
  .trash

@ // if (++d1 <= 9) jmp inc x3
@ inc_d1:

@ // if (++d2 <= 9) jmp inc x3
@ inc_d2:

@ // if (++d3 <= 9) jmp inc x3
@ inc_d3:

@ // if (++x3 <= 2) jmp inc x5; else { print "Fizz"; x3=0; do_decimal=0; }
@ inc_x3:

@ // if (++x5 <= 4) jmp check_decimal; else { print "Buzz"; }
@ inc_x5:

@ // if (do_decimal <= 0) jmp endline
@ check_decimal:

@ // print(d3 d2 d1 d0) without leading 0s
@ print_decimal:

// print(d0)
.trash
print_d0: 0xE0800
  b =
  d0_b: 0xE0801
    .val 0x0
  c =
    .val '0'
  a = b + c
  putchar(a)
  

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
