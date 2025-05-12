#include "mpc.h" // We can also use quotes "" instead of <> as quotes will look in the curr directory
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char *readline(char *prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char *cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}
void add_history(char *unused) {}

#else
#include <editline/readline.h>
#endif

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispy = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
  ",
            Number, Operator, Expr, Lispy);
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  /* In a never ending loop */
  while (1) {

    /* Output our prompt and get input */
    char *input = readline("lispy> ");

    /* Add input to history */
    add_history(input);

    /* Echo input back to user */
    /* Attempt to Parse the user Input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On Success Print the AST */
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise Print the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    /* Free retrieved input */
    free(input);
  }
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
}

/*

Bonus Marks

    › Write a regular expression matching strings of all a or b such as aababa
or bbaa. Ans = ^[ab]+$
    ^[a-b]+$
    /
    gm
    ^ asserts position at start of a line
    Match a single character present in the list below [a-b]
    + matches the previous token between one and unlimited times, as many times
as possible, giving back as needed (greedy) a-b matches a single character in
the range between a (index 97) and b (index 98) (case sensitive) $ asserts
position at the end of a line


    › Write a regular expression matching strings of consecutive a and b such as
ababab or aba. Ans =

    › Write a regular expression matching pit, pot and respite but not peat,
spit, or part. › Change the grammar to add a new operator such as %. › Change
the grammar to recognise operators written in textual format add, sub, mul, div.
    › Change the grammar to recognize decimal numbers such as 0.01, 5.21,
or 10.2. › Change the grammar to make the operators written conventionally,
between two expressions. › Use the grammar from the previous chapter to parse
Doge. You must add start and end of input.


*/
