#include <rt/except/personnality.h>
#include <rt/except/stacktrace.h>
#include <rt/except/panic.h>

#define DW_EH_PE_absptr 0x00
#define DW_EH_PE_omit   0xff

#define DW_EH_PE_uleb128  0x01
#define DW_EH_PE_udata2   0x02
#define DW_EH_PE_udata4   0x03
#define DW_EH_PE_udata8   0x04
#define DW_EH_PE_sleb128  0x09
#define DW_EH_PE_sdata2   0x0A
#define DW_EH_PE_sdata4   0x0B
#define DW_EH_PE_sdata8   0x0C
#define DW_EH_PE_signed   0x08

#define DW_EH_PE_pcrel    0x10
#define DW_EH_PE_textrel  0x20
#define DW_EH_PE_datarel  0x30
#define DW_EH_PE_funcrel  0x40
#define DW_EH_PE_aligned  0x50

#define DW_EH_PE_indirect 0x80

void _yrt_exc_save (struct _Unwind_Exception* unwindHeader, int handler, const char* lsda, _Unwind_Ptr landingPad, _Unwind_Word cfa) {
    _yrt_exception_header_t * eh = _yrt_to_exception_header (unwindHeader);
    eh-> lsda = lsda;
    eh-> handler = handler;
    eh-> landingPad = landingPad;
    eh-> cfa = cfa;
}

void _yrt_exc_restore (struct _Unwind_Exception* unwindHeader, int * handler, const char** lsda, _Unwind_Ptr* landingPad, _Unwind_Word* cfa) {
    _yrt_exception_header_t * eh = _yrt_to_exception_header (unwindHeader);
    *lsda = eh-> lsda;
    *handler = eh-> handler;
    *landingPad = eh-> landingPad ;
    *cfa = eh-> cfa;
}

uint32_t _yrt_exc_size_of_encoded_value (uint8_t encoding) {
    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0x07) {
    case DW_EH_PE_absptr :
        return sizeof (void*);
    case DW_EH_PE_udata2 :
        return 2;
    case DW_EH_PE_udata4 :
        return 4;
    case DW_EH_PE_udata8 :
        return 8;
    default :
        _yrt_exc_terminate ("reading encoded", __FILE__, __FUNCTION__, __LINE__);
    }
}

_Unwind_Ptr _yrt_exc_base_of_encoded_value (uint8_t encoding, struct _Unwind_Context* context) {
    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0x70) {
    case DW_EH_PE_absptr:
    case DW_EH_PE_pcrel:
    case DW_EH_PE_aligned:
        return 0;
    case DW_EH_PE_textrel:
        return _Unwind_GetTextRelBase (context);
    case DW_EH_PE_datarel:
        return _Unwind_GetDataRelBase (context);
    case DW_EH_PE_funcrel:
        return _Unwind_GetRegionStart (context);
    default :
        _yrt_exc_terminate ("reading encoded", __FILE__, __FUNCTION__, __LINE__);
    }
}

_Unwind_Internal_Ptr _yrt_exc_read_uleb128 (const char** pref) {
    _Unwind_Internal_Ptr result = 0;
    uint32_t shift = 0;

    const char*p = *pref;

    while (1) {
        uint8_t b = *p++;

        result |= (_Unwind_Internal_Ptr) (b & 0x7F) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
    }

    *pref = p;
    return result;
}

_Unwind_Internal_Ptr _yrt_exc_read_sleb128 (const char** pref) {
    _Unwind_Internal_Ptr result = 0;
    uint32_t shift = 0;
    uint8_t b = 0;

    const char*p = *pref;

    while (1) {
        b = *p++;

        result |= (_Unwind_Internal_Ptr) (b & 0x7F) << shift;
        shift += 7;
        if ((b & 0x80) == 0) break;
    }

    if (shift < sizeof (_Unwind_Internal_Ptr) * 8 && (b & 0x40)) {
        result |= -((_Unwind_Internal_Ptr)1 << shift);
    }

    *pref = p;
    return result;
}


void _yrt_exc_read_unaligned (const char** p, void* data, size_t size) {
    memcpy (data, *p, size);
    *p += size;
}


