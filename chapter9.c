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

// enum definitions
// 1. Lisp value types:
// a. Number
// b. Error
// c. Symbols
// d. S expression
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
// 2. Error Types
//  a. Division By Zero
//  b. Bad Operand
//  c. Bad Number (Too large probably)
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// LISP VALUE STRUCTURE
typedef struct lval {
  long num;
  int type;

  char *err;
  char *sym;

  int count;
  struct lval **cell;
} lval;
// adding definition of eval
void lval_println(lval *v);
lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_sexpr(void);
lval *lval_add(lval *v, lval *x);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
lval *lval_read(mpc_ast_t *t);
void lval_del(lval *v);
lval *lval_eval_sexpr(lval *v);
lval *lval_eval(lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *builtin_op(lval *a, char *op);

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
    expr     : <number> |  <symbol> | <sexpr>  ;  \
    lispy    : /^/  <expr>* /$/ ;             \
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
      lval *x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
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

lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->num = x;
  v->type = LVAL_NUM;
  return v;
}

lval *lval_err(char *m) {
  lval *v = malloc(sizeof(lval));
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
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

  /* Fill the list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}

lval *lval_add(lval *v, lval *x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

void lval_del(lval *v) {

  switch (v->type) {
  /* Do nothing special for number type */
  case LVAL_NUM:
    break;

  /* For Err or Sym free the string data */
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;

  /* If Sexpr then delete all elements inside */
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    /* Also free the memory allocated to contain the pointers */
    free(v->cell);
    break;
  }

  /* Free the memory allocated for the "lval" struct itself */
  free(v);
}

/* EVALUATION FUNCTION*/

lval *lval_eval_sexpr(lval *v) {
  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }
  /* Error checking in the childreb */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  /* Empty Expression */
  if (v->count == 0) {
    return v;
  }

  /* Single Expression */
  if (v->count == 1) {
    return lval_take(v, 0);
  }

  /* If all of above is not matched, then it has to be starting with a symbol */

  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v); // Throw error
    return lval_err("S-Expression does not start with symbol!");
  }

  /* Call Built in operator */
  lval *res = builtin_op(v, f->sym);
  lval_del(f);
  return res;
}

lval *lval_eval(lval *v) {
  /*Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }
  return v;
}

lval *builtin_op(lval *a, char *op) {
  /* Ensure all alrguments are numbers*/
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number");
    }
  }

  /*Pop first element */
  lval *x = lval_pop(a, 0);
  /* If no other elements in cell and operator is negative*/
  if (a->count == 0 && strcmp(op, "-") == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    lval *y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) {
      x->num += y->num;
    }
    if (strcmp(op, "-") == 0) {
      x->num -= y->num;
    }
    if (strcmp(op, "*") == 0) {
      x->num *= y->num;
    }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by Zero");
        break;
      }
      x->num /= y->num;
    }
    lval_del(y);
  }
  lval_del(a);
  return x;
}

lval *lval_pop(lval *v, int i) {
  lval *x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));

  /* Decrease the count of the items in the list */
  v->count--;

  /* Reallocate the memory used*/
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  return x;
}

lval *lval_take(lval *v, int i) {
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

void lval_expr_print(lval *v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    // Don't print the trailing element
    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%li", v->num);
    break;
  case LVAL_ERR:
    printf("Error: %s", v->err);
    break;
  case LVAL_SYM:
    printf("%s", v->sym);
    break;
  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
  }
}

void lval_println(lval *v) {
  lval_print(v);
  putchar('\n');
}
/*

Bonus Marks

Learnings:

*/
