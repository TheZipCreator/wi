# Introduction
Tungstyn is an interpreted scripting language, primarily inspired by LISP, ECMAScript, and Lua. You can invoke the REPL by running `wi` with no arguments. You can also run a file by invoking it with the file path.
## Expressions
Tungstyn contains two primary expression types: Command Blocks and Index Expressions.
### Command Blocks
A command is of the following form:
~~~
<string | expr> <expr ...>
~~~
Here's an example:
~~~
+ 1 2
~~~
This is the `+` command, which adds two numeric values. If you open the REPL, and type this in, it'll spit back `3` at you.
A command *block* is a sequence of commands seperated by semicolons (`;`). When you write `+ 1 2`, what you've really created is a command block with a single command in it.
The return value of a command block is the return value of the last command executed; e.g:
`+ 1 2; * 2 8`
returns `16`.
In order to nest command blocks, you wrap them with brackets (`[]`), like so:
~~~
+ 1 [* 3 4] 8 [- 1 2]
~~~
This is equivalent to the infix expression `1+3*4+8+(1-2)`, and returns 20. If you've ever programmed in LISP before, this should be familiar.
While we're here, I'll mention the `let!` command, which declares a variable (variables are prefixed with `$`; this actually has a reason which we'll get to later.)
~~~
let! $x 5
~~~
returns `5` and binds `$x`. We can then use `$x` in a later command within the current block.
## Index Expressions
The `list` command creates a `list` from its arguments, like so:
~~~
list 1 2 3
~~~
This returns a list containing 1, 2, and 3. Let's bind this to a variable, `$l`.
~~~
let! $l [list 1 2 3]
~~~
Now, let's say we wanted to get the second number from this list. Lists are 0-indexed, so that's index `1`. In order to do this, we use `:`, which is the only infix operator in the language.
~~~
echo $l:1
~~~
`echo` is a command which echoes its arguments. The `:` here takes the left argument, and *indexes* it by the right argument (`1`). Note that we can even call commands this way:
~~~
$l:set! 0 20
~~~
This will set the 0th index in the list to the value `20`.
## Types
As of now, Tungstyn has 8 types: null, int, float, string, list, map, externcommand, and command.
### Null
The null type has no data; sometimes returned from functions when an error occurs, or stored in a structure when a field is uninitialized. It can be created via the keyword `null`.
### Int
An int is a 64-bit signed integer. They can be created via integer literals (e.g. `1`, `2`, `1000`, `203`, etc.)
### Float
A float a 64-bit floating point number. They can be can be created via float literals (e.g. `1.0`, `23.5`, `3.14159`, `5.`, etc.)
### String
A string is a sequence of characters. They can be any length, and can contain the null character (U+0000). Any sequence of characters, excluding control characters (`;`, ` `, `[`, `]`, `"`, `$`, and `:`) creates a string (e.g. `abcd` is a string literal for `"abcd"`). They can also be quoted (e.g. `"Hello, World!"`) in which case they can contain control characters. Inside a quoted string, escape codes may also be used (e.g. `"Hello\nWorld"`).
### List
A list is a sequence of values. They can be created via the `list` command, which has been discussed previously.
### Map
A map consists of key-value pairs, with each key only being mapped to a single value, and where the keys are strings. They can be created via the `map` command, which takes a sequence of key-value pairs and creates a map from them.
e.g:
~~~
let! $m [map
	a 0
	abcd 2
	x [+ 2 3]
	y 9
];
~~~
### Externcommand
An externcommand is a command implemented in C. Most builtin commands are of this type. They can not be created from within Tungstyn.
### Command
A command is a command implemented in Tungstyn. They can be created via the `cmd` command, which takes a sequence of arguments then an expression.
e.g:
~~~
let! $double [cmd $x [* $x 2]];
echoln [double 20]; # echoes 40
~~~
## Variables
As mentioned before, variables are prefixed with `$`, and this is to distinguish them from string literals. A variable can be declared with `let!`, and reassigned with `set!`.
For example:
~~~
let! $x 10;
# $x is now declared
echoln $x; # 10
let! $y [+ $x 2];
echoln $y; # 12
# $x has already been declared, so we reassign it with set!
set! $x [+ $x 1];
echoln $x; # 11
~~~
## Control constructs
So, that's all the value types. As for control constructs, Tungstyn has all the ones you'd expect (`for`, `while`, `if`). Here's how those work:
### Do
`do` does the block that it is given.
~~~
do [+ 1 2]
~~~
returns `3`. 
### If
Before we discuss `if`, here are some conditional commands:
`=`: Detects if two values are equal.
`!=`: Detects if two values are not equal.
`>` `<` `>=` `<=`: Comparisons on numeric types.
These conditions return `1` if true, and `0` if false.
Tungstyn's `if` is more similar to a `cond` block rather than a traditional `if`. It can test as many conditions as is necessary, and can also contain an `else` condition. Here's an `if` with just one condition:
~~~
if [= $x 2] [
	echoln "x is 2!";
];
~~~
This will echo `x is 2!` if `$x` is equal to `2`, as you'd expect. However, in an `if`, you can check multiple different conditions:
~~~
if
	[< $x 2] [echoln "x is less than 2"]
	[= $x 2] [echoln "x is 2"]
	[< $x 10] [echoln "x is between 3 and 9"]
	# else condition
	[echoln "x is greater than 9"];
~~~
`if` also returns the value where the condition returns true, so this could be condensed to:
~~~
echoln [if
	[< $x 2] "x is less than 2"
	[= $x 2] "x is 2"
	[< $x 10] "x is between 3 and 9"
	"x is greater than 9"
];
~~~
which does the same thing, but with less repitition.
### While
`while` continually executes a block while a condition is active.
~~~
let! $x 0;
while [< $x 10] [
	set! $x [+ $x 1];
	echoln $x;
]
~~~
will echo the following:
~~~
1
2
3
4
5
6
7
8
9
10
~~~
### For
`for` iterates over a collection (such as a `map` or a `list`). You can optionally keep track of the index of the value.
~~~
for $x [list 1 2 3] [
	echoln $x
]
~~~
echoes
~~~
1
2
3
~~~
If you want to iterate from a start value to an end value, use the `range` command to construct a list containing all values you wish to iterate over.
~~~
# prints all numbers between 0 1 2 3 ... 9
for $x [range 0 10] [
	echoln $x
];
# you can also leave out the start in range if it's 0
# does the same thing
for $x [range 10] [
	echoln $x;
];
set! $l [list 1 2 3];
# keep track of index each time
for $i $x $l [
	$l:set! $i [+ $x 1];
];
# you can iterate over maps, too.
let $m [map
	a 2
	b 3
	some-key 20
];
# a = 2
# b = 3
# etc.
# (possibly not in that order)
for $key $value $m [
	echoln $key = $value;
];
~~~
As for control constructs, that's about it.
## Conventions
What follows aren't hard language rules; rather just some conventions about naming. You don't have to follow these rules; you should write code whichever way makes the most sense to you. But these are the rules used in the standard library, so they're worth remembering.
### Naming
The names of commands and variables in the standard library use `lisp-case`. You may also have noticed the presence of an exclamation mark (`!`) at the end of the names of some commands. This is appended to any command that modifies *state*, whether that be the state of a passed-in value (such as `list:set!` or `string:slice!`) or the state of the interpreting context (like `let!` or `set!`). In the other documentation files, you'll notice that the mutable (the one with `!`) and immutable (the one without `!`) versions of the same command will be listed next to eachother. Typically, the immutable version will create a *copy* of the data structure, and then modify that, while the mutable version will directly modify the existing structure.
### File extension
For reasons explained in the readme, the file extension for Tungstyn code should be `.w`.
## Conclusion
That's the end of the introduction. Hopefully you should be able to write some Tungstyn code now. If you want to, you can view the other files within this folder, which contain more information on the currently existing commands. You could also check under the `examples/` directory, which shows some examples of Tungstyn code.
