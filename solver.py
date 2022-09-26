from z3 import *

# import timeit
# T = lambda f: print(timeit.timeit(f, number=1))

u = lambda x: BitVecVal(x, 32)

def P(x):
  x = x ^ LShR(x, 17)
  x = x * u(0xed5ad4bb)
  x = x ^ LShR(x, 11)
  x = x * u(0xac4c1b51)
  x = x ^ LShR(x, 15)
  x = x * u(0x31848bab)
  x = x ^ LShR(x, 14)
  return x

def inverseP(y):
  return sol(X, P(X) == y)

X = BitVec("X", 32)
Y = BitVec("Y", 32)

def sol(conds):
  s = Solver()
  s.add(conds)
  s.check()
  return s.model().eval(X)

# print(sol(P(X)==u(0x01)))

def load(x):
  a = x ^ P(Y)
  return P(a)

def const_output(string: bytes, start: int):
  for c in string:
    y = start
    while True:
      s = Solver()
      s.add(P(X ^ P(u(y + 2))) & u(0xFF) == u(c))
      s.add(X >> 8 == 0)
      if s.check().r == Z3_L_TRUE:
        print("put1(0x00);\n" * (y - start))
        print()
        print(f"// putchar({c}) // => {repr(chr(c))}")
        print(f"put1(0xFF);")
        print(f"put4(0x0F ^ P({y + 1}));")
        print(f"put1({s.model().eval(X)});")
        print(f"put1(0xFF);")
        print(f"put4(0xD0 ^ P({y + 4}));")
        start = y + 5
        break
      y += 1

# const_output(b"Hi", 0)

# const_output(b"Hello, World!\n", 0)
