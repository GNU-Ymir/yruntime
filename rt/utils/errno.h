#ifndef ERRNO_H_
#define ERRNO_H_

#include "../memory/types.h"


int _yrt_get_errno ();
void _yrt_set_errno (int err);
_yrt_slice_t _yrt_str_get_errno (int err);

#endif // ERRNO_H_
