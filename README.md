This repository contains a C++14 implementation of the
[Shunting-Yard Algorithm][syard], i.e. an algorithm for parsing
infix-notation expressions (e.g. `(1+2)*3*4*5`).

The expression parser is configurable via a function and an
operator table at runtime. Adding more operators and functions
during parsing is also supported as well as operators that are
overloaded as sign characters (e.g. `3 - -2`).

For examples how to interface with the parser see also
`test/syard.cc`.

2016-10-16, Georg Sauthoff <mail@georg.so>

## License

[LGPLv3+][lgpl]


[syard]: https://en.wikipedia.org/wiki/Shunting-yard_algorithm
[lgpl]: https://www.gnu.org/licenses/lgpl-3.0.en.html
