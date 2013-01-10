lexer = require 'lex'

lex = lexer.new(16, 4096)

local tokens = {}

while true do

  local state, toks = lex.chunk()

  for i = 1, #toks do
    table.insert(tokens, toks[i])
  end

  if state == lexer.undecided then
    if not lex.full() then
      lex.refill()
    else
      break
    end

  elseif state == lexer.fin or state == lexer.fail then
    print(string.format('-- State: %d', state))
    break

  end

end

for i = 1, #tokens do
  print(string.format('%d: %q %d', tokens[i].type, tokens[i].string, tokens[i].len))
end
