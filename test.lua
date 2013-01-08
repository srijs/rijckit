local ffi = require 'ffi'

io.input('lex.hi')
ffi.cdef(io.read('*all'))

local lex = ffi.load('./liblex.so')

local toks = ffi.new('Tok [?]', 16)
local buf  = ffi.new('char [?]', 4096)
local ctx  = ffi.new('Ctx', buf, 4096, 4096, 0)

local tokens = {}

io.input(io.stdin)
local str = io.read(4096)
ctx.sz = #str
ffi.copy(buf, str, ctx.sz)

while true do

  num = lex.lex(ctx, toks, 16)
  
  for i = 0, num-1 do
    table.insert(tokens, {type   = toks[i].type,
                          string = ffi.string(buf + toks[i].off, toks[i].len),
                          len    = tonumber(toks[i].len)})
  end

  if ctx.state == lex.Success then

  elseif ctx.state == lex.Undecided then
    if ctx.off < ctx.cap then
      ffi.copy(buf, buf + ctx.off, ctx.sz)
      ctx.off = 0
      str = io.read(tonumber(ctx.cap - ctx.sz))
      if str then
        ffi.copy(buf + ctx.sz, str, ctx.cap - ctx.sz)
        ctx.sz = ctx.sz + #str
        while ctx.sz < 4 do
          ctx.buf[ctx.sz] = 0
          ctx.sz = ctx.sz + 1
        end
      else
        ffi.fill(buf + ctx.sz, 4)
        ctx.sz = 4
      end
    else
      break
    end

  else
    print(string.format('-- State: %d', ctx.state))
    break

  end

end

for i = 1, #tokens do
  print(string.format('%d: %q %d', tokens[i].type, tokens[i].string, tokens[i].len))
end
