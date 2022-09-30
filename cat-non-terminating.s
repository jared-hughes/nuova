// cat non-terminating
p: 0x4a00
  a = getchar()
  putchar(a)

  .pad_ok
  .forceA &p
  ip = a
