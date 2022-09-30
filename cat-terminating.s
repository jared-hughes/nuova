// cat terminating
p: 0x4A00
  b =
    .trash
  a = getchar()
  if (a <= b) ip =
    .val &not_eof

  .trash
  exit(0)

.trash
not_eof: 0x4A08
  putchar(a)

  .forceA &p
  ip = a
