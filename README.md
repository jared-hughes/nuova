## "Hi"

Goal:

```c
putchar(0x48);
putchar(0x69);
```

Approach:

```c
a = mg(ip++); // 0x0F
              // 0x48
putchar(a);   // 0xD0
a = mg(ip++); // 0x0F
              // 0x69
putchar(a);   // 0xD0
```

## "Hello, World!\n"

## cat unterminating

## cat, terminating on EOF

## fizbuzz with number of lines as decimal 0-10000

## prove TC