_Unwind_Ptr _yrt_exc_read_encoded_value_with_base (uint8_t encoding, _Unwind_Ptr base, const char** p) {
    const char** psave = p;
    _Unwind_Internal_Ptr result;

    if (encoding == DW_EH_PE_aligned) {
        _Unwind_Internal_Ptr a = (_Unwind_Internal_Ptr) p;
        a = (a + sizeof (void*) - 1) & (sizeof (void*));
        result = *(_Unwind_Internal_Ptr*) a;
        *p = (const char*) (a + sizeof (void*));
    } else {
        switch (encoding & 0x0f) {
        case DW_EH_PE_uleb128 :
            result = (_Unwind_Internal_Ptr) (_yrt_exc_read_uleb128 (p));
            break;

        case DW_EH_PE_sleb128 :
            result = (_Unwind_Internal_Ptr) (_yrt_exc_read_sleb128 (p));
            break;

        case DW_EH_PE_udata2 :
        case DW_EH_PE_sdata2 :
        {
            uint16_t x;
            _yrt_exc_read_unaligned (p, &x, sizeof (uint16_t));
            result = (_Unwind_Internal_Ptr) (x);
            break;
        }
        case DW_EH_PE_udata4 :
        case DW_EH_PE_sdata4 :
        {
            uint32_t x;
            _yrt_exc_read_unaligned (p, &x, sizeof (uint32_t));
            result = (_Unwind_Internal_Ptr) (x);
            break;
        }
        case DW_EH_PE_udata8 :
        case DW_EH_PE_sdata8 :
        {
            uint64_t x;
            _yrt_exc_read_unaligned (p, &x, sizeof (uint64_t));
            result = (_Unwind_Internal_Ptr) (x);
            break;
        }
        case DW_EH_PE_absptr :
        {
            size_t x;
            _yrt_exc_read_unaligned (p, &x, sizeof (size_t));
            result = (_Unwind_Internal_Ptr) (x);
            break;
        }
        default :
            _yrt_exc_terminate ("reading encoded", __FILE__, __FUNCTION__, __LINE__);
        }
    }

    if (result != 0) {
        if ((encoding & 0x70) == DW_EH_PE_pcrel) result += (_Unwind_Internal_Ptr) psave;
        else result += base;

        if (encoding & DW_EH_PE_indirect) {
            result = *(_Unwind_Ptr*)result;
        }
    }

    return result;
}

_Unwind_Ptr _yrt_exc_read_encoded_value (struct _Unwind_Context* context, uint8_t encoding, const char** p) {
    _Unwind_Ptr base = _yrt_exc_base_of_encoded_value (encoding, context);
    return _yrt_exc_read_encoded_value_with_base (encoding, base, p);
}

int32_t _yrt_exc_action_table_lookup (_Unwind_Action actions,
                                  struct _Unwind_Exception* unwindHeader,
                                  const char* actionRecord,
                                  _Unwind_Ptr TTypeBase,
                                  const char* TType,
                                  uint8_t TTypeEncoding,
                                  uint8_t * saw_handler,
                                  uint8_t * saw_cleanup)
{
    while (1) {
        const char* ap = actionRecord;
        _Unwind_Internal_Ptr ARFilter = _yrt_exc_read_sleb128 (&ap);
        const char* apn = ap;
        _Unwind_Internal_Ptr ARDisp = _yrt_exc_read_sleb128 (&ap);

        if (ARFilter == 0) {
            *saw_cleanup = 1;
        } else if (actions & _UA_FORCE_UNWIND) {}
        else if (ARFilter > 0) {
            size_t encodedSize = _yrt_exc_size_of_encoded_value (TTypeEncoding);

            const char* tp = TType - ARFilter * encodedSize;

            _Unwind_Ptr entry = _yrt_exc_read_encoded_value_with_base (TTypeEncoding, TTypeBase, &tp);
            *saw_handler = 1;

            return ARFilter;
        } else break;

        if (ARDisp == 0) break;
        actionRecord = apn + ARDisp;
    }

    return 0;
}


