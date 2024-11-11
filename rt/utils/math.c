#include <rt/utils/math.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>

float _yrt_sqrt (float x) {
    return sqrt (x);
}

void _yrt_init_rand () {
    srand (time (NULL));
}
