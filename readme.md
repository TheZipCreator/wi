# wi - Tungstyn Interpreter
This is the repository for `wi`, the interpreter for the Tungstyn language. The language design and the interpreter are both currently very much a work-in-progress, and as suchit is not very fast, and you will find bugs. Please report these as github issues. If you have any suggestions, I would also appreciate if you made them github issues too.
Documentation on the language (in markdown format) is available under the docs/ directory.
## A note on naming
The chemical symbol for tungsten (the element that is the language's namesake) is `W`. This is reflected in the file extension used for tungstyn (`.w`) and the name of this interpreter (`wi`, Tungstyn `w` Interpreter `i`).
## Building
In order to build `wi`, install all dependencies and run the `build` script. You can also specify any arguments you wish to pass to `gcc` in that script (ex. `./build -O3`).
### Dependencies
Currently, Tungstyn's only dependency is `libreadline`.
