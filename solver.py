from z3 import *

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

# import timeit
# T = lambda f: print(timeit.timeit(f, number=1))

# H
solve(load(X) & u(0xFF) == u(0x48), X >> 8 == 0, 2 <= Y, Y < 5)

# i
solve(load(X) & u(0xFF) == u(0x69), X >> 8 == 0, 8 <= Y, Y < 12)
