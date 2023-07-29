## v0.1.1
- Added a `w_readline` so `libreadline` is no longer a hard requirement. It is still recommended, however. (turn on the define `-DHAS_READLINE` in order to activate it. This is done by default on Windows).
- Fixed compilation errors that for some reason I didn't get on my computer.
## v0.2.0
- Fixed `(null):1:1` file position appearing in certain circumstances.
- Fixed the `let!` command calling itself `set!` sometimes.
- Added `string:dup`, `string:reverse`, `string:cat`, `list:dup`, and `list:reverse`
- Allowed `read` to take an argument, which is a file to read.
- Added `write
