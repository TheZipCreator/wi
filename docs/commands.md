# Commands
This documents all top-level commands
## `!=`
Returns the opposite of `!=`
## `%`
Modulo operator.
### Examples
`% 10 3` => `1`
## `&`
Short-circuit logical and operator
### Examples
`& 1 1 1` => `1`

`& 1 0` => `0`

`& 1 null` => `null`
## `*`
Multiplication operator.
### Examples
`* 10 3` => `30`

`* 5 4 2` => `40`
## `+`
Addition operator.
### Examples
`+ 2 4` => `6`

`+ 5 5 5` => `15`
## `-`
Subtraction operator.
### Examples
`- 4 2` => `2`

`- 5 2 1` => `2`

`- 2 10` => `-8`
## `/`
Division operator. Does integer division if both arguments are integers, float division otherwise.
### Examples
`/ 10 5` => `2`

`/ 2 3` => `0`

`/ 2 3.0` => `0.66666`
## `<`
Returns `1` if the left number is less than the right number.
## `<=`
Returns `1` if the left number is less than or equal to the right number.
## `=`
Returns `1` if the left value is equal to the right value. Lists are compared by each of their elements, while all other values are compared directly by their values.
### Examples
`= 1 2` => `0`

`= 1 1` => `1`

`= [list 1 2 3] [list 1 2 3]` => `1`

`= [list 1] [list 2]` => `0`

`= [map a 2] [map a 2]` => `0`
## `>`
Returns `1` if the left number is greater than the right number.
## `>=`
Returns `1` if the left number is greater than or equal to the right number.
## `|`
Short-circuit logical or operator
### Examples
`| 1 1 1` => `1`

`| 1 0` => `1`

`| 0 0 0` => `0`

`| 1 null` => `1`

`| null abcd` => `abcd`

`| a b` => `a`
## `break`
Exits a loop prematurely
### Examples
```
for $i [range 50] [
	echo $i
	if [= $i 10] [break]
]
```
will echo
```
0123456789
```
## `cmd`
Creates a command
### Examples
```
let! $double [cmd $x [* $x 2]];
echoln [double 12]; # 24
```

```
let! $2x+y [cmd $x $y [+ [* 2 $x] $y]];
echoln [2x+y 10 5] # 25
```

```
let! $factorial [cmd $x [
	if
		[= $x 0] 1
		[* $x [factorial [- $x 1]]]
]];
echoln [factorial 5] # 120
```
## `continue`
Skips to the next iteration of a loop.
### Examples
```
for $i [range 10] [
    if [= $i 5] [continue];
    echoln $i
]
```
prints

`012346789`

## `del!`
Deletes a variable
### Examples
```
let! $x 3;
echoln $x; # 3
del! $x;
# uncommenting out this line will cause an error, since $x has been deleted.
# echoln $x;
```

## `do`
Evaluates its operand.
### Examples
`do 3` => `3`

`do [+ 2 3]` => `5`

`do [let! $i 5; * $i 40]` => `200`
## `echo`
Echoes all of its arguments to stdout, and returns `null`.
### Examples
```
# echoes 3
let! $x 3;
echo $x;
```

```
echo "Hello, World!"
```
## `echoln`
Same as `echo`, but outputs a newline after.
## `float`
Converts its argument to a float. Returns `null` on failure.
## `for`
Iterates over a collection, optionally taking a value and an index. Returns the last value returned by the block executed, or `null` if the block was not executed.
### Examples
```
# echoes "aaaaa"
for [range 5] [
    echo a
]
```

```
# echoes "abcd"
let! $l [list a b c d];
for $x $l [
    echo $x;
]
```

```
# echoes entries of the map
let! $m [map
    a 1
    b 2
    c 3
];
for $key $value $m [
    echoln $key " = " $value
];
```

```
# Doubles each entry in a list
set! $l [list 1 2 3];
for $i $num $l [
    $l:set! $i [* $num 2]
];
```
## `if`
Evaluates every other argument, and if it returns truthy, returns the argument after. Optionally takes a trailing argument which it returns if none of the other conditions were truthy.
### Examples
```
if [= $x 2] [
	echoln "x is 2!";
];
```

```
if
	[< $x 2] [echoln "x is less than 2"]
	[= $x 2] [echoln "x is 2"]
	[< $x 10] [echoln "x is between 3 and 9"]
	# else condition
	[echoln "x is greater than 9"];
```

```
echoln [if
	[< $x 2] "x is less than 2"
	[= $x 2] "x is 2"
	[< $x 10] "x is between 3 and 9"
	"x is greater than 9"
];
```
## `int`
Converts its argument to an integer, returning `null` on failure.
## `let!`
Declares a variable
### Examples
```
let! $x 3;
echoln $x; # 3
```

```
let! $x 2;
# uncommenting out this line will cause an error, since $x is already declared.
# let! $x 5; 
```
## `list`
Creates a list from its arguments
### Examples
```
let! $l [list a b c]; # creates a list with 3 elements, "a", "b", and "c"
```
## `map`
Creates a map from its arguments
### Examples
```
let! $m [map
    # elements are listed sequentially
    a 2
    b 3
    c 8
]
```
## `new-list`
Creates a list with a given number of entries, each set to `null`.
### Examples
```
set! $l [[new-list 20]:fill 0]; # create a list of length 20 and fill it with zeroes.
```
## `range`
Creates a range between a start and an end. The start is optional, which if left out will infer 0.
### Examples
```
# print every number between 0 and 4
for $i [range 5] [
    echoln $i;
]
```

```
# fill a list with 0 1 0 1 0 1 etc.
set! $l [new-list 20];
for $i [range $l:len] [
    $l:set! $i [% $i 2];
];
```
## `read`
Reads all of stdin.
### Examples
```
# Takes a number from stdin, doubles it, and echoes it
echo [* [int [read]] 2]
```
## `readln`
Reads a line from stdin, optionally taking a prompt.
### Examples
```
let! $name [readln "What is your name? "];
echoln "Hello, " $name;
```
## `refcount`
Gets the reference count of a value. Returns `-1` if the value is not a reference counted type (such as `int` or `float`).
### Examples
```
set! $a [list a b c];
set! $b [list d $a f];
echoln [refcount $a]; # 2
```
## `return`
Returns a value from a function prematurely.
### Examples
```
let! $c [cmd [
    for $i [range 10] [
        if [= $i 4] [return $i];
    ]
]];
echoln [c]; # 4
```
## `set!`
Sets an already declared variable
### Examples
```
# increments a variable
let! $x 5;

set! $x [+ $x 1];
```

```
# uncommenting the next line causes an error, since $x is not declared.
# set! $x 5
```
## `string`
Converts a value to a string. Can not fail.
## `swap!`
Swaps the values of two variables
### Examples
```
let! $a 2;
let! $b 5;
echoln $a , $b; # 2,5
swap! $a $b;
echoln $a , $b; # 5,2
```
## `while`
Runs a block while a condition remains true
### Examples
```
let! $i 0;
while [< $i 10] [
    echoln $i;
    set! $i [+ $i 1];
];
```
