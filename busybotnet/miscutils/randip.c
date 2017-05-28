#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int randip_main () {
        srand(time(NULL));
        printf("%i.%i.%i.%i\n",rand()%255,rand()%255,rand()%255,rand()%255);
        exit (0);
}

