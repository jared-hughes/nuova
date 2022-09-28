from z3 import *

import timeit, time

T = lambda f: print(timeit.timeit(f, number=1))

u = lambda x: BitVecVal(x, 32)


def P(x):
    if type(x) != int:
        raise TypeError("P argument should be int. If you want z3, use P_z3")
    x ^= x >> 17
    x *= 0xED5AD4BB
    x &= 0xFFFFFFFF
    x ^= x >> 11
    x *= 0xAC4C1B51
    x &= 0xFFFFFFFF
    x ^= x >> 15
    x *= 0x31848BAB
    x &= 0xFFFFFFFF
    x ^= x >> 14
    return x


def inverseP(x):
    if type(x) != int:
        raise TypeError("P argument should be int. If you want z3, use P_z3")
    x ^= x >> 28 << 14
    x ^= (x & 0x0FFFFFFF) >> 14
    x *= 0x32B21703
    x &= 0xFFFFFFFF
    x ^= x >> 30 << 15
    x ^= (x & 0x3FFFFFFF) >> 15
    x *= 0x469E0DB1
    x &= 0xFFFFFFFF
    x ^= x >> 22 << 11
    x ^= (x & 0x003FFFFF) >> 11
    x *= 0x79A85073
    x &= 0xFFFFFFFF
    x ^= x >> 17
    return x


def P_z3(x):
    x = x ^ LShR(x, 17)
    x = x * u(0xED5AD4BB)
    x = x ^ LShR(x, 11)
    x = x * u(0xAC4C1B51)
    x = x ^ LShR(x, 15)
    x = x * u(0x31848BAB)
    x = x ^ LShR(x, 14)
    return x


A, B, C, D, X, Y, I = BitVecs("A B C D X Y I", 32)
A1, B1, C1, A2, B2, C2, A3, B3, C3, A4, B4, C4 = BitVecs(
    "A1, B1, C1, A2, B2, C2, A3, B3, C3, A4, B4, C4".replace(",", ""), 32
)
E, F = BitVecs("E F", 8)


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
            s.add(X & 15 != 15)
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
    # print(hex(idx), "put1", f"{x:02x}")
    prog.write(bytes([x]))
    idx += 1


def put4(x):
    global prog, idx, clr
    # print(hex(idx), "put4", f"{x:08x}")
    prog.write(bytes([x >> 24, x >> 16 & 0xFF, x >> 8 & 0xFF, x & 0xFF]))
    idx += 1


def hi():
    const_output(b"Hi")


def hello_world():
    const_output(b"Hello, World!\n")


def full(value):
    s = Solver()
    s.add(LShR(A, 8) == 0)
    s.add(LShR(B, 8) == 0)
    s.add(LShR(C, 8) == 0)
    s.add(LShR(D, 8) == 0)
    b = A ^ P_z3(u(idx + 2))
    b = P_z3(b)
    c = B ^ u(P(idx + 5))
    b = P_z3(b)
    c = P_z3(c)
    a = b + c
    a = P_z3(a)
    b = P_z3(b)
    c = P_z3(c)

    b = a
    a = P_z3(a)
    b = P_z3(b)
    c = P_z3(c)
    c = C ^ u(P(idx + 12))
    a = P_z3(a)
    b = P_z3(b)
    c = P_z3(c)
    a = b + c

    b = a
    a = P_z3(a)
    b = P_z3(b)
    c = P_z3(c)
    c = D ^ u(P(idx + 18))
    a = P_z3(a)
    b = P_z3(b)
    c = P_z3(c)
    a = b + c
    s.add(a == value)


def intermediates(value):
    s = Solver()
    s.add(A >> 8 == 0)
    s.add(B >> 8 == 0)
    s.add(C >> 8 == 0)
    s.add(D >> 8 == 0)
    s.add(
        B1 == P_z3(P_z3(A ^ u(P(idx + 2)))),
        C1 == P_z3(B ^ u(P(idx + 5))),
        B2 == P_z3(B1),
        A1 == P_z3(B2 + C1),
        B3 == P_z3(P_z3(A1)),
        C2 == P_z3(C ^ u(P(idx + 12))),
        A2 == B3 + C2,
        B4 == P_z3(P_z3(A2)),
        C3 == P_z3(D ^ u(P(idx + 18))),
        A3 == B4 + C3,
        A3 == value,
    )


def get_solver(value):
    s = Solver()
    s.set("local_ctx", True)
    s.add(LShR(A, 8) == 0)
    s.add(LShR(B, 8) == 0)
    # s.add(LShR(C, 8) == 0)
    # s.add(LShR(D, 8) == 0)
    # s.add(A == a0, B == b0)
    b = A ^ P_z3(u(idx + 2))
    # b = u(0 ^ P(idx + 2)) # slower
    b = P_z3(b)
    c = B ^ u(P(idx + 5))

    b = P_z3(b)
    c = P_z3(c)
    a = b + c
    # a = P_z3(a)
    # b = P_z3(b)
    # c = P_z3(c)

    # b = a
    # a = P_z3(a)
    # b = P_z3(b)
    # c = P_z3(c)
    # c = C ^ u(P(idx + 12))
    # a = P_z3(a)
    # b = P_z3(b)
    # c = P_z3(c)
    # a = b + c

    # b = a
    # a = P_z3(a)
    # b = P_z3(b)
    # c = P_z3(c)
    # c = D ^ u(P(idx + 18))
    # a = P_z3(a)
    # b = P_z3(b)
    # c = P_z3(c)
    # a = b + c
    s.add(a == value)
    return s


def get_model(value):
    s = get_solver(value)
    if s.check().r == Z3_L_TRUE:
        print("sat", s.model())
        return s.model()
    print("unsat")
    return None


def set_a(value):
    while True:
        print(idx)

        t = time.time()
        m = get_model(value)
        print(time.time() - t)

        print("model constructed")
        if m is not None:
            print(m)
            print("True")
            # i = idx
            put1(0xFF)
            put4(0x1F ^ P(idx))  # b =
            put1(m.eval(A).as_long())  # 0xab ^ P(i + 2)
            put1(0xFF)  # P()
            put4(0x2F ^ P(idx))  # c =
            put1(m.eval(B).as_long())  # 0xcd ^ P(i + 5)
            put1(0xFF)  # P()
            put4(0xB0 ^ P(idx))  # a = b + c

            put1(0xFF)  # P()
            put4(0x60 ^ P(idx))  # b = a
            put1(0xFF)  # P()
            put4(0x2F ^ P(idx))  # c =
            put1(m.eval(C).as_long())  # 0xef ^ P(i + 12)
            put1(0xFF)  # P()
            put4(0xB0 ^ P(idx))  # a = b + c

            put1(0xFF)  # P()
            put4(0x60 ^ P(idx))  # b = a
            put1(0xFF)  # P()
            put4(0x2F ^ P(idx))  # c =
            put1(m.eval(D).as_long())  # 0xef ^ P(i + 19)
            put1(0xFF)  # P()
            put4(0xB0 ^ P(idx))  # a = b + c
            break
        put1(0x00)


# a = sse could be used to go back to 0 or 1

solve(P_z3(P_z3(X)) == X)

import sys

sys.exit(0)

try:
    prog = open("prog", "wb")
    idx = 0
    T(lambda: set_a(0x30))
    exit()
finally:
    prog.close()
