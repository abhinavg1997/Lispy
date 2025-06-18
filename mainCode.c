#include "mpc.h" // We can also use quotes "" instead of <> as quotes will look in the curr directory
#include <stdarg.h>
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
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };
// 2. Error Types
//  a. Division By Zero
//  b. Bad Operand
//  c. Bad Number (Too large probably)
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// LISP VALUE STRUCTURE

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

typedef lval *(*lbuiltin)(lenv *, lval *);

/*
 * ################################
 * #### LISP VALUE STRUCTURE ######
 * ################################
 * */

typedef struct lval {
  long num;
  int type;

  char *err;
  char *sym;
  lbuiltin fun;

  int count;
  struct lval **cell;
} lval;

/*
 * ################################
 * #### ENV STRUCTURE ######
 * ################################
 * */

struct lenv {
  int count;
  // list of lval* and chars*
  char **syms;
  lval **vals;
};

// adding definition of eval
void lval_println(lval *v);
lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *s);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_fun(lbuiltin func);
lval *lval_add(lval *v, lval *x);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
lval *lval_read(mpc_ast_t *t);
void lval_del(lval *v);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *builtin(lenv *e, lval *a, char *func);
lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_def(lenv *e, lval *a);
lval *lval_join(lval *x, lval *y);
char *ltype_name(int i);
lval *lval_copy(lval *v);
lenv *lenv_new(void);
void lenv_del(lenv *e);
void lenv_put(lenv *e, lval *k, lval *v);
lval *lenv_get(lenv *e, lval *v);
void lenv_add_builtin(lenv *e, char *name, lbuiltin func);
void lenv_add_builtins(lenv *e);
lval *builtin_add(lenv *e, lval *a);
lval *builtin_sub(lenv *e, lval *a);
lval *builtin_mul(lenv *e, lval *a);
lval *builtin_div(lenv *e, lval *a);

#define LASSERT(args, cond, fmt, ...)                                          \
  {                                                                            \
    if (!(cond)) {                                                             \
      lval *err = lval_err(fmt, ##__VA_ARGS__);                                \
      lval_del(args);                                                          \
      return err;                                                              \
    }                                                                          \
  }
#define LASSERT_TYPE(func, args, index, expect)                                \
  LASSERT(args, args->cell[index]->type == expect,                             \
          "Function '%s' passed incorrect type for argument %i. Got %s, "      \
          "Expected %s.",                                                      \
          func, index, ltype_name(args->cell[index]->type),                    \
          ltype_name(expect));

#define LASSERT_COUNT(func, args, num)                                         \
  LASSERT(args, args->count == num,                                            \
          "Function '%s' passed incorrect number of arguments. Got %i, "       \
          "Expected %i.",                                                      \
          func, args->count, num);

#define LASSERT_NOT_EMPTY(func, args, index)                                   \
  LASSERT(args, args->cell[index]->count != 0,                                 \
          "Function '%s' passed {} for argument %i.", func, index);

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Qexpr = mpc_new("qexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispy = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
    number   : /-?[0-9]+/ ;                             \
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;         \
    sexpr     :  '(' <expr>* ')' ;  \
    qexpr     :  '{' <expr>* '}' ;  \
    expr     : <number> |  <symbol> | <sexpr> | <qexpr>  ;  \
    lispy    : /^/  <expr>* /$/ ;             \
  ",
            Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");
  /* Initialize an environment*/
  lenv *e = lenv_new();
  lenv_add_builtins(e);

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
      lval *x = lval_eval(e, lval_read(r.output));
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
  mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
}

/*
 * ################################
 * #### LISP TYPES ################
 * ################################
 * */

lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->num = x;
  v->type = LVAL_NUM;
  return v;
}

lval *lval_err(char *fmt, ...) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  /* Allocate 512 bytes of space */
  v->err = malloc(512);

  /* printf the error string with a max of 511 characters */
  vsnprintf(v->err, 511, fmt, va);

  /* Reallocate to number of bytes actually used */
  v->err = realloc(v->err, strlen(v->err) + 1);

  va_end(va);
  return v;
  ;
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

lval *lval_qexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval *lval_fun(lbuiltin func) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = func;
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

  if (strstr(t->tag, "qexpr")) {
    x = lval_qexpr();
  }

  /* Fill the list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "{") == 0) {
      continue;
    }
    if (strcmp(t->children[i]->contents, "}") == 0) {
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
  /* Do nothing special for number type of functions */
  case LVAL_NUM:
  case LVAL_FUN:
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
  case LVAL_QEXPR:
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

/*
 * ################################
 * #### EVALUATION FUNCTION ##########
 * ################################
 * */

lval *lval_eval_sexpr(lenv *e, lval *v) {
  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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
  if (f->type != LVAL_FUN) {
    lval_del(f);
    lval_del(v); // Throw error
    return lval_err("first element is not a function");
  }

  /* Call Function to get result */
  lval *res = f->fun(e, v);
  lval_del(f);
  return res;
}

lval *lval_eval(lenv *e, lval *v) {
  /*Evaluate Sexpressions */
  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(e, v);
  }
  return v;
}

/*
 * ################################
 * #### BUILTIN FUNCTION ##########
 * ################################
 * */

