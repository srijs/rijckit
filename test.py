from cffi import FFI
from sys import stdin

ffi = FFI()

with open('lex.hi') as header:
  ffi.cdef(header.read())

STATE = True

@ffi.callback('Cont')
def printTok(context, token):

  global STATE

  def refill():
    context.sz += stdin.readinto(ffi.buffer(context.buf + context.sz, context.back_sz - context.sz))
    while context.sz < 4:
      context.buf[context.sz] = '\0'
      context.sz += 1

  if token.state == 'Success':
    print '%s %s: %s' % (token.type, token.len, ffi.string(context.buf, token.len))
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
    print 'End'
    STATE = False

Lex = ffi.dlopen('./liblex.so')

buf = ffi.new('char[]', 4096)
ctx = ffi.new('Ctx *', [4096, 4096, buf, buf])

ctx.sz = stdin.readinto(ffi.buffer(buf, ctx.back_sz))

while STATE:
  Lex.lex(ctx, printTok)
