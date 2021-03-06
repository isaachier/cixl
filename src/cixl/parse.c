#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cixl/box.h"
#include "cixl/mfile.h"
#include "cixl/cx.h"
#include "cixl/error.h"
#include "cixl/func.h"
#include "cixl/int.h"
#include "cixl/lambda.h"
#include "cixl/parse.h"
#include "cixl/str.h"
#include "cixl/vec.h"

static bool parse_type(struct cx *cx,
		       const char *id,
		       struct cx_vec *out,
		       bool lookup) {
  if (lookup) {
    struct cx_type *t = cx_get_type(cx, id, false);
    if (!t) { return false; }
    
    cx_tok_init(cx_vec_push(out),
		CX_TTYPE(),
		cx->row, cx->col)->as_ptr = t;
  } else {
    cx_tok_init(cx_vec_push(out),
		CX_TID(),
		cx->row, cx->col)->as_ptr = strdup(id);
  }

  return true;
}

static bool parse_const(struct cx *cx,
		       const char *id,
		       struct cx_vec *out) {
  cx_tok_init(cx_vec_push(out),
	      CX_TID(),
	      cx->row, cx->col)->as_ptr = strdup(id);
  
  return true;
}

char *parse_fimp(struct cx *cx,
		 struct cx_func *func,
		 FILE *in,
		 struct cx_vec *out) {
  char c = fgetc(in);

  if (c != '<') {
    ungetc(c, in);
    return 0;
  }
  
  int row = cx->row, col = cx->col;
  struct cx_mfile id;
  cx_mfile_open(&id);
  char sep = 0;
  bool done = false;
  
  while (!done) {
    if (!cx_parse_tok(cx, in, out, false)) {
      cx_error(cx, row, col, "Invalid func type");
      cx_mfile_close(&id);
      free(id.data);
      return NULL;
    }
    
    struct cx_tok *tok = cx_vec_pop(out);

    if (tok->type == CX_TID() && strcmp(tok->as_ptr, ">") == 0) {
      cx_tok_deinit(tok);
      break;
    }

    if (sep) { fputc(sep, id.stream); }
    
    if (tok->type == CX_TLITERAL()) {
      cx_dump(&tok->as_box, id.stream);
    } else if (tok->type == CX_TID()) {
      char *s = tok->as_ptr;
      size_t len = strlen(s);

      if (s[len-1] == '>') {
	s[len-1] = 0;
	done = true;
      }

      if (strncmp(s, "Arg", 3) == 0 && isdigit(s[3])) {
	fputs(s, id.stream);
      } else if (isupper(s[0])) {
	struct cx_type *type = cx_get_type(cx, s, false);
	if (!type) { return NULL; }
	fputs(type->id, id.stream);
      } else {
	cx_error(cx, row, col, "Invalid func type: %s", s);
	cx_tok_deinit(tok);
	cx_mfile_close(&id);
	free(id.data);
	return NULL;
      }
    } else {
      cx_error(cx, row, col, "Invalid func type: %s", tok->type->id);
      cx_tok_deinit(tok);
      cx_mfile_close(&id);
      free(id.data);
      return NULL;
    }

    cx_tok_deinit(tok);
    sep = ' ';
  }

  cx_mfile_close(&id);
  return id.data;
}

static bool parse_func(struct cx *cx, const char *id, FILE *in, struct cx_vec *out) {
  bool ref = id[0] == '&';
  int row = cx->row, col = cx->col;
  struct cx_func *f = cx_get_func(cx, ref ? id+1 : id, false);
  if (!f) { return false; }
  
  struct cx_fimp *imp = NULL;
  char *imp_id = parse_fimp(cx, f, in, out);
	  
  if (imp_id) {
    struct cx_fimp **found = cx_set_get(&f->imps, &imp_id);
    free(imp_id);
    
    if (!found) {
      cx_error(cx, row, col, "Fimp not found");
      return false;
    }

    imp = *found;
  }
  
  if (ref) {
    struct cx_box *box = &cx_tok_init(cx_vec_push(out),
				      CX_TLITERAL(),
				      row, col)->as_box;
    if (imp) {
      cx_box_init(box, cx->fimp_type)->as_ptr = imp;
    } else {
      cx_box_init(box, cx->func_type)->as_ptr = f;
    }
  } else {
    if (imp) {
      cx_tok_init(cx_vec_push(out),
		  CX_TFIMP(),
		  row, col)->as_ptr = imp;
    } else {
      cx_tok_init(cx_vec_push(out),
		  CX_TFUNC(),
		  row, col)->as_ptr = f;
    }
  }

  return true;
}

