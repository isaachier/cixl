#include "cixl/cx.h"
#include "cixl/box.h"
#include "cixl/error.h"
#include "cixl/func.h"
#include "cixl/types/nil.h"
#include "cixl/scope.h"

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  return true;
}

static bool ok_imp(struct cx_box *x) {
  return false;
}

static void fprint_imp(struct cx_box *v, FILE *out) {
  fputc('_', out);
}

struct cx_type *cx_init_nil_type(struct cx *cx) {
  struct cx_type *t = cx_add_type(cx, "Nil", cx->opt_type);
  t->equid = equid_imp;
  t->ok = ok_imp;
  t->fprint = fprint_imp;
  return t;
}
