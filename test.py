from cffi import FFI
from sys import stdin
from copy import copy
from matplotlib import pyplot as plt

BENCH = False

ffi = FFI()

with open('lex.hi') as header:
    ffi.cdef(header.read())

TOKENS = []

Lex = ffi.dlopen('./liblex.so')

toks = ffi.new('Tok []', 16)
buf = ffi.new('char[]', 4096)
ctx = ffi.new('Ctx *', [buf, 4096, 4096, 0])

bufRW = ffi.buffer(buf, 4096)

ctx.sz = stdin.readinto(bufRW)

while True:
    num = Lex.lex(ctx, toks, 16)

    for i in range(0, num):
        string = bufRW[toks[i].off:toks[i].off + toks[i].len].encode('string-escape')
        if BENCH:
            TOKENS.append((copy(toks[i].type), string, toks[i].len, toks[i].t))
        else:
            TOKENS.append((copy(toks[i].type), string, toks[i].len))

    if ctx.state == 'Success':
        pass

    elif ctx.state == 'Undecided':
        if ctx.off < ctx.cap:  # Refill
            bufRW[:ctx.sz] = bufRW[ctx.off:ctx.off + ctx.sz]
            ctx.off = 0
            ctx.sz += stdin.readinto(ffi.buffer(ctx.buf + ctx.off + ctx.sz, ctx.cap - ctx.sz))
            while ctx.sz < 4:
                ctx.buf[ctx.off + ctx.sz] = '\0'
                ctx.sz += 1
        else:
            break

    else:
        print '-- State: %s' % ctx.state
        break

if BENCH:
    metrics = {'length': [], 'cycles': [], 'throughput': [], 'type': []}

    for t in TOKENS:
        print '%s: %s %d %d' % t
        metrics['length'].append(t[2])
        metrics['cycles'].append(t[3])
        metrics['throughput'].append(t[3] / t[2])
        metrics['type'].append(t[0])

    print 'Scattering...'
    plt.scatter(metrics['length'], metrics['cycles'], c='b')
    plt.scatter(metrics['length'], metrics['throughput'], c='r')
    plt.show()

else:
    for t in TOKENS:
        print '%s: %s %d' % t
