#include "mpc.h" // We can also use quotes "" instead of <> as quotes will look in the curr directory
#include <cstring>
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

// enum definitions
// 1. Lisp value types:
// a. Number
// b. Error
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
// 2. Error Types
//  a. Division By Zero
//  b. Bad Operand
//  c. Bad Number (Too large probably)
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// LISP VALUE STRUCTURE
typedef struct {
  long num;
  int type;

  char *err;
  char *sym;

  int count;
  struct lval **cell;
} lval;
// adding definition of eval
lval eval(mpc_ast_t *t);
void lval_println(lval v);
lval eval_op(lval x, char *op, lval y);
lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_sexpr(void);

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispy = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
    number   : /-?[0-9]+/ ;                             \
    symbol : '+' | '-' | '*' | '/' ;                  \
    sexpr     :  '(' <expr>* ')' ;  \
    expr     : <number> | '(' <symbol> <sexpr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
  ",
            Number, Symbol, Sexpr, Expr, Lispy);
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
      //      mpc_ast_print(r.output);
      //     mpc_ast_delete(r.output);

      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise Print the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    /* Free retrieved input */
    free(input);
  }
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
}

lval eval(mpc_ast_t *t) {
  // If tagged as number, then return it directly
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  // operator is ALWAYS the second child
  char *op = t->children[1]->contents;

  lval x = eval(t->children[2]);

  int i = 3; // Start from index 3 since we already evaluated children[2]
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  return x;
}

lval eval_op(lval x, char *op, lval y) {
  if (x.type == LVAL_ERR) {
    return x;
  }
  if (y.type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    return *lval_num(x.num + y.num);
  }
  if (strcmp(op, "-") == 0) {
    return *lval_num(x.num - y.num);
  }
  if (strcmp(op, "*") == 0) {
    return *lval_num(x.num * y.num);
  }
  if (strcmp(op, "/") == 0) {

    return y.num == 0 ? *lval_err(LERR_DIV_ZERO) : *lval_num(x.num / y.num);
  }
  return *lval_err(LERR_BAD_OP);
}

lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->num = x;
  v->type = LVAL_NUM;
  return v;
}

lval *lval_err(char *m) {
  lval *v = malloc(sizeof(lval));
  v->err = malloc(strlen(m) + 1);
  v->type = LVAL_ERR;
  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval *v) {

  switch (v->type) {
  case LVAL_NUM:
    break; // Not doing anything for num

  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;

  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  }
  free(v);
}

lval *lval_read_num(mpc_ast_t *t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid num");
}

lval *lval_read(mpc_ast_t *t) {

  /* If number or symbol, return conversion to that type */
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }
  /* If root (>) or sexpr then create empty list */
  lval *x = NULL;
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM:
    printf("%li", v.num);
    break;
  case LVAL_ERR:

    if (v.err == LERR_DIV_ZERO) {
      printf("Error: Division By Zero!");
    }
    if (v.err == LERR_BAD_OP) {
      printf("Error: Invalid Operator!");
    }
    if (v.err == LERR_BAD_NUM) {
      printf("Error: Invalid Number!");
    }
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}
/*

Bonus Marks

Learnings:

*/
