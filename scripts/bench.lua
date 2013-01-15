lexer = require 'lex'

lex = lexer.new(16, 4096)

local metrics = {
  [lexer.number]      = {},
  [lexer.identifier]  = {},
  [lexer.whitespace]  = {},
  [lexer.string]      = {},
  [lexer.character]   = {},
  [lexer.punctuation] = {},
  [lexer.directive]   = {}
}

while true do

  local state, toks = lex.chunk(true)

  for i = 1, #toks do
    if not metrics[toks[i].type][toks[i].len] then
      metrics[toks[i].type][toks[i].len] = {}
    end
    table.insert(metrics[toks[i].type][toks[i].len], toks[i].cycles)
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

function mean_and_stddev (values)
  local sum,n = 0,#values
  for i = 1,n do
    sum = sum + values[i]
  end
  local mean = sum/n
  sum = 0
  for i = 1,n do
    sum = sum + (mean - values[i])^2
  end
  return mean, math.sqrt(sum/n)
end

for typ in pairs(metrics) do
  local file = io.open(string.format('bench%d.dat', typ), 'w+')
  for len, points in ipairs(metrics[typ]) do
    local mean, stddev = mean_and_stddev(points)
    file:write(string.format('%f %f %f\n', len, mean, stddev))
  end
  file:close()
end