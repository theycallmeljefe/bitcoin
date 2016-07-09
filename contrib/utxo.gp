set terminal pngcairo enhanced size 1280,800 font 'Verdana,10' dashed
set output 'utxo_size.png'
set xdata time
set xlabel "Date"
set ylabel "MByte"
set xrange [1292344831 to 1468080438]
set timefmt "%s"
set nokey
set format x "%b '%y"
set grid
set rmargin 11
set title "UTXO breakdown per output amount: UTXO set bytes"
plot "utxo.dat" using 5:(($57)/1000000) with filledcurve y2 lc rgb "#ff0000" notitle, \
     "utxo.dat" using 5:(($58)/1000000) with filledcurve y2 lc rgb "#e91500" notitle, \
     "utxo.dat" using 5:(($59)/1000000) with filledcurve y2 lc rgb "#d42a00" notitle, \
     "utxo.dat" using 5:(($60)/1000000) with filledcurve y2 lc rgb "#bf3f00" notitle, \
     "utxo.dat" using 5:(($61)/1000000) with filledcurve y2 lc rgb "#aa5500" notitle, \
     "utxo.dat" using 5:(($62)/1000000) with filledcurve y2 lc rgb "#946a00" notitle, \
     "utxo.dat" using 5:(($63)/1000000) with filledcurve y2 lc rgb "#7f7f00" notitle, \
     "utxo.dat" using 5:(($64)/1000000) with filledcurve y2 lc rgb "#6a9400" notitle, \
     "utxo.dat" using 5:(($65)/1000000) with filledcurve y2 lc rgb "#55aa00" notitle, \
     "utxo.dat" using 5:(($66)/1000000) with filledcurve y2 lc rgb "#3fbf00" notitle, \
     "utxo.dat" using 5:(($67)/1000000) with filledcurve y2 lc rgb "#2ad400" notitle, \
     "utxo.dat" using 5:(($68)/1000000) with filledcurve y2 lc rgb "#15e900" notitle, \
     "utxo.dat" using 5:(($69)/1000000) with filledcurve y2 lc rgb "#00ff00" notitle, \
     "utxo.dat" using 5:(($70)/1000000) with filledcurve y2 lc rgb "#00e915" notitle, \
     "utxo.dat" using 5:(($71)/1000000) with filledcurve y2 lc rgb "#00d42a" notitle, \
     "utxo.dat" using 5:(($72)/1000000) with filledcurve y2 lc rgb "#00bf3f" notitle, \
     "utxo.dat" using 5:(($73)/1000000) with filledcurve y2 lc rgb "#00aa55" notitle, \
     "utxo.dat" using 5:(($74)/1000000) with filledcurve y2 lc rgb "#00946a" notitle, \
     "utxo.dat" using 5:(($75)/1000000) with filledcurve y2 lc rgb "#007f7f" notitle, \
     "utxo.dat" using 5:(($76)/1000000) with filledcurve y2 lc rgb "#006a94" notitle, \
     "utxo.dat" using 5:(($77)/1000000) with filledcurve y2 lc rgb "#0055aa" notitle, \
     "utxo.dat" using 5:(($78)/1000000) with filledcurve y2 lc rgb "#003fbf" notitle, \
     "utxo.dat" using 5:(($79)/1000000) with filledcurve y2 lc rgb "#002ad4" notitle, \
     "utxo.dat" using 5:(($80)/1000000) with filledcurve y2 lc rgb "#0015e9" notitle, \
     "utxo.dat" using 5:(($81)/1000000) with filledcurve y2 lc rgb "#0000ff" notitle, \
     "utxo.dat" using 5:(($57)/1000000) with line lw 1.5 lc rgb "#000000" title "  All" at end, \
     "utxo.dat" using 5:(($60)/1000000) with line lw 0.5 lc rgb "#000000" notitle, \
     "utxo.dat" using 5:(($63)/1000000) with line lw 1.5 lc rgb "#000000" title "  1 uBTC" at end, \
     "utxo.dat" using 5:(($66)/1000000) with line lw 0.5 lc rgb "#000000" title "  10 uBTC" at end, \
     "utxo.dat" using 5:(($69)/1000000) with line lw 0.5 lc rgb "#000000" title "  100 uBTC" at end, \
     "utxo.dat" using 5:(($72)/1000000) with line lw 1.5 lc rgb "#000000" title "  1 mBTC" at end, \
     "utxo.dat" using 5:(($75)/1000000) with line lw 0.5 lc rgb "#000000" title "  10 mBTC" at end, \
     "utxo.dat" using 5:(($78)/1000000) with line lw 0.5 lc rgb "#000000" title "  100 mBTC" at end, \
     "utxo.dat" using 5:(($81)/1000000) with line lw 1.5 lc rgb "#000000" title "  1 BTC" at end

