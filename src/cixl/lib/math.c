#include <inttypes.h>

#include "cixl/arg.h"
#include "cixl/box.h"
#include "cixl/cx.h"
#include "cixl/error.h"
#include "cixl/fimp.h"
#include "cixl/func.h"
#include "cixl/lib.h"
#include "cixl/rat.h"
#include "cixl/scope.h"
#include "cixl/lib/math.h"

static bool inc_imp(struct cx_scope *scope) {
  struct cx_box *v = cx_test(cx_peek(scope, false));
  v->as_int++;
  return true;
}

static bool dec_imp(struct cx_scope *scope) {
  struct cx_box *v = cx_test(cx_peek(scope, false));
  v->as_int--;
  return true;
}

static bool int_add_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  x->as_int += y.as_int;
  return true;
}

static bool int_sub_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  x->as_int -= y.as_int;
  return true;
}

static bool int_mul_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  x->as_int *= y.as_int;
  return true;
}

static bool int_div_imp(struct cx_scope *scope) {
  struct cx *cx = scope->cx;
  
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    x = *cx_test(cx_pop(scope, false));

  if (!y.as_int) {
    cx_error(cx, cx->row, cx->col, "Division by zero");
    return false;
  }
  
  cx_rat_init(&cx_box_init(cx_push(scope), cx->rat_type)->as_rat,
	      cx_abs(x.as_int), cx_abs(y.as_int),
	      (x.as_int >= 0 || y.as_int > 0) && (x.as_int < 0 || y.as_int < 0));
  
  return true;
}

static bool int_mod_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  x->as_int = x->as_int % y.as_int;
  return true;
}

static bool int_abs_imp(struct cx_scope *scope) {
  struct cx_box *n = cx_test(cx_peek(scope, false));
  if (n->as_int < 0) { n->as_int = -n->as_int; }
  return true;
}

static bool rand_imp(struct cx_scope *scope) {
  struct cx_box max = *cx_test(cx_pop(scope, false));
  cx_box_init(cx_push(scope), scope->cx->int_type)->as_int = cx_rand(max.as_int);
  return true;
}

static bool rat_add_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  cx_rat_add(&x->as_rat, &y.as_rat);
  return true;
}

static bool rat_mul_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  cx_rat_mul(&x->as_rat, &y.as_rat);
  return true;
}

static bool rat_scale_imp(struct cx_scope *scope) {
  struct cx_box
    y = *cx_test(cx_pop(scope, false)),
    *x = cx_test(cx_peek(scope, false));

  x->as_rat.num *= y.as_int;
  return true;
}

static bool rat_int_imp(struct cx_scope *scope) {
  struct cx_box v = *cx_test(cx_pop(scope, false));
  cx_box_init(cx_push(scope), scope->cx->int_type)->as_int = cx_rat_int(&v.as_rat);
  return true;
}

cx_lib(cx_init_math, "cx/math") {
  struct cx *cx = lib->cx;

  if (!cx_use(cx, "cx/abc", "A", "Fimp", "Func", "Int", "Num", "Opt", "Seq") ||
      !cx_use(cx, "cx/cond", "=", "?", "if-else") ||
      !cx_use(cx, "cx/func", "recall") ||
      !cx_use(cx, "cx/iter", "for")) {
    return false;
  }

  cx->rat_type = cx_init_rat_type(lib);

  cx_add_cfunc(lib, "++",
	       cx_args(cx_arg("v", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       inc_imp);
  
  cx_add_cfunc(lib, "--",
	       cx_args(cx_arg("v", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       dec_imp);

  cx_add_cfunc(lib, "+",
	       cx_args(cx_arg("x", cx->int_type), cx_arg("y", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       int_add_imp);
  
  cx_add_cfunc(lib, "-",
	       cx_args(cx_arg("x", cx->int_type), cx_arg("y", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       int_sub_imp);
  
  cx_add_cfunc(lib, "*",
	       cx_args(cx_arg("x", cx->int_type), cx_arg("y", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       int_mul_imp);
  
  cx_add_cfunc(lib, "/",
	       cx_args(cx_arg("x", cx->int_type), cx_arg("y", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->rat_type)),
	       int_div_imp);

  cx_add_cfunc(lib, "mod",
	       cx_args(cx_arg("x", cx->int_type), cx_arg("y", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->rat_type)),
	       int_mod_imp);

  cx_add_cfunc(lib, "abs",
	       cx_args(cx_arg("n", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       int_abs_imp);

  cx_add_cfunc(lib, "rand",
	       cx_args(cx_arg("n", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       rand_imp);

  cx_add_cfunc(lib, "+",
	       cx_args(cx_arg("x", cx->rat_type), cx_arg("y", cx->rat_type)),
	       cx_args(cx_arg(NULL, cx->rat_type)),
	       rat_add_imp);
  
  cx_add_cfunc(lib, "*",
	       cx_args(cx_arg("x", cx->rat_type), cx_arg("y", cx->rat_type)),
	       cx_args(cx_arg(NULL, cx->rat_type)),
	       rat_mul_imp);

  cx_add_cfunc(lib, "*",
	       cx_args(cx_arg("x", cx->rat_type), cx_arg("y", cx->int_type)),
	       cx_args(cx_arg(NULL, cx->rat_type)),
	       rat_scale_imp);

  cx_add_cfunc(lib, "int",
	       cx_args(cx_arg("r", cx->rat_type)),
	       cx_args(cx_arg(NULL, cx->int_type)),
	       rat_int_imp);
  
  cx_add_cxfunc(lib, "fib-rec",
		cx_args(cx_arg("a", cx->int_type),
			cx_arg("b", cx->int_type),
			cx_arg("n", cx->int_type)),
		cx_args(cx_arg(NULL, cx->int_type)),
		"$n?<Opt> {$b $a $b +<Int Int> $n -- recall} $a if-else");

  cx_add_cxfunc(lib, "fib",
		cx_args(cx_arg("n", cx->int_type)),
		cx_args(cx_arg(NULL, cx->int_type)),
		"0 1 $n fib-rec");
    
  cx_add_cxfunc(lib, "sum",
		cx_args(cx_arg("in", cx->seq_type)),
		cx_args(cx_arg(NULL, cx->any_type)),
		"0 $in &+ for");

  return true;
}
