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

skip_decls: 0xF0000
  ip =
    .val &get_x

global_declare(x, .val 'a', 0xF0100)

get_x: 0xF0200
  b_eq_global(x, got_x)

got_x: 0xF0300
  a = b
  putchar(a)

set_x: 0xF0400
  b =
    .val 'A'
  global_eq_b(x)

get_x2: 0xF0500
  b_eq_global(x, got_x2)

got_x2: 0xF0600
  a = b
  putchar(a)

exit: 0xF0700
  exit(0)
