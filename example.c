#include "slconfig.h"
#include <stdarg.h>
#include <stdlib.h>

static char shell[120];
static char editor[120];
static char pkgmgr[120];

static char m_pfx[25];
static char w_pfx[25];
static char e_pfx[25];

void die(const char *reason, ...) {
  va_list list;
  va_start(list, reason);
  vfprintf(stdout, reason, list);
  putc('\n', stdout);
  va_end(list);
  exit(1);
}
void message(const char *msg) { printf("%s %s\n", m_pfx, msg); }
void warn(const char *msg) { printf("%s %s\n", w_pfx, msg); }
void error(const char *msg) { printf("%s %s\n", e_pfx, msg); }

int main(void) {
  slc_pairs pairs = {SLC_PAIR("shell", 120, shell),
                     SLC_PAIR("editor", 120, editor),
                     SLC_PAIR("pkgmgr", 120, pkgmgr),

                     SLC_PAIR("msg_prefix", 25, m_pfx),
                     SLC_PAIR("warn_prefix", 25, w_pfx),
                     SLC_PAIR("error_prefix", 25, e_pfx),

                     SLC_END};

  slc_file conf;
  if (slc_openfile(&conf, "example.conf"))
    die("couldn't open the config file for reading");
  slc_result res;
  if ((res = slc_scanfile(&conf, pairs, SLC_ENV)) != SLC_OK) {
    die("slc_scanfile () failed. Reason: %s, line: %u, column: %u\n",
        slc_geterror(res), conf.line, conf.column);
  }

  printf("the shell of choice is %s\nthe editor %s\nthe package manager %s\n",
         shell, editor, pkgmgr);

  message("this is a message");
  warn("this is a warning");
  error("this is an error");

  return 0;
}
