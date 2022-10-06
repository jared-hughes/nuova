// cat non-terminating
p: 0x1500
  a = getchar()
  putchar(a)

  .trash
  .forceA `&p
  .trash
  ip = a
