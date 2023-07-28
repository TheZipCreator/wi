# Map Commands
These are command accesible by indexing a map. Note that commands not ending with `!` copy the map and modify and return that.
## set!
## set
Sets a value in a map.
### Examples
```
set! $m [map
    x 0
    y 2
];
$m:set! y 10;
echoln $m; # [map x 0 y 10]
```
