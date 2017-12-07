//
// Created by dcat on 12/7/17.
//
void dprint_(const char *str);
#if defined(dprintf)
#undef dprintf
#undef dprintf_begin
#undef dprintf_cont
#undef dprintf_end
#undef dwprintf
#undef deprintf
#endif
#if DEBUG_PRINT_LEVEL > 0
#define dprintf(str, args...) ({\
strformatw(serial_write, COM1,"[D][%d][%s] " str "\n",current_pid,__func__,##args);\
})
#define dprintf_begin(str, args...)({\
strformatw(serial_write, COM1,"[D][%d][%s] " str ,current_pid,__func__,##args);\
})
#define dprintf_cont(str, args...)({\
strformatw(serial_write, COM1,str,##args);\
})
#define dprintf_end()({\
strformatw(serial_write, COM1,"\n");\
})
#define dwprintf(str, args...) ({\
strformatw(serial_write, COM1,"[W][%d][%s] " str "\n",current_pid,__func__,##args);\
})
#define deprintf(str, args...) ({\
strformatw(serial_write, COM1,"[ERROR][%d][%s] " str "\n",current_pid,__func__,##args);\
})
#else
#define dprintf(str, args...)
#define dprintf_begin(str, args...)
#define dprintf_cont(str, args...)
#define dprintf_end()
#define dwprintf(str, args...)
#define deprintf(str, args...)
#endif