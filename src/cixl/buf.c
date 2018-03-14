#include <ctype.h>
#include <string.h>

#include "cixl/box.h"
#include "cixl/buf.h"
#include "cixl/cx.h"
#include "cixl/error.h"
#include "cixl/malloc.h"

struct cx_buf *cx_buf_new(struct cx *cx) {
  struct cx_buf *b = cx_malloc(&cx->buf_alloc);
  b->cx = cx;
  cx_vec_init(&b->data, sizeof(unsigned char));
  b->pos = 0;
  b->nrefs = 1;
  return b;
}

struct cx_buf *cx_buf_ref(struct cx_buf *b) {
  b->nrefs++;
  return b;
}

void cx_buf_deref(struct cx_buf *b) {
  cx_test(b->nrefs);
  b->nrefs--;

  if (!b->nrefs) {
    cx_vec_deinit(&b->data);
    cx_free(&b->cx->buf_alloc, b);
  }
}

void cx_buf_push_char(struct cx_buf *b, unsigned char c) {
  *(unsigned char *)cx_vec_push(&b->data) = c;
}

void cx_buf_push_str(struct cx_buf *b, const char *s) {
  size_t len = strlen(s);
  cx_vec_grow(&b->data, b->data.count+len);
  memcpy(b->data.items+b->data.count, s, len);
  b->data.count += len;
}

void cx_buf_clear(struct cx_buf *b) {
  cx_vec_clear(&b->data);
  b->pos = 0;
}

unsigned char *cx_buf_ptr(struct cx_buf *b) {
  return b->data.items + b->pos;
}

size_t cx_buf_len(struct cx_buf *b) {
  return b->data.count - b->pos;
}

static void new_imp(struct cx_box *out) {
  out->as_buf = cx_buf_new(out->type->lib->cx);
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  return x->as_buf == y->as_buf;
}

static bool ok_imp(struct cx_box *v) {
  return cx_buf_len(v->as_buf);
}

static void copy_imp(struct cx_box *dst, const struct cx_box *src) {
  dst->as_buf = cx_buf_ref(src->as_buf);
}

static void dump_imp(struct cx_box *v, FILE *out) {
  struct cx_buf *b = v->as_buf;
  fputs("Buf(", out);
  cx_do_vec(&b->data, unsigned char, c) {
    if (isgraph(*c)) {
      fputc(*c, out);
    } else {
      fprintf(out, "@%d", *c); 
    }
  }

  fputc(')', out);
}

static void deinit_imp(struct cx_box *v) {
  cx_buf_deref(v->as_buf);
}

struct cx_type *cx_init_buf_type(struct cx_lib *lib) {
  struct cx *cx = lib->cx;
  struct cx_type *t = cx_add_type(lib, "Buf", cx->any_type);
  t->new = new_imp;
  t->equid = equid_imp;
  t->ok = ok_imp;
  t->copy = copy_imp;
  t->dump = dump_imp;
  t->deinit = deinit_imp;
  return t;
}
