#ifndef PERSONNALITY_H_
#define PERSONNALITY_H_

#include <rt/except/exception.h>

void _yrt_exc_save (struct _Unwind_Exception* unwindHeader, int handler, const char* lsda, _Unwind_Ptr landingPad, _Unwind_Word cfa);

void _yrt_exc_restore (struct _Unwind_Exception* unwindHeader, int * handler, const char** lsda, _Unwind_Ptr* landingPad, _Unwind_Word* cfa);

uint32_t _yrt_exc_size_of_encoded_value (uint8_t encoding);

_Unwind_Ptr _yrt_exc_base_of_encoded_value (uint8_t encoding, struct _Unwind_Context* context);

_Unwind_Internal_Ptr _yrt_exc_read_uleb128 (const char** pref);

_Unwind_Internal_Ptr _yrt_exc_read_sleb128 (const char** pref);

void _yrt_exc_read_unaligned (const char** p, void* data, size_t size);

_Unwind_Ptr _yrt_exc_read_encoded_value (struct _Unwind_Context* context, uint8_t encoding, const char** p);

int32_t _yrt_exc_action_table_lookup (_Unwind_Action actions,
									  struct _Unwind_Exception* unwindHeader,
									  const char* actionRecord,
									  _Unwind_Ptr TTypeBase,
									  const char* TType,
									  uint8_t TTypeEncoding,
									  uint8_t * saw_handler,
									  uint8_t * saw_cleanup);

_Unwind_Reason_Code _yrt_exc_scan_lsda (const char* lsda,
                                        _Unwind_Action actions,
                                        struct _Unwind_Exception* unwindHeader,
                                        struct _Unwind_Context* context,
                                        _Unwind_Word cfa,
                                        _Unwind_Ptr* landingPad,
                                        int* handler);

/**
 * Personality called when unwinding
 */
_Unwind_Reason_Code _yrt_exc_personality (_Unwind_Action actions,
										  _Unwind_Exception_Class exceptionClass,
										  struct _Unwind_Exception* unwindHeader,
										  struct _Unwind_Context* context);




#endif // PERSONNALITY_H_
