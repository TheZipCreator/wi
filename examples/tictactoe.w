# A simple tic-tac-toe game

let! $board [[new-list 9]:fill! 0] $player 1;
echoln "Positions: \n0|1|2\n-----\n3|4|5\n-----\n6|7|8";

# converts number to cell name

let! $cell-name [cmd $cell [if
	[= $cell 0] " "
	[= $cell 1] X
	[= $cell 2] O
]];

# returns 0 if nobody won, or 1/2 if either player won
let! $won [cmd [
	# check for horizontal/vertical wins
	for $i [range 3] [
		let! $3i [* $i 3];
		if
			[& [= $board:$3i $board:[+ $3i 1]] [= $board:[+ $3i 1] $board:[+ $3i 2]]] [return $board:$3i]
			[& [= $board:$i $board:[+ $i 3]] [= $board:[+ $i 3] $board:[+ $i 6]]] [return $board:$i];
	];
	# diagonals
	let! $center $board:4;
	if
		[& [= $center $board:0] [= $center $board:8]] [return $center]
		[& [= $center $board:2] [= $center $board:6]] [return $center];
	return 0
]];

# prints the board
let! $print-board [cmd [
	echoln "Board:";
	for $i $cell $board [
		echo [cell-name $cell];
		if 
			[= [% $i 3] 2] [
				echoln [if
					[!= $i 8] "\n-----"
					""
				];
			]
			[echo "|"];
	]
]];

while 1 [
	# print board
	print-board;
	while 1 [
		# get position
		let! $pos [int [readln [cell-name $player] "? "]];
		# if invalid, try again
		if 
			[| [< $pos 0] [> $pos 8]] [
				echoln "Invalid position.";
				continue;
			]
			[!= $board:$pos 0] [
				echoln "There's already something in that position.";
				continue;
			];
		# set board
		$board:set! $pos $player;
		break;
	];
	# see if anyone won
	let! $who-won [won];
	if [!= $who-won 0] [
		print-board;
		echoln [cell-name $who-won] " won!";
		break;
	];
	# swap player	
	set! $player [if
		[= $player 1] 2
		[= $player 2] 1
	];
];
