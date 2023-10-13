#include "utils.hpp"

typedef enum {
  ARG_NULL = 0,
  ARG_BOOL_SET,
  ARG_BOOL_CLR,
  ARG_STRING,
  ARG_INT,
  ARG_HEX,
  ARG_FLOAT,
  ARG_DOUBLE,
  ARG_USAGE
} arg_t;

struct args {
  const char *arg;	// FIXME: have short/long form, eg:  -v --verbose
  arg_t       type;
  void        *flag;
  const char *desc;
};

void
do_usage(const struct args *ap, const char *usage);

void
doCommandLineArgs(int32_t *argcp, char ***argvp, const struct args *ap, int32_t minparams, int32_t maxparams, const char *usage);
