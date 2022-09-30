p: 0xE0000
  b = a
  .pad_ok
  c = a
  .pad_ok
  a = b - c
  .pad_ok
  c = a

.pad_ok

getc:
  a = getchar()
  b =
    .trash
  if (a <= b) ip =
    .val &cplus

// if (num_lines-- <= 0) jmp exit
dec_num_lines:
  .trash
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
  if (b <= c) ip =
    .val &exit

.pad_ok
test_putchar_loop:
  a =
    .val 0x61
  putchar(a)

  .pad_ok
  ip =
    .val &dec_num_lines


@ // if (++d0 <= 9) jmp inc x3
@ inc_d0:

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

@ // print "\n"; do_decimal = .trash; jmp dec_num_lines
@ endline:

.pad_ok
exit: 0xF0000
  exit(0)

.pad_ok
// cplus: do c = 10 * c + (a - '0')
cplus: 0xF00F0
  // BEGIN a = 10*c + a
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  b = a
  a = b + c
  // END
  b = a
  c =
    // '0'
    .val 0x30
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
