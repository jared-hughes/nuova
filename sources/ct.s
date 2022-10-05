// Cyclic Tag

#define glue2(x,y) x##y
#define glue(x,y) glue2(x,y)
#define glue3(x,y,z) glue(glue(x,y),z)

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

#define IP_EQ 0x3F

#define INC_A \
  b = a;
  c =;
    .val 1;
  a = b + c

// PUT "ip =" at pos, ".val &label" at pos+1, and does pos+=2
#define PUT_AT(suffix, pos, label, addr) \
glue3(put_, label, suffix): addr;
  b_eq_global(pos, glue3(put_ret_, label, suffix));
glue3(put_ret_, label, suffix): addr + 0x20;
  a = b;
  b =;
    .val IP_EQ;
  ms(a, b);
  INC_A;
  b =;
    .val &label;
  ms(a, b);
  INC_A;
  b = a;
  global_eq_b(pos)

// === global variables ===

skip_decls: 0xE0000
  ip =
    .val &parse_prod

global_declare(head, .val &start, 0xE0100)
global_declare(tail, .trash, 0xE0200)

// runtime
global_declare(do_append, .trash, 0xE0300)
global_declare(curr, .val &start, 0xE0400)

// === parse input ===

// End result after parsing e.g. "011"10"101;101;
// Note '"' < '0' < '1' < ';'
@ start:
@ curr:
@   pdone p0 p1 p1
@   pdone p1 p0
@   pdone p1 p0 p1 preturn
@ head:
@   d1 d0 d1
@ tail: .trash

parse_prod: 0xE3000
  a = getchar()
  b =
    .val '1'
  if (a <= b) ip =
    .val &leq_1
  PUT_AT(_parse_prod1, head, preturn, 0xE3020)
  .trash
  ip =
    .val &init_head

leq_1: 0xE3100
  b =
    .val '0'
  if (a <= b) ip =
    .val &leq_0
  PUT_AT(_parse_prod2, head, p1, 0xE3120)
  .trash
  ip =
    .val &parse_prod

leq_0: 0xE3200
  b =
    .val '"'
  if (a <= b) ip =
    .val &leq_quote
  PUT_AT(_parse_prod3, head, p0, 0xE3220)
  .trash
  ip =
    .val &parse_prod

leq_quote: 0xE3300
  .trash
  PUT_AT(_parse_prod4, head, pdone, 0xE3320)
  .trash
  ip =
    .val &parse_prod

// after first semicolon
init_head: 0xE3400
  b_eq_global(head, init_head_ret)
init_head_ret: 0xE3420
  global_eq_b(tail)
  // fallthrough to parse_data

parse_data: 0xE5000
  a = getchar()
  b =
    .val '1'
  if (a <= b) ip =
    .val &pd_leq_1
  .trash
  // finish parsing, start exec
  ip =
    .val &done_parsing

pd_leq_1: 0xE5100
  b =
    .val '0'
  if (a <= b) ip =
    .val &pd_leq_0
  PUT_AT(_parse_data1, tail, d1, 0xE5120)
  .trash
  ip =
    .val &parse_data

pd_leq_0: 0xE5200
  .trash
  PUT_AT(_parse_data2, tail, d0, 0xE5220)
  .trash
  ip =
    .val &parse_data

// === global, fixed ===

pdone: 0xF0000
  b_eq_global(head, pdone_ret)
pdone_ret: 0xF0010
  a = b
  ip = a

d0: 0xF0100
  b =
    .val 0
  ip =
    .val &do_db

d1: 0xF0200
  b =
    .val 1
  ip =
    .val &do_db

@ precondition:
@   b = 0 or 1 based on the data bit
@ result:
@   print(b + '0')
@   do_append = b
@   head += 2
@   curr += 2
@   goto curr
do_db: 0xF0300
  c =
    .val '0'
  a = b + c
  putchar(a)
  global_eq_b(do_append)
// fallthrough
inc_head: 0xF0400
  b_eq_global(head, inc_head_ret)
inc_head_ret: 0xF0420
  c =
    .val 2
  a = b + c
  b = a
  global_eq_b(head)
inc_prod: 0xF0440
  ip =
    .val &goto_inc_curr

#define pb_(pb, db, pos, pos_plus_0x60) \
pb: pos;
  b_eq_global(do_append, glue(pb, _ret));
glue(pb, _ret): pos + 0x20;
  a = b;
  .trash;
  b =;
    .val 0;
  .trash;
  if (a <= b) ip =;
    .val &goto_inc_curr;
glue(pb, _put_d): pos + 0x40;
  PUT_AT(_runtime, tail, db, pos_plus_0x60);
  .trash;
  ip =;
    .val &goto_inc_curr

pb_(p0, d0, 0xF0600, 0xF0660)

pb_(p1, d1, 0xF0700, 0xF0760)

goto_inc_curr: 0xF0800
  b_eq_global(curr, goto_inc_curr_ret)
goto_inc_curr_ret: 0xF0820
  c =
    .val 2
  a = b + c
  b = a
  c = a
  global_eq_b(curr)
  a = c
  ip = a

preturn: 0xF0900
  b =
    .val &start
  global_eq_b(curr)
  a = b
  ip = a

// === data block ===

done_parsing: 0xFFFFE
  .trash

start: 0x100000
  .trash
