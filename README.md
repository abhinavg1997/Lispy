Lispy

A simple Lisp-like interpreter in C, following the Build Your Own Lisp tutorial by Daniel Holden.

Lispy is an implementation of a minimal Lisp interpreter written in C. It follows the step-by-step chapters of the Build Your Own Lisp tutorial, from parsing to evaluation, extending with Q-expressions, variables, user-defined functions, and more.

Features

Each milestone corresponds to a tutorial chapter:

REPL & Parsing

Read, evaluate, print loop

Basic parsing of numbers and symbols

S-Expressions

Nested expressions ( + 1 ( * 2 3 ) )

Q-Expressions

Quote expressions as first-class lists {1 2 3}

Built-in Arithmetic & Comparison

+, -, *, /, %, ==, !=, <, >

Variables & Environments

def for global bindings

= for local bindings

User-Defined Functions

Lambda creation with \

Function application

Advanced Built-ins

List manipulation: head, tail, list, join

Evaluation: eval

Error Handling

Safe type and argument checking

Prerequisites

C compiler with support for C99 (e.g. GCC or Clang)

Make (optional, but recommended)

Unix-like environment (Linux, macOS, WSL)

Building

Clone this repo

git clone https://github.com/abhinavg1997/Lispy.git
cd Lispy

and then compile with gcc:
gcc -std=c99 -Wall -Werror -o lispy main.c mpc.c

and then run: ./lispy

