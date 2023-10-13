#include "do_args.hpp"

void
do_usage(const struct args *ap, const char *usage)
{
  epf("%s\n", usage);
  while (ap->arg != NULL) {
    epf(" %12s %s\n", ap->arg, ap->desc);
    ++ap;
  }
  exit(1);
}

void
doCommandLineArgs(int32_t *argcp, char ***argvp, const struct args *ap, int32_t minparams, int32_t maxparams, const char *usage)
{
  int32_t argc = *argcp;
  char  **argv = *argvp;
  while (argc > 1 && argv[1][0] == '-') {
    for (uint32_t i = 0; ; ++i) {
      if (ap[i].arg == NULL) {
        epf("Bad flag: %s\n", argv[1]);
        do_usage(ap, usage);
      }
      if (strcmp(argv[1], ap[i].arg) == 0) {
	char *endptr = NULL;
	arg_t t = ap[i].type;
        switch (t) {
        case ARG_USAGE:         do_usage(ap, usage);                break;
        case ARG_BOOL_SET:      * (unsigned char *) ap[i].flag = 1; break;
        case ARG_BOOL_CLR:      * (unsigned char *) ap[i].flag = 0; break;
	  
	case ARG_STRING:
	case ARG_INT:
	case ARG_HEX:
	case ARG_FLOAT:
	  if (argc < 3) {
	    do_usage(ap, usage);
	  }
	  if (t == ARG_STRING) {
	    *(char **) ap[i].flag = argv[2];
	  } else {
	    if (t == ARG_HEX) {
	      * (uint32_t *) ap[i].flag = strtol(argv[2], &endptr, 16);
	    } else if (t == ARG_INT) {
	      * (int32_t *) ap[i].flag = strtol(argv[2], &endptr,  0);
	    } else if (t == ARG_FLOAT) {
	      * (float *) ap[i].flag = strtod(argv[2], &endptr);
	    } else if (t == ARG_DOUBLE) {
	      * (double *) ap[i].flag = strtod(argv[2], &endptr);
	    } else {
	      Assert(0);
	    }
	    if (*endptr != '\0') {
	      epf("Invalid argument for %s: %s\n", argv[1], argv[2]);
	      do_usage(ap, usage);
	    }
	  }
	  --argc;
	  ++argv;
	  break;

        default:
          fatal("Bad arg type: %d", ap[i].type);
        }
        break;
      }
    }
    --argc;
    ++argv;
  }
  if (argc < minparams + 1 || argc > maxparams + 1) {
    do_usage(ap, usage);
  }
  *argcp = argc;
  *argvp = argv;
  return;
}
