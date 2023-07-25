# calculates the first 20 factorials

let! $factorial [cmd $x [
	if
		[= $x 0] 1
		[* $x [factorial [- $x 1]]]
]];

for $i [range 20] [
	echoln [factorial $i]
]
