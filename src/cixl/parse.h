#ifndef CX_PARSE_H
#define CX_PARSE_H

#include <stdbool.h>
#include <stdio.h>

#include "cixl/tok.h"

struct cx;
struct cx_vec;

bool cx_parse_tok(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup);
bool cx_parse_end(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup);
bool cx_parse(struct cx *cx, FILE *in, struct cx_vec *out);
bool cx_parse_str(struct cx *cx, const char *in, struct cx_vec *out);

#endif
