set term canvas mouse size 600,800
set output "bench.html"

set title 'Rijckit Benchmark Result'

set xlabel 'Token Length'
set ylabel 'Processor Cycles'

set xrange [0:15]

plot 'bench1.dat' title "Number" with errorbars, \
     'bench2.dat' title "Identifier" with errorbars, \
     'bench3.dat' title "Whitespace" with errorbars, \
     'bench4.dat' title "String" with errorbars, \
     'bench5.dat' title "Character" with errorbars, \
     'bench6.dat' title "Punctuation" with errorbars, \
     'bench7.dat' title "Directive" with errorbars, \
     'bench1.dat' title "" with lines, \
     'bench2.dat' title "" with lines, \
     'bench3.dat' title "" with lines, \
     'bench4.dat' title "" with lines, \
     'bench5.dat' title "" with lines, \
     'bench6.dat' title "" with lines, \
     'bench7.dat' title "" with lines