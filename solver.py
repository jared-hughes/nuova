from z3 import *

# import timeit
# T = lambda f: print(timeit.timeit(f, number=1))

u = lambda x: BitVecVal(x, 32)

def P(x):
  if type(x) != int:
    raise TypeError("P argument should be int. If you want z3, use P_z3")
  x ^= x >> 17
  x *= 0xed5ad4bb
  x &= 0xFFFFFFFF
  x ^= x >> 11
  x *= 0xac4c1b51
  x &= 0xFFFFFFFF
  x ^= x >> 15
  x *= 0x31848bab
  x &= 0xFFFFFFFF
  x ^= x >> 14
  return x

def P_z3(x):
  x = x ^ LShR(x, 17)
  x = x * u(0xed5ad4bb)
  x = x ^ LShR(x, 11)
  x = x * u(0xac4c1b51)
  x = x ^ LShR(x, 15)
  x = x * u(0x31848bab)
  x = x ^ LShR(x, 14)
  return x

X = BitVec("X", 32)
Y = BitVec("Y", 32)

def sol(conds):
  s = Solver()
  s.add(conds)
  s.check()
  return s.model().eval(X)

def const_output(string: bytes):
  global idx
  for c in string:
    while True:
      s = Solver()
      s.add(P_z3(X ^ u(P(idx + 2))) & u(0xFF) == u(c))
      s.add(X >> 8 == 0)
      if s.check().r == Z3_L_TRUE:
        # print(f"// putchar({c}) // => {repr(chr(c))}")
        put1(0xFF)
        put4(0x0F ^ P(idx))
        put1(s.model().eval(X).as_long())
        put1(0xFF)
        put4(0xD0 ^ P(idx))
        break
      put1(0x00)

def exit():
  global idx
  put1(0xFF)
  put4(0xF0 ^ P(idx))

def put1(x):
  global prog, idx
  prog.write(bytes([x]))
  idx += 1

def put4(x):
  global prog, idx
  prog.write(bytes([x >> 24, x >> 16 & 0xFF, x >> 8 & 0xFF, x & 0xFF]))
  idx += 1

try:
  prog = open("prog", "wb")
  idx = 0
  const_output(b"Hi")
  # const_output(b"Hello, World!\n")
  exit()
finally:
  prog.close()
