#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>
#include <stddef.h>
#include <string.h>
#include "yarray.h"
#include "type.h"

_Unwind_Exception_Class gycExceptionClass =
  ('G' << 56) |
  ('N' << 48) |
  ('U' << 40) |
  ('C' << 32) |
  ('Y' << 24);


/**
 * Checks for GYC exception class.
 */
char isGycExceptionClass(_Unwind_Exception_Class c) 
{
  return c == gycExceptionClass;    
}

typedef struct _yrt_exception_ {
  struct _yrt_exception_* next;
  _yrt_array_ trace;
} _yrt_exception_;

typedef struct  _yrt_exception_header_ {
  struct _Unwind_Exception unwindHeader;

  _yrt_exception_* object; // the Ymir class Exception thrown
  
  // Which catch was found
  int handler;

  // I don't know
  const char* languageSpecificData;

  // Pointer to cathc code
  _Unwind_Ptr landingPad;

  // Canoncal Frame Address (CFA) for the enclosing handler
  _Unwind_Word canonicalFrameAddress;

  // stack other thrown exception in the current thread
  struct _yrt_exception_header_ * next;
    
} _yrt_exception_header_;

// The local stack of chained exceptions
_yrt_exception_header_ * stack;

// Pre allocated storage for 1 instance per thread
// Use calloc / free
_yrt_exception_header_ ehstorage;


/**
 * Terminate the program due to a irrecoverable error
 */
void _yrt_exc_terminate (const char * msg, unsigned int line) {
  static char terminating = 0;
  if (terminating) {
    fprintf (stderr, "terminating called recursively\n");
    abort ();
  }

  terminating = 1;
  fprintf (stderr, "gcc.deh(%u): %s\n", line, msg);

  abort ();
}


/**
 * Allocate and init an _yrt_exception_header_
 */
_yrt_exception_header_* _yrt_exc_create_header (void* object) {
  _yrt_exception_header_* eh = &ehstorage;
  if (eh-> object != NULL) { // pre allocat already in use
    eh = (_yrt_exception_header_*) __builtin_calloc (sizeof (_yrt_exception_header_), 1);
    if (!eh) {
      _yrt_exc_terminate ("out of memory\n", __LINE__);
    }
  }

  eh-> object = object;
  eh-> unwindHeader.exception_class = gycExceptionClass;

  return eh;
}

/**
 * Free exception created by create ()
 */
void _yrt_exc_free_header (_yrt_exception_header_* head) {
  memset (head, 0, sizeof (_yrt_exception_header_));
  if (head != &ehstorage) {
    __builtin_free (head);
  }
}

/**
 * Push the exception header onto the stack 
 */
void _yrt_exc_push (_yrt_exception_header_* e) {
  e-> next = stack;
  stack = e;  
}

/**
 * Pop the last pushed exception of the stack
 */
_yrt_exception_header_* _yrt_exc_pop () {
  _yrt_exception_header_* eh = stack;
  stack = eh-> next;
  return eh;
}


_yrt_exception_header_* _yrt_to_exception_header (struct _Unwind_Exception* exc) {
  // the unwindHeader is the first field of the _yrt_exception_header_
  // So it has the same address 
  return (_yrt_exception_header_*) ((void*) exc); 
}

/**
 * Save handler information in the exception object
 */
void _yrt_exc_save (struct _Unwind_Exception* unwindHeader, _Unwind_Word cfa, int handler, const char* lsda, _Unwind_Ptr landingPad) {
  _yrt_exception_header_* eh = _yrt_to_exception_header (unwindHeader);
  eh-> canonicalFrameAddress = cfa;
  eh-> handler = handler;
  eh-> languageSpecificData = lsda;
  eh-> landingPad = landingPad;
}

/**
 * Restored the catch handler data saved during phase 1
 */
void _yrt_exc_restore (struct _Unwind_Exception* unwindHeader, int * handler, const char** lsda, _Unwind_Ptr* landingPad, _Unwind_Word * cfa) {
  _yrt_exception_header_* eh = _yrt_to_exception_header (unwindHeader);
  *cfa = eh-> canonicalFrameAddress;
  *handler = eh-> handler;
  *lsda = eh-> languageSpecificData;
  *landingPad = (_Unwind_Ptr) eh-> landingPad;
}

/**
 * Called by unwinder when exception object needs destruction outside of ymir runtime
 */
void _yrt_exc_exception_cleanup (_Unwind_Reason_Code code, struct _Unwind_Exception* exc) {
  if (code != _URC_FOREIGN_EXCEPTION_CAUGHT && code != _URC_NO_REASON) {
    _yrt_exc_terminate ("uncaught exception", __LINE__);
  }

  _yrt_exception_header_* eh = _yrt_to_exception_header (exc);
  _yrt_exc_free_header (eh);
}


void _yrt_exc_throw (char *file, char *function, unsigned line, _ytype_info info, void* data) {  
  _yrt_exception_header_* eh = _yrt_exc_create_header (data);

  // Add to thrown exception stack
  _yrt_exc_push (eh);

  eh-> unwindHeader.exception_cleanup = &_yrt_exc_exception_cleanup;
  _Unwind_Reason_Code r = _Unwind_RaiseException (&eh-> unwindHeader);

  if (r == _URC_END_OF_STACK)
    _yrt_exc_terminate ("uncaught exception", __LINE__);

  _yrt_exc_terminate ("unwind error ", __LINE__);  
}

