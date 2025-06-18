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
   gcc -std=c99 -Wall mainCode.c mpc.c -ledit -o Lispy
   ```
4. Run the interpreter:
   ```
   ./lispy
   ```

## Usage

Lispy is a REPL (Read-Eval-Print Loop) interpreter. You can enter Lisp-like expressions, and the interpreter will evaluate them and print the result.

### 1. Basic Evaluation

You can perform arithmetic operations directly:

```
lispy> (+ 1 2 3)
6
lispy> (* 2 3 4)
24
lispy> (- 10 4)
6
lispy> (/ 20 2 2)
5
```

---

### 2. Variable Declaration

You can declare variables using the `def` keyword:

```
lispy> (def {x} 5)
()
lispy> (def {y z} 10 20)
()
```

---

### 3. Using Variables in Expressions

Once declared, variables can be used in further expressions:

```
lispy> (+ x 2)
7
lispy> (* y z)
200
lispy> (- z x)
15
```

---

### 4. Q-Expressions and List Operations

Q-Expressions allow you to treat code as data (like lists):

```
lispy> {1 2 3 4}
{1 2 3 4}
lispy> (head {1 2 3 4})
{1}
lispy> (tail {1 2 3 4})
{2 3 4}
lispy> (join {1 2} {3 4})
{1 2 3 4}
lispy> (eval {+ 1 2 3})
6
```

---

### 5. Error Handling

Lispy provides helpful error messages for invalid operations:

```
lispy> (head 1)
Error: Function 'head' passed incorrect type for argument 0. Got Number, Expected Q-Expression.
lispy> (/ 1 0)
Error: Division by Zero
lispy> (+ 1 {2 3})
Error: Function '+' passed incorrect type for argument 1. Got Q-Expression, Expected Number.
```

---

### 6. Defining Multiple Variables

You can define multiple variables at once:

```
lispy> (def {a b c} 10 20 30)
()
lispy> (+ a b c)
60
```

---

### 7. Nested Expressions

You can nest expressions for more complex calculations:

```
lispy> (+ (* 2 3) (/ 10 2))
11
```

---

## FEATURES

Lispy supports the following features:

- Basic parsing of numbers and symbols
- S-Expressions (nested expressions)
- Q-Expressions (quoted expressions as first-class lists)
- Built-in arithmetic and comparison operations (+, -, *, /)
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
