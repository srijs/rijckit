local ffi = require 'ffi'

ffi.cdef('int read(int, void *, size_t)')
ffi.cdef(io.open('lex.hi', 'r'):read('*all'))

local lex = ffi.load('./liblex.so')

local toks = ffi.new('Tok [?]', 16)
local buf  = ffi.new('char [?]', 4096)
local ctx  = ffi.new('Ctx', buf, 4096, 4096, 0)

local tokens = {}

ctx.sz = ffi.C.read(0, buf, 4096)

while true do

  local num = lex.lex(ctx, toks, 16)
  
  for i = 0, num-1 do
    table.insert(tokens, {type   = toks[i].type,
                          string = ffi.string(buf + toks[i].off, toks[i].len),
                          len    = tonumber(toks[i].len)})
  end

  if ctx.state == lex.Undecided then
    if ctx.off < ctx.cap then
      ffi.copy(buf, buf + ctx.off, ctx.sz)
      ctx.off = 0
      ctx.sz = ctx.sz + ffi.C.read(0, buf + ctx.sz, ctx.cap - ctx.sz)
      if ctx.sz < 4 then
        ffi.fill(buf + ctx.sz, 4 - ctx.sz)
        ctx.sz = 4
      end
    else
      break
    end

  elseif ctx.state == lex.End or ctx.state == lex.Fail then
    print(string.format('-- State: %d', ctx.state))
    break

  end

end

for i = 1, #tokens do
  print(string.format('%d: %q %d', tokens[i].type, tokens[i].string, tokens[i].len))
end