/* add all predefined builtin */
void lenv_add_builtins(lenv *e) {
  /* list functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "def", builtin_def);

  /* math functions" */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
}

/* add a custom builtin */
void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);

  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

lval *builtin(lenv *e, lval *a, char *func) {
  if (strcmp("list", func) == 0) {
    return builtin_list(e, a);
  }
  if (strcmp("head", func) == 0) {
    return builtin_head(e, a);
  }
  if (strcmp("tail", func) == 0) {
    return builtin_tail(e, a);
  }
  if (strcmp("join", func) == 0) {
    return builtin_join(e, a);
  }
  if (strcmp("eval", func) == 0) {
    return builtin_eval(e, a);
  }
  if (strstr("+-/*", func)) {
    return builtin_op(e, a, func);
  }
  lval_del(a);
  return lval_err("Unknown Function!");
}

lval *builtin_def(lenv *e, lval *a) {
  LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

  lval *syms = a->cell[0];

  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM,
            "Function 'def' cannot define non-symbol. "
            "Got %s, Expected %s.",
            ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }

  LASSERT(a, syms->count == a->count - 1,
          "Function 'def' passed too many arguments for symbols. "
          "Got %i, Expected %i.",
          syms->count, a->count - 1);

  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i + 1]);
  }
  lval_del(a);
  return lval_sexpr();
}

lval *builtin_op(lenv *e, lval *a, char *op) {
  /* Ensure all alrguments are numbers*/
  for (int i = 0; i < a->count; i++) {
    // if (a->cell[i]->type != LVAL_NUM) {
    //   lval_del(a);
    //   return lval_err("Cannot operate on non-number");
    // }
    LASSERT_TYPE("op", a, i, LVAL_NUM);
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

lval *builtin_add(lenv *e, lval *a) { return builtin_op(e, a, "+"); }

lval *builtin_sub(lenv *e, lval *a) { return builtin_op(e, a, "-"); }

lval *builtin_mul(lenv *e, lval *a) { return builtin_op(e, a, "*"); }

lval *builtin_div(lenv *e, lval *a) { return builtin_op(e, a, "/"); }

lval *builtin_head(lenv *e, lval *a) {
  LASSERT_COUNT("head", a, 1);
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("head", a, 0);

  lval *x = lval_take(a, 0);

  while (x->count > 1) {
    lval_del(lval_pop(x, 1));
  }
  return x;
}

lval *builtin_tail(lenv *e, lval *a) {
  LASSERT_COUNT("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("tail", a, 0);

  lval *v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval *builtin_list(lenv *e, lval *a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval *builtin_eval(lenv *e, lval *a) {
  LASSERT_COUNT("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

  lval *x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval *builtin_join(lenv *e, lval *a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("join", a, i, LVAL_QEXPR);
  }
  lval *x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }
  lval_del(a);
  return x;
}

lval *lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval *lval_pop(lval *v, int i) {
  lval *x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(
      &v->cell[i], &v->cell[i + 1],
      sizeof(lval *) *
          (v->count - i -
           1)); //        void *memmove(void *dest, const void *src, size_t n);

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

lval *lval_copy(lval *v) {

  lval *x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
  /* Direct copy for function and number */
  case LVAL_FUN:
    x->fun = v->fun;
    break;
  case LVAL_NUM:
    x->num = v->num;
    break;

    /* Copy strings using mallox and strcpy */

  case LVAL_ERR:
    x->err = malloc(strlen(v->err) + 1);
    strcpy(x->err, v->err);
    break;

  case LVAL_SYM:
    x->sym = malloc(strlen(v->sym) + 1);
    strcpy(x->sym, v->sym);
    break;

    /* Copy list by copying each sub-expression */

  case LVAL_QEXPR:
  case LVAL_SEXPR:
    x->count = v->count;
    x->cell = malloc(sizeof(lval *) * v->count);
    for (int i = 0; i < v->count; i++) {
      x->cell[i] = lval_copy(v->cell[i]);
    }
    break;
  }
  return x;
}

/* ######################
 * ## ENV FUCNTIONS #####
 * ######################
 */

lenv *lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));

  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv *v) {

  for (int i = 0; i < v->count; i++) {
    free(v->syms[i]);
    lval_del(v->vals[i]);
  }
  free(v->vals);
  free(v->syms);
  free(v);
}

// lenv get function
lval *lenv_get(lenv *e, lval *v) {
  // iterate over e to look for v
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], v->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  //  If no symbol found, return error
  return lval_err("unbound symbol '%s'", v->sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {

  /* Iterate and check if all items in enviromnent exists*/
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  /* If no exisiting entry is found, then allocate space for new entry */
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval *) * e->count);
  e->syms = realloc(e->syms, sizeof(char *) * e->count);

  /* copy the new value to vals and syms */
  e->vals[e->count - 1] = lval_copy(v);
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);
}

char *ltype_name(int i) {
  switch (i) {
  case LVAL_FUN:
    return "Function";
  case LVAL_NUM:
    return "Number";
  case LVAL_ERR:
    return "Error";
  case LVAL_SYM:
    return "Symbol";
  case LVAL_SEXPR:
    return "S-expression";
  case LVAL_QEXPR:
    return "Q-expression";
  default:
    return "Unknown";
  }
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
  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;
  case LVAL_FUN:
    printf("<Function>");
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
