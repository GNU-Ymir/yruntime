#ifndef __PERSONALITY_H_
#define __PERSONALITY_H_

#include "except.h"

/**
 * Personality called when unwinding
 */
_Unwind_Reason_Code _yrt_exc_personality (_Unwind_Action actions,
					  _Unwind_Exception_Class exceptionClass,
					  struct _Unwind_Exception* unwindHeader,
					  struct _Unwind_Context* context);


#endif