set output 'utxo_outputs.png'
set xdata time
set xlabel "Date"
set ylabel "Million outputs"
set timefmt "%s"
set nokey
set format x "%b '%y"
set grid
set rmargin 11
set title "UTXO breakdown per output amount: outputs"
plot "utxo.dat" using 5:(($32)/1000000) with filledcurve y2 lc rgb "#ff0000" notitle, \
     "utxo.dat" using 5:(($33)/1000000) with filledcurve y2 lc rgb "#e91500" notitle, \
     "utxo.dat" using 5:(($34)/1000000) with filledcurve y2 lc rgb "#d42a00" notitle, \
     "utxo.dat" using 5:(($35)/1000000) with filledcurve y2 lc rgb "#bf3f00" notitle, \
     "utxo.dat" using 5:(($36)/1000000) with filledcurve y2 lc rgb "#aa5500" notitle, \
     "utxo.dat" using 5:(($37)/1000000) with filledcurve y2 lc rgb "#946a00" notitle, \
     "utxo.dat" using 5:(($38)/1000000) with filledcurve y2 lc rgb "#7f7f00" notitle, \
     "utxo.dat" using 5:(($39)/1000000) with filledcurve y2 lc rgb "#6a9400" notitle, \
     "utxo.dat" using 5:(($40)/1000000) with filledcurve y2 lc rgb "#55aa00" notitle, \
     "utxo.dat" using 5:(($41)/1000000) with filledcurve y2 lc rgb "#3fbf00" notitle, \
     "utxo.dat" using 5:(($42)/1000000) with filledcurve y2 lc rgb "#2ad400" notitle, \
     "utxo.dat" using 5:(($43)/1000000) with filledcurve y2 lc rgb "#15e900" notitle, \
     "utxo.dat" using 5:(($44)/1000000) with filledcurve y2 lc rgb "#00ff00" notitle, \
     "utxo.dat" using 5:(($45)/1000000) with filledcurve y2 lc rgb "#00e915" notitle, \
     "utxo.dat" using 5:(($46)/1000000) with filledcurve y2 lc rgb "#00d42a" notitle, \
     "utxo.dat" using 5:(($47)/1000000) with filledcurve y2 lc rgb "#00bf3f" notitle, \
     "utxo.dat" using 5:(($48)/1000000) with filledcurve y2 lc rgb "#00aa55" notitle, \
     "utxo.dat" using 5:(($49)/1000000) with filledcurve y2 lc rgb "#00946a" notitle, \
     "utxo.dat" using 5:(($50)/1000000) with filledcurve y2 lc rgb "#007f7f" notitle, \
     "utxo.dat" using 5:(($51)/1000000) with filledcurve y2 lc rgb "#006a94" notitle, \
     "utxo.dat" using 5:(($52)/1000000) with filledcurve y2 lc rgb "#0055aa" notitle, \
     "utxo.dat" using 5:(($53)/1000000) with filledcurve y2 lc rgb "#003fbf" notitle, \
     "utxo.dat" using 5:(($54)/1000000) with filledcurve y2 lc rgb "#002ad4" notitle, \
     "utxo.dat" using 5:(($55)/1000000) with filledcurve y2 lc rgb "#0015e9" notitle, \
     "utxo.dat" using 5:(($56)/1000000) with filledcurve y2 lc rgb "#0000ff" notitle, \
     "utxo.dat" using 5:(($32)/1000000) with line lw 1.5 lc rgb "#000000" title "  All" at end, \
     "utxo.dat" using 5:(($35)/1000000) with line lw 0.5 lc rgb "#000000" notitle, \
     "utxo.dat" using 5:(($38)/1000000) with line lw 1.5 lc rgb "#000000" title "  1 uBTC" at end, \
     "utxo.dat" using 5:(($41)/1000000) with line lw 0.5 lc rgb "#000000" title "  10 uBTC" at end, \
     "utxo.dat" using 5:(($44)/1000000) with line lw 0.5 lc rgb "#000000" title "  100 uBTC" at end, \
     "utxo.dat" using 5:(($47)/1000000) with line lw 1.5 lc rgb "#000000" title "  1 mBTC" at end, \
     "utxo.dat" using 5:(($50)/1000000) with line lw 0.5 lc rgb "#000000" title "  10 mBTC" at end, \
     "utxo.dat" using 5:(($53)/1000000) with line lw 0.5 lc rgb "#000000" title "  100 mBTC" at end, \
     "utxo.dat" using 5:(($56)/1000000) with line lw 1.5 lc rgb "#000000" title "  1 BTC" at end

