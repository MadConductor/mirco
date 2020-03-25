//#include <stdio.h>

#ifdef DEBUG
  #define debug(args...) fprintf(stderr,args)
#else
  #define debug(args...)
#endif