void* _yrt_exc_begin_catch (struct _Unwind_Exception* unwindHeader) {
  _yrt_exception_header_* header = _yrt_to_exception_header (unwindHeader);
  void* object = header-> object;

  // Something went wront when stacking headers
  if (header != _yrt_exc_pop ()) {
    _yrt_exc_terminate ("Catch error", __LINE__);
  }

  // The exception handling is complete
  _Unwind_DeleteException (&header-> unwindHeader);

  return object;
}

_Unwind_Reason_Code _yrt_exc_continue_unwinding (struct _Unwind_Exception* unwindHeader,
						 struct _Unwind_Context * context)
{
  return _URC_CONTINUE_UNWIND;
}


_Unwind_Reason_Code _yrt_exc_scan_lsda (const char* lsda, _Unwind_Exception_Class excClass,
					_Unwind_Action actions,
					struct _Unwind_Exception* unwindHeader,
					struct _Unwind_Context* context,
					_Unwind_Word cfa,
					_Unwind_Ptr* landingPad,
					int * handler)
{
  if (lsda == NULL) return _yrt_exc_continue_unwinding (unwindHeader, context);

  const char * p = lsda;
  
  int start = (context != NULL ? _Unwind_GetRegionStart (context) : 0);
  char LPStartEncoding = *p++;
  _Unwind_Ptr LPStart = 0;
  /* if (LPStartEncoding != DW_EH_PE_omit) */
  /*   LPStart = read_encoded_value (context, LPStartEncoding, p); */
  /* else LPStart = start; */

  _yrt_exc_terminate ("TODO scan", __LINE__);
}


_Unwind_Reason_Code _yrt_exc_personality (_Unwind_Action actions,
					  _Unwind_Exception_Class excClass,
					  struct _Unwind_Exception* unwindHeader,
					  struct _Unwind_Context* context)
{
  const char* lsda;
  _Unwind_Ptr landingPad;
  _Unwind_Word cfa;
  int handler;

  if (actions == (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME) && isGycExceptionClass (excClass)) {
    _yrt_exc_restore (unwindHeader, &handler, &lsda, &landingPad, &cfa);
    if (landingPad == 0) {
      _yrt_exc_terminate ("Unwind error", __LINE__);
    }
  } else {
    lsda = (const char*) (_Unwind_GetLanguageSpecificData (context));
    cfa = _Unwind_GetCFA (context);
    _Unwind_Reason_Code result = _yrt_exc_scan_lsda (lsda, excClass, actions, unwindHeader, context, cfa, &landingPad, &handler);

    if (result) return result;
  }

  if (handler < 0) {
    _yrt_exc_terminate ("unwind error", __LINE__);
  }

  if (isGycExceptionClass (excClass)) {
    _yrt_exception_header_ * eh = _yrt_to_exception_header (unwindHeader);
    const char* currentLsd = lsda;
    _Unwind_Word currentCfa = cfa;
    char bypassed = 0;

    while (eh-> next) {
      _yrt_exception_header_* ehn = eh-> next;
      const char* nextLsd;
      _Unwind_Ptr nextLandingPad;
      _Unwind_Word nextCfa;
      int nextHandler;

      _yrt_exc_restore (&ehn-> unwindHeader, &nextHandler, &nextLsd, &nextLandingPad, &nextCfa);

      if (eh-> object != NULL && ehn-> object != NULL) {
	currentLsd = nextLsd;
	currentCfa = nextCfa;
	eh = ehn;
	bypassed = 1;
	continue;
      }

      if (currentLsd != nextLsd && currentCfa != nextCfa) break;

      // put the exception at the end of the exception chain
      _yrt_exception_* n = ehn-> object;
      while (n-> next != NULL) n = n-> next;            
      n-> next = eh-> object;


      eh-> object = ehn-> object;
      if (nextHandler != handler && bypassed == 0) {
	handler = nextHandler;
	_yrt_exc_save (unwindHeader, cfa, handler, lsda, landingPad);
      }

      eh-> next = ehn-> next;
      _Unwind_DeleteException (&ehn-> unwindHeader);
    }

    if (bypassed == 1) {
      eh = _yrt_to_exception_header (unwindHeader);
      _yrt_exception_header_* ehn = eh-> next;
      eh-> next = ehn-> next;
      _Unwind_DeleteException (&ehn-> unwindHeader);
    }    
  }

  _Unwind_SetGR (context, __builtin_eh_return_data_regno (0), (_Unwind_Ptr) unwindHeader);
  _Unwind_SetGR (context, __builtin_eh_return_data_regno (1), handler);
  _Unwind_SetIP (context, landingPad);

  return _URC_INSTALL_CONTEXT;  
}


_Unwind_Reason_Code __gyc_personality_v0 (int iversion,
					_Unwind_Action actions,
					_Unwind_Exception_Class excClass,
					struct _Unwind_Exception* unwindHeader,
					struct _Unwind_Context* context)
{

  if (iversion != 1) {
    return _URC_FATAL_PHASE1_ERROR;
  }

  return _yrt_exc_personality (actions, excClass, unwindHeader, context);
}