_Unwind_Reason_Code _yrt_exc_scan_lsda (const char* lsda,
                                        _Unwind_Action actions,
                                        struct _Unwind_Exception* unwindHeader,
                                        struct _Unwind_Context* context,
                                        _Unwind_Word cfa,
                                        _Unwind_Ptr* landingPad,
                                        int* handler)
{
    if (lsda == NULL) return _URC_CONTINUE_UNWIND;

    const char* p = lsda;
    _Unwind_Ptr start = (context ? _Unwind_GetRegionStart (context) : 0);

    uint8_t LPStartEncoding = (uint8_t) *p++;
    _Unwind_Ptr LPStart = 0;

    if (LPStartEncoding != DW_EH_PE_omit) {
        LPStart = _yrt_exc_read_encoded_value (context, LPStartEncoding, &p);
    } else LPStart = start;

    uint8_t TTypeEncoding = *p++;
    const uint8_t* TType = NULL;

    if (TTypeEncoding != DW_EH_PE_omit) {
        _Unwind_Internal_Ptr TTBase = _yrt_exc_read_uleb128 (&p);
        TType = p + TTBase;
    }

    uint8_t CSEncoding = *p++;
    _Unwind_Internal_Ptr CSTableSize = _yrt_exc_read_uleb128 (&p);
    const char* actionTable = p + CSTableSize;

    _Unwind_Internal_Ptr TTypeBase = _yrt_exc_base_of_encoded_value (TTypeEncoding, context);

    int ip_before_insn;
    _Unwind_Internal_Ptr ip = _Unwind_GetIPInfo (context, &ip_before_insn);
    if (!ip_before_insn) ip -= 1;

    uint8_t saw_cleanup = 0;
    uint8_t saw_handler = 0;
    const uint8_t * actionRecord = NULL;

    while (p < actionTable) {
        _Unwind_Internal_Ptr CSStart = _yrt_exc_read_encoded_value (NULL, CSEncoding, &p);
        _Unwind_Internal_Ptr CSLen = _yrt_exc_read_encoded_value (NULL, CSEncoding, &p);
        _Unwind_Internal_Ptr CSLandingPad = _yrt_exc_read_encoded_value (NULL, CSEncoding, &p);
        _Unwind_Internal_Ptr CSAction = _yrt_exc_read_uleb128 (&p);

        if (ip < start + CSStart) {
            p = actionTable;
        } else if (ip < start + CSStart + CSLen) {
            if (CSLandingPad) *landingPad = LPStart + CSLandingPad;
            if (CSAction) actionRecord = actionTable + CSAction - 1;
            break;
        }
    }

    if (landingPad == 0) {
    } else if (actionRecord == NULL) {
        saw_cleanup = 1;
    } else {
        *handler = _yrt_exc_action_table_lookup (actions, unwindHeader, actionRecord,
                                                 TTypeBase, TType, TTypeEncoding,
                                                 &saw_handler, &saw_cleanup);
    }

    if (!saw_cleanup && !saw_handler) {
        return _URC_CONTINUE_UNWIND;
    }

    if (actions & _UA_SEARCH_PHASE) {
        if (!saw_handler) return _URC_CONTINUE_UNWIND;

        _yrt_exc_save (unwindHeader, *handler, lsda, *landingPad, cfa);

        return _URC_HANDLER_FOUND;
    }

    return 0;
}




_Unwind_Reason_Code _yrt_exc_personality (_Unwind_Action actions,
                                          _Unwind_Exception_Class excClass,
                                          struct _Unwind_Exception* unwindHeader,
                                          struct _Unwind_Context* context)
{
    const char* lsda = NULL;
    _Unwind_Ptr landingPad = 0;
    _Unwind_Word cfa = 0;
    int handler = 0;


    if (actions == (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME)) {
        _yrt_exc_restore (unwindHeader, &handler, &lsda, &landingPad, &cfa);
        if (landingPad == 0) {
            _yrt_exc_terminate ("unwind error", __FILE__, __FUNCTION__, __LINE__);
        }
    } else {
        lsda = _Unwind_GetLanguageSpecificData (context);
        cfa = _Unwind_GetCFA (context);

        char result = _yrt_exc_scan_lsda (lsda, actions, unwindHeader, context, cfa, &landingPad, &handler);

        if (result) return result;
    }

    if (landingPad == 0) {
        return _URC_CONTINUE_UNWIND;
    }

    _Unwind_SetGR (context, __builtin_eh_return_data_regno (0),
                   (_Unwind_Ptr) unwindHeader);
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

#ifdef _WIN32

EXCEPTION_DISPOSITION __gyc_personality_seh0 (void* ms_exc, void* this_frame,
                                              void* ms_orig_context, void* ms_disp)
{
    return _GCC_specific_handler(ms_exc, this_frame, ms_orig_context,
                                 ms_disp, &__gyc_personality_v0);
}
#endif
