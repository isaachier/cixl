#include <stdlib.h>

#include "cixl/cx.h"
#include "cixl/bin.h"
#include "cixl/box.h"
#include "cixl/error.h"
#include "cixl/eval.h"
#include "cixl/op.h"
#include "cixl/scope.h"
#include "cixl/tok.h"
#include "cixl/types/lambda.h"

struct cx_lambda *cx_lambda_init(struct cx_lambda *lambda, struct cx_scope *scope) {
  lambda->bin = NULL;
  lambda->start_op = -1;
  lambda->num_ops = -1;
  lambda->scope = cx_scope_ref(scope);
  lambda->nrefs = 1;
  cx_vec_init(&lambda->toks, sizeof(struct cx_tok));
  return lambda;
}

struct cx_lambda *cx_lambda_deinit(struct cx_lambda *lambda) {
  cx_do_vec(&lambda->toks, struct cx_tok, t) { cx_tok_deinit(t); }
  cx_vec_deinit(&lambda->toks);
  if (lambda->bin) { cx_bin_unref(lambda->bin); }
  cx_scope_unref(lambda->scope);
  return lambda;
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  return x->as_ptr == y->as_ptr;
}

static bool call_imp(struct cx_box *value, struct cx_scope *scope) {
  struct cx *cx = scope->cx;
  struct cx_lambda *l = value->as_ptr;
  cx_push_scope(cx, l->scope);
  bool ok = false;
  
  if (l->bin) {
    struct cx_bin *prev_bin = cx->bin;
    struct cx_op *prev_op = cx->op;

    cx->bin = l->bin;
    struct cx_op
      *start = cx_vec_get(&l->bin->ops, l->start_op),
      *end = start + l->num_ops;
    cx->op = start;
    
    while (cx->op != end) {
      if (!(ok = cx_eval_next(cx))) { break; }
    }

    cx->bin = prev_bin;
    cx->op = prev_op;
  } else {
    ok = cx_eval(cx, &l->toks, cx_vec_start(&l->toks));
  }
  
  if (cx->scopes.count > 1 && cx_scope(cx, 0) == l->scope) {
    cx_pop_scope(cx, false);
  }
  
  return ok;
}

static void copy_imp(struct cx_box *dst, struct cx_box *src) {
  struct cx_lambda *l = src->as_ptr;
  dst->as_ptr = l;
  l->nrefs++;
}

static void fprint_imp(struct cx_box *value, FILE *out) {
  struct cx_lambda *l = value->as_ptr;
  fprintf(out, "Lambda(%p@%d)", l, l->nrefs);
}

static void deinit_imp(struct cx_box *value) {
  struct cx_lambda *l = value->as_ptr;
  cx_test(l->nrefs > 0);
  l->nrefs--;
  if (!l->nrefs) { free(cx_lambda_deinit(l)); }
}

struct cx_type *cx_init_lambda_type(struct cx *cx) {
  struct cx_type *t = cx_add_type(cx, "Lambda", cx->any_type, NULL);
  t->equid = equid_imp;
  t->call = call_imp;
  t->copy = copy_imp;
  t->fprint = fprint_imp;
  t->deinit = deinit_imp;
  return t;
}
