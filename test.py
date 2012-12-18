from cffi import FFI
from sys import stdin

ffi = FFI()

with open('lex.hi') as header:
  ffi.cdef(header.read())

STATE = True

TOKENS = []

@ffi.callback('Cont')
def printTok(context, token):

  global STATE, TOKENS

  def refill():
    context.sz += stdin.readinto(ffi.buffer(context.buf + context.sz, context.back_sz - context.sz))
    while context.sz < 4:
      context.buf[context.sz] = '\0'
      context.sz += 1

  if token.state == 'Success':
    TOKENS.append((token.type, ffi.buffer(context.buf, token.len)[:].encode("string-escape")))
    context.buf += token.len
    context.sz  -= token.len
    if context.sz < 4:
      refill()

  elif token.state == 'Undecided':
    if context.sz < context.back_sz:
      ffi.buffer(context.back_buf, context.sz)[:] = ffi.buffer(context.buf, context.sz)[:]
      context.buf = context.back_buf
      refill()
    else:
      STATE = False

  elif token.state == 'Fail':
    print 'Fail, Tok: %s, Char: %s' % (token.type, buf[0])
    STATE = False

  elif token.state == 'End':
    STATE = False

Lex = ffi.dlopen('./liblex.so')

buf = ffi.new('char[]', 4096)
ctx = ffi.new('Ctx *', [4096, 4096, buf, buf])

ctx.sz = stdin.readinto(ffi.buffer(buf, ctx.back_sz))

while STATE:
  Lex.lex(ctx, printTok)

for t in TOKENS:
  print '%s: %s' % t