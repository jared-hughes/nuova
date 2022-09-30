num_lines: 0xDFFEB
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

.pad_ok
print: 0xE00F0
  exit(0)

.pad_ok
// cplus: do c = 10 * c + (a - '0')
cplus: 0xF0000
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