static bool parse_line_comment(struct cx *cx, FILE *in) {
  bool done = false;
  
  while (!done) {
    char c = fgetc(in);

    switch(c) {
    case '\n':
      cx->row++;
      cx->col = 0;
    case EOF:
      done = true;
      break;
    default:
      cx->col++;
      break;
    }
  }

  return true;
}

static bool parse_block_comment(struct cx *cx, FILE *in) {
  int row = cx->row, col = cx->col;
  char pc = 0;
  
  while (true) {
    char c = fgetc(in);

    switch(c) {
    case EOF:
      cx_error(cx, row, col, "Unterminated comment");
      return false;
    case '\n':
      cx->row++;
      cx->col = 0;
      break;
    default:
      cx->col++;
      break;
    }
    
    if (c == '/' && pc == '*') { break; }
    pc = c;
  }

  return true;
}

static bool parse_id(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup) {
  struct cx_mfile id;
  cx_mfile_open(&id);
  bool ok = true;
  int col = cx->col;
  char pc = 0;
  
  while (true) {
    char c = fgetc(in);
    if (c == EOF) { goto done; }
    bool sep = cx_is_separator(cx, c);

    if (col != cx->col &&
	(col-cx->col > 2 || pc != '&') &&
	(sep || c == '<')) {
      ok = ungetc(c, in) != EOF;
      goto done;
    }

    fputc(c, id.stream);
    col++;
    if (sep) { break; }
    pc = c;
  }
 done: {
    cx_mfile_close(&id);

    if (ok) {
      char *s = id.data;

      if (isupper(s[0])) {
	ok = parse_type(cx, s, out, lookup);
      } else if (lookup && s[0] == '#') {
	ok = parse_const(cx, s, out);
      } else if (!lookup || s[0] == '$') {
	cx_tok_init(cx_vec_push(out),
		    CX_TID(),
		    cx->row, cx->col)->as_ptr = strdup(s);
      } else if (s[0] == '/' && s[1] == '/') {
	ok = parse_line_comment(cx, in);
      } else if (s[0] == '/' && s[1] == '*') {
	ok = parse_block_comment(cx, in);
      } else {
	if (lookup) {
	  struct cx_macro *m = cx_get_macro(cx, s, true);
	  
	  if (m) {
	    cx->col = col;
	    ok = m->imp(cx, in, out);
	    goto exit;
	  }
	}
	
	ok = parse_func(cx, s, in, out);
      }

      cx->col = col;
    } else {
      cx_error(cx, cx->row, cx->col, "Failed parsing id");
    }

  exit:
    free(id.data);
    return ok;
  }
}

static bool parse_int(struct cx *cx, FILE *in, struct cx_vec *out) {
  struct cx_mfile value;
  cx_mfile_open(&value);
  int col = cx->col;
  bool ok = true;
  
  while (true) {
    char c = fgetc(in);
    if (c == EOF) { goto exit; }
      
    if (col > cx->col && !isdigit(c)) {
      ok = (ungetc(c, in) != EOF);
      goto exit;
    }
    
    fputc(c, value.stream);
    col++;
  }
  
 exit: {
    cx_mfile_close(&value);
    
    if (ok) {
      int64_t int_value = strtoimax(value.data, NULL, 10);
      free(value.data);
      
      if (int_value || !errno) {
	struct cx_box *box = &cx_tok_init(cx_vec_push(out),
					  CX_TLITERAL(),
					  cx->row, col)->as_box;
	cx_box_init(box, cx->int_type)->as_int = int_value;
	cx->col = col;
      }
    } else {
      cx_error(cx, cx->row, cx->col, "Failed parsing int");
      free(value.data);
    }
    
    return ok;
  }
}

