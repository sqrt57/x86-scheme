# x86-scheme

## Overview
This is a simple implementation of Scheme programming language in x86 32-bit assembly.

## Goals
- Learn how to implement Scheme interpreter close to the metal.
- Bootstrap a more ambitious Scheme interpreter/compiler "enscheme"
- Easy porting to other OS (Ported to GNU/Linux and Windows, next target is KolibriOS)

## Non-goals
- 16-bit or 64-bit version
- Other processor architectures
- REPL
- Interpretation speed
- Small memory consumption
- Completness or standards conformance
- Good error reporting
- Debugging facilities

## Features
- Full call-with-current-continuation
- Stop-and-copy precise garbage collector
- Supports rebinding all symbols including builtins
- Common Lisp like macros

## Building instructions
x86-scheme is compiled with [flat assembler](http://flatassembler.net/). It can be compiled under Windows or GNU/Linux. All build scripts build two executables of x86-scheme: one for GNU/Linux and one for Windows.

### Linux
Download flat assembler for linux and put it in the fasm-linux subdirectory of the project. Run build.sh to build executables or test.sh to build and run interpreter on test.scm. This can be done with the following shell commands:
```
git clone git@github.com:sqrt57/x86-scheme.git x86-scheme
curl http://flatassembler.net/fasm-1.71.54.tgz -o fasm-1.71.54.tgz
cd x86-scheme
tar -xzf ../fasm-1.71.54.tgz
mv fasm fasm-linux
./test.sh
```

### Windows
Download flat assembler for windows and put it in the fasm-win subdirectory of the project. Run build.bat to build executables or test.bat to build and run interpreter on test.scm.

## Command-line arguments
x86-scheme accepts the following arguments
- `-e expr` Executes expression.
- `filename` Executes file.
- `--` Treats all remaining command line arguments as file names.

For example:
```
x86-scheme file1.scm -e "(write (quote hello))" -- file2.scm
```
executes file1.scm, then expression `(write (quote hello))` and then file2.scm.

## Implementation rationale
I have always heard that Scheme interpreter is easy to implement. So I started this project to really find out. Writing in assembly is an experiment to find out how little is required from the underlying platfrom to support and how easy it is to implement Scheme close to the metal.

I suck at structuring even medium-sized assembly program, so I had to make simplifications to finish something working and useful without being overwhelmed. Some of the simplifications:
- Reader:
  - Only nonnegative numbers. It simplifies reader and has a workaround for entering negative numbers: `(- 5)`.
  - Only `(quote x)` syntax for quoation, not the standard `'x`.
  - `#t` and `#f` are self-evaluating symbols, not separate boolean objects.
  - `(a b . c)` syntax for improper list is limited. It understands only atomic (symbols, strings, numbers) object after dot. `(a b . (c d))` is not supported.
- No REPL. It can be implemented in Scheme language if needed.
- Common Lisp like macros because they were easiest to implement.

## Acknowledgements
- Flat assembler by Tomasz Grysztar is my favourite assembler. It is actively developed and supports all modern x86 and x86-64 instruction set extensions. And its macro  system is awesome.
  - [http://flatassembler.net/](http://flatassembler.net/)
- "Structure and Interpretation of Computer Programs" book
  - [https://mitpress.mit.edu/sicp/](https://mitpress.mit.edu/sicp/)
- Revised 5 Report on the Algorithmic Language Scheme
  - [http://www.schemers.org/Documents/Standards/R5RS/](http://www.schemers.org/Documents/Standards/R5RS/)
- Revised 7 Report on the Algorithmic Language Scheme
  - [http:/www.r7rs.org/](http://www.r7rs.org/)
