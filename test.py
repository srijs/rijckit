from cffi import FFI
from sys import stdin

from copy import copy

from matplotlib import pyplot as plt

ffi = FFI()

with open('lex.hi') as header:
    ffi.cdef(header.read())

TOKENS = []

Lex = ffi.dlopen('./liblex.so')

toks = ffi.new('Tok []', 1)
buf = ffi.new('char[]', 4096)
ctx = ffi.new('Ctx *', [buf, 4096, 4096, 0])

bufRW = ffi.buffer(buf, 4096)

ctx.sz = stdin.readinto(bufRW)

run = True
while run:
    st = Lex.lex(ctx, toks)

    if st == 'Success':
        string = bufRW[toks[0].off:toks[0].off + toks[0].len].encode('string-escape')
        TOKENS.append((copy(toks[0].type), string, toks[0].len, toks[0].t))

    elif st == 'Undecided':
        if ctx.off < ctx.cap:  # Refill
            ctx.sz += stdin.readinto(ffi.buffer(ctx.buf + ctx.off + ctx.sz, ctx.cap - ctx.sz))
            while ctx.sz < 4:
                ctx.buf[ctx.off + ctx.sz] = '\0'
                ctx.sz += 1
        else:
            run = False

    else:
        print '-- State: %s' % st
        run = False

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
