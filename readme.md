### Wordle CE

A clone of the word guessing game [Wordle](https://www.powerlanguage.co.uk/wordle/)
for the TI-84 Plus CE and TI-83 Premium CE graphing calculators.

![Animated screenshot of gameplay](screenshot.png)

### Usage
*(A video tutorial for the following steps is also available
[here](https://www.youtube.com/watch?v=_e8pgw9d7S4))*

Transfer [WORDLE.8xp](https://github.com/commandblockguy/wordle/releases/latest/download/WORDLE.8xp),
[WORDS.8xv](https://github.com/commandblockguy/wordle/releases/latest/download/WORDS.8xv)
and the [C Libraries](https://tiny.cc/clibs) to the calculator using TI Connect CE
or TiLP. Then, select prgmWORDLE from the program menu and press enter.
If this results in an error, [arTIfiCE](https://yvantt.github.io/arTIfiCE/) is
required for your OS version. If you have not set your system clock, be sure to
do that from the calculator's mode menu so that you can get the latest puzzle.

Help is available in-game by pressing the y= key while the game is running.

### Compiling
To build [this repository](https://github.com/commandblockguy/wordle) from source,
the [CE C Toolchain](https://github.com/CE-Programming/toolchain) is required.
After installing the toolchain, clone this repository and run `make gfx`
inside it to generate the graphics data, and then `make` to compile. The output .8xp
can be found in the `bin/` directory.

### Credits
Based on [Wordle](https://www.powerlanguage.co.uk/wordle/) by Josh Wardle.