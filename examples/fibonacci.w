# Prints the first 50 fibonacci numbers

let! $a 0 $b 1;
for [range 50] [
	echoln $a;
	set! $a [+ $a $b];
	swap! $a $b
];