static bool parse_char(struct cx *cx, FILE *in, struct cx_vec *out) {
  int row = cx->row, col = cx->col;
  cx->col++;
  
  char c = fgetc(in);
  
  if (c == EOF) {
    cx_error(cx, row, col, "Invalid char literal");
    return false;
  }

  cx->col++;

  if (c == '@') {
    c = fgetc(in);
    
    switch(c) {
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 's':
      c = ' ';
      break;
    case 't':
      c = '\t';
      break;
    default:      
      ungetc(c, in);
      
      if (isdigit(c)) {
	char n[4];
	
	if (!fgets(n, 4, in)) {
	  cx_error(cx, row, col, "Invalid char literal");
	  return false;
	}

	c = 0;
	
	for (int i=0, m = 100; i<3; i++, m /= 10) {
	  if (!isdigit(n[i])) {
	    cx_error(cx, row, col, "Expected digit: %c", n[i]);
	    return false;
	  }

	  c += (n[i] - '0') * m;
	}
      } else {
	c = '@';
      }
    }
  }
  
  struct cx_box *box = &cx_tok_init(cx_vec_push(out),
				    CX_TLITERAL(),
				    cx->row, cx->col)->as_box;
  cx_box_init(box, cx->char_type)->as_char = c;
  return true;
}

static bool parse_str(struct cx *cx, FILE *in, struct cx_vec *out) {
  struct cx_mfile value;
  cx_mfile_open(&value);
  int row = cx->row, col = cx->col;
  cx->col++;
  bool ok = false;
  
  while (true) {
    char c = fgetc(in);

    if (c == EOF) {
      cx_error(cx, row, col, "Unterminated str literal");
      goto exit;
    }

    if (c == '\'') {
      cx->col++;
      break;
    }

    if (c == '\n') {
      cx->row++;
      cx->col = 0;
    } else if (c == '@') {
      ungetc(c, in);
      if (!parse_char(cx, in, out)) { goto exit; }
      struct cx_tok *t = cx_vec_pop(out);
      c = t->as_box.as_char;
    } else {
      cx->col++;
    }

    fputc(c, value.stream);
  }

  ok = true;
 exit: {
    cx_mfile_close(&value);
    
    if (ok) {
      struct cx_box *box = &cx_tok_init(cx_vec_push(out),
					CX_TLITERAL(),
					row, col)->as_box;

      cx_box_init(box, cx->str_type)->as_str = cx_str_new(value.data, value.size);
    }

    free(value.data);
    return ok;
  }
}

static bool parse_sym(struct cx *cx, FILE *in, struct cx_vec *out) {
  struct cx_mfile id;
  cx_mfile_open(&id);
  int col = cx->col;
  cx->col++;
  bool ok = true;
  
  while (true) {
    char c = fgetc(in);
    if (c == EOF) { goto exit; }
    
    if (cx_is_separator(cx, c)) {
      ok = ungetc(c, in) != EOF;
      goto exit;
    }

    fputc(c, id.stream);
    col++;
  }

  ok = true;
 exit:
  cx_mfile_close(&id);

  if (ok) {
    struct cx_box *box = &cx_tok_init(cx_vec_push(out),
				      CX_TLITERAL(),
				      cx->row, col)->as_box;
    cx_box_init(box, cx->sym_type)->as_sym = cx_sym(cx, id.data);
  }
  
  free(id.data);
  return ok;
}


static bool parse_group(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup) {
  cx->col++;
  struct cx_vec *body = &cx_tok_init(cx_vec_push(out),
				     CX_TGROUP(),
				     cx->row, cx->col)->as_vec;
  cx_vec_init(body, sizeof(struct cx_tok));

  while (true) {
    if (!cx_parse_tok(cx, in, body, lookup)) { return false; }

    if (body->count) {
      struct cx_tok *tok = cx_vec_peek(body, 0);
    
      if (tok->type == CX_TUNGROUP()) {
	cx_tok_deinit(cx_vec_pop(body));
	break;
      }
    }
  }

  return true;
}

