# String Commands
These are command accesible by indexing a string. Note that commands not ending with `!` copy the string and modify and return that.
## `set!`
## `set`
Sets a given index in a string. Value can either be an integer (in which case it's converted to a byte) or a 1-length string.
### Examples
`abcd:set 2 k` => `abkd`

`"1111":set 0 48` => `0111`
## `slice!`
## `slice`
Slices a string given two indices
### Examples
```
let! $s abcdefg;
$s:slice! 0 4;
echoln $s; # abcde
```
## `split`
Splits a string by a given delimiter.
### Examples
`"this is a sentence":split " "` => `[list "this" "is" "a" "sentence"]`

```
let! $points-string 1,2|3,4|5,6;
let! $points [list];
for $point [$points-string:split |] [
    let! $s [$point:split ,];
    $points:push! [map
        x [int $s:0]
        y [int $s:1]
    ];
];
echoln $points; # [list [map y 2 x 1] [map y 4 x 3] [map y 6 x 5]]
```
