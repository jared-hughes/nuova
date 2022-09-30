// cat terminating
p: 0x4A00
  b =
    .trash
  a = getchar()
  if (a <= b) ip =; else PPP
    .val &not_eof

  .pad_ok
  exit(0)

.pad_ok

not_eof: 0x4A08
  putchar(a)

  .pad_ok
  .forceA &p
  ip = a
