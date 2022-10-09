// cat non-terminating
p: 0x1500
  a = getchar()
  putchar(a)

  .zero_a
  .trash
  ip = a
