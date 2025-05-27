# Lispy

A simple Lisp-like interpreter in C, following the Build Your Own Lisp tutorial by Daniel Holden.

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/abhinavg1997/Lispy.git
   ```
2. Change to the project directory:
   ```
   cd Lispy
   ```
3. Compile the project using GCC:
   ```
   gcc -std=c99 -Wall -Werror -o lispy main.c mpc.c
   ```
4. Run the interpreter:
   ```
   ./lispy
   ```

## Usage

Lispy is a REPL (Read-Eval-Print Loop) interpreter. You can enter Lisp-like expressions, and the interpreter will evaluate them and print the result.

Here's an example of using Lispy:

```
lispy> (+ 1 2 3)
6
lispy> (- 10 4)
6
lispy> (def x 5)
x
lispy> (+ x 1)
6
lispy> (def double (\ (x) (* x 2)))
double
lispy> (double 5)
10
```

## API

Lispy supports the following features:

- Basic parsing of numbers and symbols
- S-Expressions (nested expressions)
- Q-Expressions (quoted expressions as first-class lists)
- Built-in arithmetic and comparison operations (+, -, *, /, %, ==, !=, <, >)
- Variables and environments (global and local bindings)
- User-defined functions with lambda expressions
- Advanced built-ins (head, tail, list, join, eval)
- Error handling with safe type and argument checking

## Contributing

Contributions to Lispy are welcome! If you find any issues or have ideas for improvements, please feel free to open an issue or submit a pull request.

## License

Lispy is licensed under the [MIT License](LICENSE).

## Testing

Lispy does not currently have a formal test suite. However, you can test the interpreter by running various expressions and verifying the output.