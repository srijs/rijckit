local ffi = require 'ffi'

ffi.cdef('int read(int, void *, size_t)')
ffi.cdef(io.open('lex.h', 'r'):read('*all'))
local lib = ffi.load('../liblex.so')

local lexer = {
  success   = lib.Success,   fail = lib.Fail,
  undecided = lib.Undecided, fin  = lib.End,
  undefined   = lib.Undefined,   number     = lib.Number,
  identifier  = lib.Identifier,  whitespace = lib.Whitespace,
  string      = lib.String,      character  = lib.Character,
  punctuation = lib.Punctuation, directive  = lib.Directive
}

function lexer.new(tsz, bsz)

  local tok = ffi.new('Tok [?]',  tsz)
  local buf = ffi.new('char [?]', bsz)
  local ctx = ffi.new('Ctx',      buf, 0, bsz, 0)

  local obj = {}

  function obj.chunk()
    local tokens = {}
    local num = lib.lex(ctx, tok, tsz)
    for i = 0, num-1 do
      table.insert(tokens, {type   = tonumber(tok[i].type),
                            string = ffi.string(buf + tok[i].off, tok[i].len),
                            len    = tonumber(tok[i].len)})
    end
    return tonumber(ctx.state), tokens
  end

  function obj.full()
    return not (ctx.off < ctx.cap)
  end

  function obj.refill()
    ffi.copy(buf, buf + ctx.off, ctx.sz)
    ctx.off = 0
    ctx.sz = ctx.sz + ffi.C.read(0, buf + ctx.sz, ctx.cap - ctx.sz)
    if ctx.sz < 4 then
      ffi.fill(buf + ctx.sz, 4 - ctx.sz)
      ctx.sz = 4
    end
  end

  return obj

end

return lexer
