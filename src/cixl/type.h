#ifndef CX_TYPE_H
#define CX_TYPE_H

#include <stdint.h>
#include <stdio.h>
#include "cixl/set.h"

struct cx;
struct cx_box;
struct cx_iter;
struct cx_scope;

struct cx_type {
  struct cx_lib *lib;
  char *id, *emit_id;
  size_t tag, level;
  struct cx_set parents, children;
  struct cx_vec is;
  bool trait;
  
  void (*new)(struct cx_box *);
  bool (*eqval)(struct cx_box *, struct cx_box *);
  bool (*equid)(struct cx_box *, struct cx_box *);
  enum cx_cmp (*cmp)(const struct cx_box *, const struct cx_box *);
  bool (*call)(struct cx_box *, struct cx_scope *);
  bool (*ok)(struct cx_box *);
  void (*copy)(struct cx_box *dst, const struct cx_box *src);
  void (*clone)(struct cx_box *dst, struct cx_box *src);
  struct cx_iter *(*iter)(struct cx_box *);
  void (*write)(struct cx_box *, FILE *);
  void (*dump)(struct cx_box *, FILE *);
  void (*print)(struct cx_box *, FILE *);
  bool (*emit)(struct cx_box *, const char *, FILE *);
  void (*deinit)(struct cx_box *);

  void *(*type_deinit)(struct cx_type *);
};

struct cx_type *cx_type_init(struct cx_type *type,
			     struct cx_lib *lib,
			     const char *id);

struct cx_type *cx_type_reinit(struct cx_type *type);
void *cx_type_deinit(struct cx_type *type);

void cx_derive(struct cx_type *child, struct cx_type *parent);
bool cx_is(const struct cx_type *child, const struct cx_type *parent);

struct cx_type *cx_init_meta_type(struct cx_lib *lib);

#endif