set output 'utxo_amount.png'
set xdata time
set xlabel "Date"
set ylabel "Million BTC"
set timefmt "%s"
set nokey
set format x "%b '%y"
set grid
set rmargin 11
set title "UTXO breakdown per output amount: total value"
plot "utxo.dat" using 5:(($82)/100000000000000.0) with filledcurve y2 lc rgb "#ff0000" notitle, \
     "utxo.dat" using 5:(($83)/100000000000000.0) with filledcurve y2 lc rgb "#e91500" notitle, \
     "utxo.dat" using 5:(($84)/100000000000000.0) with filledcurve y2 lc rgb "#d42a00" notitle, \
     "utxo.dat" using 5:(($85)/100000000000000.0) with filledcurve y2 lc rgb "#bf3f00" notitle, \
     "utxo.dat" using 5:(($86)/100000000000000.0) with filledcurve y2 lc rgb "#aa5500" notitle, \
     "utxo.dat" using 5:(($87)/100000000000000.0) with filledcurve y2 lc rgb "#946a00" notitle, \
     "utxo.dat" using 5:(($88)/100000000000000.0) with filledcurve y2 lc rgb "#7f7f00" notitle, \
     "utxo.dat" using 5:(($89)/100000000000000.0) with filledcurve y2 lc rgb "#6a9400" notitle, \
     "utxo.dat" using 5:(($90)/100000000000000.0) with filledcurve y2 lc rgb "#55aa00" notitle, \
     "utxo.dat" using 5:(($91)/100000000000000.0) with filledcurve y2 lc rgb "#3fbf00" notitle, \
     "utxo.dat" using 5:(($92)/100000000000000.0) with filledcurve y2 lc rgb "#2ad400" notitle, \
     "utxo.dat" using 5:(($93)/100000000000000.0) with filledcurve y2 lc rgb "#15e900" notitle, \
     "utxo.dat" using 5:(($94)/100000000000000.0) with filledcurve y2 lc rgb "#00ff00" notitle, \
     "utxo.dat" using 5:(($95)/100000000000000.0) with filledcurve y2 lc rgb "#00e915" notitle, \
     "utxo.dat" using 5:(($96)/100000000000000.0) with filledcurve y2 lc rgb "#00d42a" notitle, \
     "utxo.dat" using 5:(($97)/100000000000000.0) with filledcurve y2 lc rgb "#00bf3f" notitle, \
     "utxo.dat" using 5:(($98)/100000000000000.0) with filledcurve y2 lc rgb "#00aa55" notitle, \
     "utxo.dat" using 5:(($99)/100000000000000.0) with filledcurve y2 lc rgb "#00946a" notitle, \
     "utxo.dat" using 5:(($100)/100000000000000.0) with filledcurve y2 lc rgb "#007f7f" notitle, \
     "utxo.dat" using 5:(($101)/100000000000000.0) with filledcurve y2 lc rgb "#006a94" notitle, \
     "utxo.dat" using 5:(($102)/100000000000000.0) with filledcurve y2 lc rgb "#0055aa" notitle, \
     "utxo.dat" using 5:(($103)/100000000000000.0) with filledcurve y2 lc rgb "#003fbf" notitle, \
     "utxo.dat" using 5:(($104)/100000000000000.0) with filledcurve y2 lc rgb "#002ad4" notitle, \
     "utxo.dat" using 5:(($105)/100000000000000.0) with filledcurve y2 lc rgb "#0015e9" notitle, \
     "utxo.dat" using 5:(($106)/100000000000000.0) with filledcurve y2 lc rgb "#0000ff" notitle, \
     "utxo.dat" using 5:(($82)/100000000000000.0) with line lw 1.5 lc rgb "#000000" title "  All" at end, \
     "utxo.dat" using 5:(($85)/100000000000000.0) with line lw 0.5 lc rgb "#000000" notitle, \
     "utxo.dat" using 5:(($88)/100000000000000.0) with line lw 1.5 lc rgb "#000000" title "  1 uBTC" at end, \
     "utxo.dat" using 5:(($91)/100000000000000.0) with line lw 0.5 lc rgb "#000000" title "  10 uBTC" at end, \
     "utxo.dat" using 5:(($94)/100000000000000.0) with line lw 0.5 lc rgb "#000000" title "  100 uBTC" at end, \
     "utxo.dat" using 5:(($97)/100000000000000.0) with line lw 1.5 lc rgb "#000000" title "  1 mBTC" at end, \
     "utxo.dat" using 5:(($100)/100000000000000.0) with line lw 0.5 lc rgb "#000000" title "  10 mBTC" at end, \
     "utxo.dat" using 5:(($103)/100000000000000.0) with line lw 0.5 lc rgb "#000000" title "  100 mBTC" at end, \
     "utxo.dat" using 5:(($106)/100000000000000.0) with line lw 1.5 lc rgb "#000000" title "  1 BTC" at end

