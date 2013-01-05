from cffi import FFI
from sys import stdin

from copy import copy

from matplotlib import pyplot as plt

ffi = FFI()

with open('lex.hi') as header:
  ffi.cdef(header.read())

TOKENS = []

Lex = ffi.dlopen('./liblex.so')

tok = ffi.new('Tok *')
buf = ffi.new('char[]', 4096)
ctx = ffi.new('Ctx *', [4096, 4096, buf, buf])

bufRD = ffi.buffer(buf)

ctx.sz = stdin.readinto(ffi.buffer(buf, ctx.back_sz))

run = True
while run:
  ret = Lex.lex(ctx, tok)

  def refill():
    ctx.sz += stdin.readinto(ffi.buffer(ctx.buf + ctx.sz, ctx.back_sz - ctx.sz))
    while ctx.sz < 4:
      ctx.buf[ctx.sz] = '\0'
      ctx.sz += 1

  if ret.state == 'Success':
    string = bufRD[tok.ptr - ctx.back_buf :
                   tok.ptr - ctx.back_buf + tok.len].encode('string-escape')
    TOKENS.append((copy(tok.type), string, tok.len, ret.t))

  elif ret.state == 'Undecided':
    if ctx.sz < ctx.back_sz:
      refill()
    else:
      run = False

  elif ret.state == 'Fail':
    print 'Fail, Tok: %s, Char: %s' % (tok.type, ctx.buf[0])
    run = False

  elif ret.state == 'End':
    run = False

x_length     = []
y_cycles     = []
y_throughput = []
z_type       = []

for t in TOKENS:
  print '%s: %s %d %d' % t
  x_length.append(t[2])
  y_cycles.append(t[3])
  y_throughput.append(t[3] / t[2])
  z_type.append(t[0])

print 'Scattering...'
plt.scatter(x_length, y_cycles, c = 'b')
plt.scatter(x_length, y_throughput, c = 'r')
plt.show()
