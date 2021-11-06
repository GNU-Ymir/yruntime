#include <math.h>
#include <stdlib.h>

float _yrt_sqrt (float x) {
    return sqrt (x);
}


void _yrt_init_rand () {
    srand (time (NULL));
}