static bool parse_stack(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup) {
  cx->col++;
  struct cx_vec *body = &cx_tok_init(cx_vec_push(out),
				     CX_TSTACK(),
				     cx->row, cx->col)->as_vec;
  cx_vec_init(body, sizeof(struct cx_tok));

  while (true) {
    if (!cx_parse_tok(cx, in, body, lookup)) { return false; }

    if (body->count) {
      struct cx_tok *tok = cx_vec_peek(body, 0);
    
      if (tok->type == CX_TUNSTACK()) {
	cx_tok_deinit(cx_vec_pop(body));
	break;
      }
    }
  }

  return true;
}

static bool parse_lambda(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup) {
  int row = cx->row, col = cx->col;
  cx->col++;
  
  struct cx_vec *body = &cx_tok_init(cx_vec_push(out),
				     CX_TLAMBDA(),
				     row, col)->as_vec;
  cx_vec_init(body, sizeof(struct cx_tok));

  while (true) {
    if (!cx_parse_tok(cx, in, body, lookup)) { return false; }

    if (body->count) {
      struct cx_tok *tok = cx_vec_peek(body, 0);
    
      if (tok->type == CX_TUNLAMBDA()) {
	cx_tok_deinit(cx_vec_pop(body));
	break;
      }
    }
  }

  return true;
}

bool cx_parse_tok(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup) {
  int row = cx->row, col = cx->col;
  bool done = false;

  while (!done) {
    char c = fgetc(in);
      
    switch (c) {
    case EOF:
      done = true;
      break;
    case ' ':
    case '\t':
      cx->col++;
      break;
    case '\n':
      cx->row++;
      cx->col = 0;
      break;
    case ';':
      cx_tok_init(cx_vec_push(out), CX_TEND(), row, col);
      cx->col++;
      return true;
    case '(':
      return parse_group(cx, in, out, lookup);
    case ')':
      cx_tok_init(cx_vec_push(out), CX_TUNGROUP(), row, col);
      return true;	
    case '[':
      return parse_stack(cx, in, out, lookup);
    case ']':
      cx_tok_init(cx_vec_push(out), CX_TUNSTACK(), row, col);
      return true;	
    case '{':
      return parse_lambda(cx, in, out, lookup);
    case '}':
      cx_tok_init(cx_vec_push(out), CX_TUNLAMBDA(), row, col);
      return true;
    case '@':
      return parse_char(cx, in, out);
    case '\'':
      return parse_str(cx, in, out);
    case '`':
      return parse_sym(cx, in, out);
    case '-': {
      char c1 = fgetc(in);
      ungetc(c1, in);
      ungetc(c, in);
      
      if (isdigit(c1)) {
	return parse_int(cx, in, out);
      } else {
	return parse_id(cx, in, out, lookup);
      }
	
      break;
    }
    default:
      if (isdigit(c)) {
	ungetc(c, in);
	return parse_int(cx, in, out);
      }
	
      ungetc(c, in);
      return parse_id(cx, in, out, lookup);
    }
  }

  return false;
}

bool cx_parse_end(struct cx *cx, FILE *in, struct cx_vec *out, bool lookup) {
  int depth = 1;
  
  while (depth) {
    if (!cx_parse_tok(cx, in, out, lookup)) { return false; }
    struct cx_tok *tok = cx_vec_peek(out, 0);

    if (tok->type == CX_TID()) {
      char *id = tok->as_ptr;
      if (id[strlen(id)-1] == ':') { depth++; }
    } else if (tok->type == CX_TEND()) {
      depth--;
    }
  }

  cx_vec_pop(out);
  return true;
}

bool cx_parse(struct cx *cx, FILE *in, struct cx_vec *out) {  
  while (!feof(in)) { cx_parse_tok(cx, in, out, true); }
  return !cx->errors.count;
}

bool cx_parse_str(struct cx *cx, const char *in, struct cx_vec *out) {
  int row = cx->row, col = cx->col;
  cx->row = 1; cx->col = 0;
  FILE *is = fmemopen ((void *)in, strlen(in), "r");
  bool ok = false;
  if (!cx_parse(cx, is, out)) { goto exit; }
  ok = true;
 exit:
  cx->row = row; cx->col = col;
  fclose(is);
  return ok;
}
