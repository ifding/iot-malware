#include "busybox.h"
int patator_main( int argc, char *argv[] )
{
  int i;
  printf("hellocmd called:n");
  for (i = 0 ; i < argc ; i++) {
    printf("arg[%d] = %sn", i, argv[i]);
  }
  return 0;
}
