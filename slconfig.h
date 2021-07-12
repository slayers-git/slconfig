/* slconfig - Unix configuration file parser tool
 * Copyright (C) 2021 Sergey Lafin
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. */

#ifndef __SLCONFIG_H__
#define __SLCONFIG_H__

#include <stdio.h>
#include <string.h>

/* For windows compatibility */
#if defined(_WIN32) || defined(__WIN32__)
#	if defined(slconfig_EXPORTS) /* should be added by cmake */
#		define  SLCONFIG_API __declspec(dllexport)
#	else
#		define  SLCONFIG_API __declspec(dllimport)
#	endif
#elif defined(linux) || defined(__linux)
#	define SLCONFIG_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* type for the slc flags */
typedef char slc_flags;

/* list of the flags */
enum {
	/* should we parse the env variables */
	SLC_ENV = 1,
	/* allow no valuers */
	SLC_ALLOW_NOVAL = (1 << 1),
};

typedef enum slc_result {
	SLC_OK = 0,
	SLC_UNEXP,
	SLC_NOTFOUND,
	SLC_NOVAL,
	SLC_BADVAL,
	SLC_QUOTE,
	SLC_KEYQUOTE,
	SLC_MEQUALS,
	SLC_INVAR,
	SLC_IDSIZE,
	SLC_VALSIZE,
	SLC_VARDECL,

	/* invalid function parameters */
	SLC_INVAL,
} slc_result;


struct slc_file {
	FILE *file;
	unsigned int line;
	unsigned int column;

	/* NULL if built without SLC_VARIABLES */
	struct var_field *fields;
};
typedef struct slc_file slc_file;

/* the slc_pair structure to help scan the file for the entries */
typedef struct __slc_pair {
	/* the key, that will correspond to the value */
	const char *key;
	/* the size of the buffer */
	size_t nr;
	/* the value buffer, should be at least the size of nr */
	void *value;
} slc_pair;

#define SLC_PAIR(key, nr, value)\
	{key, nr, value}
#define SLC_END\
	{ NULL }

typedef slc_pair slc_pairs[];

/* slc_openfile
 * @param file
 *   pointer to an allocated slc_file
 * @param path
 *   path for the config file
 * @return
 *   0 on success, non-zero on failure */
char slc_openfile (slc_file *file, const char *path);

/* slc_closefile
 * @param file
 *   slc_file pointer to be freed */
void slc_closefile (slc_file *file);

/* slc_scanfile
 * @param file
 *   file to be read
 * @param pairs
 *   pairs for the scanner
 * @param flags
 *   flags for the parser
 * @return
 *   zero on success, non-zero on failure 
 * NOTES:
 *   pairs should be null-terminated for the function to work */
SLCONFIG_API char slc_scanfile (slc_file *file, slc_pair *pairs, slc_flags flags);

/* slc_readproperty
 * @param file
 *   file to be read
 * @param key
 *   key that should be found
 * @param value
 *   value buffer
 * @param nr
 *   buffer size
 * @param flags
 *   flags that should be passed to the parser
 * @param err_ptr
 *   pointer to slc_error structure, can be NULL
 * @return
 *   zero on success, non-zero otherwise
 *
 * @note
 *   use of this function is discouraged, it's recommended to use
 *   slc_scanfile () instead. */
SLCONFIG_API char slc_readproperty (slc_file *file, const char *key, char *value, size_t nr, slc_flags flags);

/* slc_writeproperty
 * @param file
 *   file to write to
 * @param key
 *   key of the entry
 * @param value
 *   value of the entry
 * @return
 *   number of bytes written on success, otherwise 0 */
SLCONFIG_API size_t slc_writeproperty (slc_file *file, const char *key, const char * value);

/* slc_geterror
 * @param code
 *   error code to get error 
 * @return
 *   pointer to the error message 
 * NOTES:
 *   if slconfig was compiled without SLC_ERRORS, 
 *   the function will always return NULL */
SLCONFIG_API const char *slc_geterror (slc_result code);

#ifdef __cplusplus
}
#endif

#undef SLCONFIG_API

#endif
