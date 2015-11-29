# Produced with:
# (OLDS=0; NEWS=0; WITS=0; while read HASH NTX OLD NEW WIT; do OLDS=$(($OLDS+$OLD)); NEWS=$(($NEWS+$NEW)); WITS=$(($WITS+$WIT)); echo "$HASH $NTX $OLDS $NEWS $WITS"; done) <block_sizes.txt >cumulative_block_sizes.txt

set xlabel "Time"
set ylabel "Gigabytes"
set xdata time
set format x "%b '%y"
set key left top
set title "Bitcoin blockchain size with and without segregated witness"
set timefmt "%d/%m/%y"
set xrange ["01/01/11" to "01/01/16"]
set xtics ("01/01/11", "01/01/12", "01/01/13", "01/01/14", "01/01/15", "01/01/16")
set timefmt "%s"
#set logscale y
set grid x y
set style fill noborder
set terminal pngcairo linewidth 1.7 fontscale 1.3 enhanced size 1024,768
set output "size-witness.png"
plot "cumulative_block_sizes.txt" using (1239119566 + ($2) * 543.37):((($4)+($5))/1000000000) with filledcurves x1 fs transparent solid 0.8 lc rgb "#77FF77" title "SW: witness data", \
     "cumulative_block_sizes.txt" using (1239119566 + ($2) * 543.37):(($4)/1000000000) with filledcurves x1 lc rgb "#008000" title "SW: base data", \
     "cumulative_block_sizes.txt" using (1239119566 + ($2) * 543.37):(($3)/1000000000) with line lc rgb "black" linewidth 1.5 title "Actual blockchain"

set terminal pngcairo linewidth 3 fontscale 2.5 enhanced size 2560,1920
set output "size-witness-larger.png"
replot

set terminal pngcairo linewidth 1 fontscale 1 enhanced size 640,480
set output "size-witness-small.png"
replot

set terminal pngcairo linewidth 1.7 fontscale 1.3 enhanced size 1024,768
set output "size-witness-0.5.png"
plot "cumulative_block_sizes.txt" using (1239119566 + ($2) * 543.37):((($4)+($5)*0.5)/1000000000) with filledcurves x1 fs transparent solid 0.8 lc rgb "#77FF77" title "SW: witness data (counted 50%)", \
     "cumulative_block_sizes.txt" using (1239119566 + ($2) * 543.37):(($4)/1000000000) with filledcurves x1 lc rgb "#008000" title "SW: base data", \
     "cumulative_block_sizes.txt" using (1239119566 + ($2) * 543.37):(($3)/1000000000) with line lc rgb "black" linewidth 1.5 title "Actual blockchain"

set terminal pngcairo linewidth 3 fontscale 2.5 enhanced size 2560,1920
set output "size-witness-0.5-larger.png"
replot

set terminal pngcairo linewidth 1 fontscale 1 enhanced size 640,480
set output "size-witness-0.5-small.png"
replot
