bfi - brainfuck interpreter
===========================
bfi is a simple, but flexible brainfuck interpreter.

1) Installation
---------------
Compile:

`$ make`

Edit the Makefile to match your local setup. It's per default installed to
/usr/local. Depending on setup you may have to be root:

`$ make install`

Remove binary and object files:

`$ make clean`


2) Running
----------
Interpreting a file is just

`$ bfi file`

For further information and options see the man page or invoke bfi with
usage information

`$ bfi --help`


3) Bugs
-------
Please forward any bugs you find to me at `mathias.justesen@gmail.com` or make
a pull request.


4) License and copyright information
------------------------------------
This software is licensed under the MIT License, see LICENSE for more
information.
