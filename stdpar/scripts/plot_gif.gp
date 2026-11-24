set terminal gif animate delay 5 optimize size 800,800
set output 'jacobi.gif'

files = system("ls -1 Mesh_Iters_Parallel/*.dat | sort")
n = words(files)

set size square
unset colorbox
set palette rgb 33, 13, 10
# row 0 of the matrix at the top; a plain "set yrange [0:101]" would undo the reversal
set xrange [-0.5:101.5]
set yrange [101.5:-0.5]
# set margins 0,0,0,0

set title "A gif made of 42 images!" font " , 23"
set xlabel "x" font " , 18"
set ylabel "y" font " , 18"

do for [i=1:n] {
	fname = word(files, i)
	plot fname matrix with image
}

reset
set output
set terminal qt #just to return back to normal output terminal; MIGHT NOT BE QT ON YOUR SYSTEM 
