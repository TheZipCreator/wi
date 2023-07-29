# List Commands
These are command accesible by indexing a list. Note that commands not ending with `!` copy the list and modify and return that.
## `cat!`
## `cat`
Concats a list to the list given.
### Examples
```
set! $a [a b c];
set! $b [d e f];
$a:cat! $b;
echoln $a; # [list a b c d e f]
```
## `dup!`
## `dup`
Duplicates a list N times
### Examples
```
set! $l [list 1 2 3];
echoln
    [$l:dup 3] # [list 1 2 3 1 2 3 1 2 3]
    [$l:dup 2] # [list 1 2 3 1 2 3]
    [$l:dup 0] # [list]
    [$l:dup 1] # [list 1 2 3]
```
## `fill!`
## `fill`
Fills a list with a value
### Examples
```
set! $l [list a b c];
$l:fill! 3;
echoln $l; # [list 3 3 3]
```
## `pop!`
## `pop`
Pops a value from the end of a list. Returns the value popped.
### Examples
```
let! $l [list a b c];
echoln [$l:pop!]; # c
echoln $l; # [list a b]
```
## `push!`
## `push`
Pushes a value to the end of a list. Returns the list.
### Examples
```
let! $l [list a b c];
echoln [$l:push d]; # [list a b c d]
```
## `reverse!`
## `reverse`
Reverses a list.
### Examples
`[list 1 2 3]:reverse` => `[list 3 2 1]`

`[list]:reverse` => `[list]`

`[list abc def]:reverse` => `[list def abc]`
## `set!`
## `set`
Sets a value in a list. Returns the list.
### Examples
```
let! $l [list 1 2 3];
$l:set! 2 20;
echoln $l; # [list 1 2 20]
```
## `shift!`
## `shift`
Pops a value from the start of a list. Returns the value shifted.
### Examples
```
let! $l [list a b c];
echoln [$l:shift!]; # a
echoln $l; # [list b c]
```
## `slice!`
## `slice`
Slices a list by two indices. Returns the slice.
### Examples
```
set! $l [list a b c d e f g h];
echoln [$l:slice! 2 5]; # [list c d e]
```

```
set! $l [list a b c d e f g h];
$l:slice! 2 5;
echoln $l; # [list c d e]
```
## `unshift!`
## `unshift`
Pushes a value to the start of a list. Returns the list
### Examples
```
let! $l [list b c d];
$l:unshift! a;
echoln $l; # [list a b c d]
```
