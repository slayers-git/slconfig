#include "slconfig.h"

#include <stdlib.h>

#define SLC_BUFSIZ 1024
#define SLC_KEYSIZ  128
#define SLC_VALSIZ 1024

/* if this is not defined
 * this rule becomes illegal:
 *      var = "#121212"
 * ->   key = $(var) */
#ifdef SLC_VARIABLES
#	define __vars 1
#	include <stdlib.h>

void *xmalloc (size_t nr) {
	void *p = malloc (nr);
	if (!p) {
		puts ("failed to allocate memory");
		abort ();
	}
	return p;
}

/* SLL for variable fields */
static struct var_field {
	const char *name;
	char *value;

	struct var_field *next;
} *fields = NULL;

static struct var_field *__getvar (slc_file *file, const char *name) {
	struct var_field *p = file->fields;
	while (p) {
		if (strcmp (p->name, name) == 0) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}
static void __changevar (slc_file *file, const char *name, const char *value) {
	struct var_field *p;
	if (!(p = __getvar (file, name)))
		p = file->fields;
	if (p == NULL) {
		p = file->fields = xmalloc (sizeof (struct var_field));
	} else {
		while (p->next)
			p = p->next;
			
		p->next = xmalloc (sizeof (struct var_field));
		p = p->next;
	}
	p->next = NULL;
	p->name = xmalloc (strlen (name) + 1);
	p->value = xmalloc (strlen (value) + 1);
	strcpy ((char *)p->name, name);
	strcpy ((char *)p->value, value);
}
static void __clearvars (slc_file *file) {
	struct var_field *p = file->fields;
	void *_p;
	while (p) {
		free ((char *)p->name);
		free ((char *)p->value);

		_p = p->next;
		free (p);
		p = _p;
	}
	file->fields = NULL;
}
#endif

#define __isspace(x)\
	(x == ' ' || x == '\t')
#define __error(msg, ret)\
{\
	puts("slconfig: " msg);\
	return ret;\
}
#define __isvalid(x)\
	(x != '{' &&\
	 x != '}' &&\
	 x != '.' &&\
	 x != '(' &&\
	 x != ')' &&\
	 x != '$')

const char *error_map[] = {
	"success",
	"unexpected character",
	"entry not found",
	"entry has no value",
	"entry has an invalid value",
	"unterminated quote",
	"quoted key",
	"multiple equal sings",
	"invalid variable usage",
	"key exceeds the maximum size",
	"value exceeds the maximum size",
	"variable declaration is invalid",
	"invalid parameters were given",
};

/* read the next line in the static buffer */
static size_t __readnextline (FILE *file, char *buf, size_t nr) {
	size_t res = fread (buf, 1, nr, file);
	if (res == 0)
		return res;
	char *_buf = strchr (buf, '\n');
	*_buf = 0x0;

	size_t ret = (size_t)(_buf - buf);
	if (!ret) {
		fseek (file, -res + 1, SEEK_CUR);
		return 1;
	}
	fseek (file, (ssize_t)ret - (ssize_t)res + 1, SEEK_CUR);

	return ret;
}
/* skip to content by advancing the pointer to the first 
 * non-whitespace character */
static char *__tocontent (char *buf, size_t *len) {
	while (__isspace (*buf)) {
		++buf;
		--(*len);
	}
	return buf;
}
static char *__findnexttoken (char *buf, size_t *len) {
	while (!__isspace (*buf)) {
		if (!(*len) || *buf == 0x0)
			return NULL;
		++buf;
		--(*len);
	}
	*buf = 0x0;
	++buf;
	return __tocontent (buf, len);
}

/* parse the enviromental variable, should be in the format of
 * ${VARNAME} */
static char *__parsevar (slc_file *file, char **buf) {
	/* end of buffer */
	char *eob;
	enum { slc_env, slc_loc } type;

	switch (**buf) {
		case '{':
			type = slc_env;
			break;
		case '(':
			type = slc_loc;
			break;
		default:
			__error ("invalid variable usage", NULL);
	}

	++*buf;
	if (type == slc_env) {
		if ((eob = strchr (*buf, '}')) == NULL) {
			__error ("to match this: '{'", NULL);
		}
	} else {
		if ((eob = strchr (*buf, ')')) == NULL) {
			__error ("to match this: '}'", NULL);
		}
	}
	*eob = 0x0;

	char *var;
	if (type == slc_env) {
		var = getenv (*buf);
		*buf = eob;
		return var;
	} else {
#if __vars
		struct var_field *p = __getvar (file, *buf);
		*buf = eob;
		if (!p) {
			__error ("dereferenced variable does not exist", eob);
		}
		return p->value;
#else
		__error ("user variables are restricted", eob);
#endif
	}
}

/* find the size of the slc_pair structure array */
size_t __arraysize (slc_pairs pairs) {
	size_t i = 0;
	while (*(slc_pair **)pairs != 0) {
		++i; ++pairs;
	}
	return i;
}

typedef char slc_bool;

static char __parsenextline (slc_file *fd, char *key, char *value, size_t key_nr, size_t val_nr, slc_flags flags, size_t *line_len) {
#define addchar(x)\
	if (!right) {\
		if (!key_nr)\
			__error ("key exceeds the maximum id size", SLC_IDSIZE);\
\
		--key_nr;\
		*key = x;\
		++key;\
	} else {\
		if (!val_nr)\
			__error ("key exceed the maximum value size", SLC_VALSIZE);\
\
		--val_nr;\
		*value = x;\
		++value;\
	}\
	comment_line = 0;
	     /* the local copy of `key`   */
	char *__key = key,
	     /* the local copy of `value` */
	     *__val = value,
	     /* the buffer, in which the statement will be read */
	     buf[SLC_BUFSIZ];
	/* pointer, which will be used to traverse the buffer */
	char *ptr = buf;
	/* is the statement enclosed in quotations? */
	enum { not = 0, slc_qsingle, slc_qdouble } inquote = 0;
	/* are we parsing the right-hand side of the statement */
	slc_bool right = 0;
	/* should we terminate the value field as it is not already terminated? */
	slc_bool vterm = 0;
	/* parse? */
	slc_bool parse = 1;
	/* we should ignore default grammar rules if the whole line is a comment */
	slc_bool comment_line = 1;
	/* is the left side a variable declaration? */
	slc_bool vardecl = 0;

	if (!(*line_len = __readnextline (fd->file, buf, SLC_BUFSIZ))) {
		return SLC_OK;
	}
    ptr = __tocontent (buf, line_len);

	while (*ptr && parse) {
		if (inquote) {
			if ((inquote == slc_qdouble && *ptr == '"') ||
				inquote == slc_qsingle && *ptr == '\'') {
				inquote = 0;
				++ptr;
				++fd->column;
				continue;
			}
			addchar (*ptr);
			++ptr;
			++fd->column;
			continue;
		}
		switch (*ptr) {
			case ' ':
				break;
			case '#': /* comment */
				parse = 0;
				break;
			case '"':  /* quotation*/
				if (!right)
					__error ("key has quotation marks", SLC_KEYQUOTE);
				inquote = slc_qdouble;
				break;
			case '\'': /* also a valid quotation */
				if (!right)
					__error ("key has quotation marks", SLC_KEYQUOTE);
				inquote = slc_qsingle;
				break;
			case '=': /* equals sign */
				if (right)
					__error ("double equal sign", SLC_MEQUALS);
				right = 1;
				break;
			case '$': /* variable reference */
				++ptr;
				++fd->column;
				if (!right) {
					if (*ptr == '(' || *ptr == '{') {
						__error ("variable dereference outside of RHS", SLC_VARDECL);
					}
					while (*ptr != ' ' && *ptr != '=' && *ptr != 0x0) {
						if (!__isvalid (*ptr)) {
							__error ("variable declaration contains dissalowed symbols", SLC_VARDECL);
						}
						addchar (*ptr);
						++ptr;
						++fd->column;
					}
					
					vardecl = 1;
					break;
				}
				if (inquote) /* we don't want to interpret anything in quoutes */
					continue;
				char *_ptr = ptr,
				     *_var;
				_var = __parsevar (fd, &ptr);
				if (!_var) {
					__error ("variable has no value", SLC_NOVAL);
				}
				size_t s = strlen (_var);
				fd->column += (_ptr - ptr);
				memcpy (value, _var, s);
				value += s;
				break;
			default: /* identifier or a value */
				addchar(*ptr);
				break;
		}
		++fd->column;
		++ptr;
	}

	*key = 0x0;
	if (!vterm)
		*value = 0x0;

	if (inquote)
		__error ("unclosed quoute", SLC_QUOTE);
	if (!comment_line && (!right || (*__val == 0x0 && !(flags & SLC_ALLOW_NOVAL))))
		__error ("no value was specified", SLC_NOVAL);
	if (vardecl) {
#if __vars
		__changevar (fd, __key, __val);
#else
		__error ("invalid variable declaration", SLC_VARDECL);
#endif
	}
	return SLC_OK;
#undef addchar
}

char slc_openfile (slc_file *file, const char *path) {
	file->column = 1;
	file->line   = 1;
	file->fields = NULL;
	return (file->file = fopen (path, "r+")) ? 0 : 1;
}
void slc_closefile (slc_file *file) {
	fclose (file->file);
#if __vars
	__clearvars (file);
#endif
}

char slc_scanfile (slc_file *file, slc_pair *pairs, slc_flags flags) {
	fseek (file->file, 0, SEEK_SET);

	char key  [SLC_KEYSIZ], /* a buffer for the key, that will be read */
	     value[SLC_VALSIZ], /* a buffer for the value */
	     buf  [SLC_BUFSIZ]; /* general buffer to store the line contents */

	size_t psize = __arraysize (pairs),
	       i,
	       line_len;
	if (psize == 0) {
		return SLC_INVAL;
	}

	slc_result res;
	while ((res = __parsenextline (file, key, value, SLC_KEYSIZ, SLC_VALSIZ, flags, &line_len)) == SLC_OK && line_len != 0) {
		for (i = 0; i < psize; ++i) {
			if (strcmp (pairs[i].key, key) == 0) {
				strncpy (pairs[i].value, value, pairs[i].nr);
			}
		}
		file->column = 1;
		++file->line;
	}

	return res;
}

char slc_readproperty (slc_file *file, const char *key, char *value, size_t nr, slc_flags flags) {
	fseek (file->file, 0, SEEK_SET);

	char __key[SLC_KEYSIZ],
	     buffer[SLC_BUFSIZ],
	     nl;
	size_t line_len;

	slc_result res;
	while ((res = __parsenextline (file, __key, value, SLC_KEYSIZ, nr, flags, &line_len)) == SLC_OK && line_len != 0) {
		if (strcmp (__key, key) == 0) 
			break;
	}
	return res;
}

size_t slc_writeproperty (slc_file *file, const char *key, const char *value) {
	if (!key || !value)
		return 0;

	size_t skey = strlen (key),
	       sval = strlen (value);

	if ((skey + sval + 5) >= SLC_BUFSIZ) {
		__error ("buffer size was exceeded.", 0);
	}

	fseek (file->file, 0, SEEK_END);
	if (getc (file->file) != '\n')
		putc ('\n', file->file);

	char buf[SLC_BUFSIZ];

	strcpy (buf, key);
	strcat (buf, " = ");
	strcat (buf, value);
	strcat (buf, "\n");

	return fwrite (buf, 1, strlen (buf), file->file);
}

const char *slc_geterror (slc_result code) {
#ifdef SLC_ERRORS
	return error_map [code];
#else
	return NULL;
#endif
}
