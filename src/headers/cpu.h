#ifndef CPU_DEFINE
#define CPU_DEFINE

#include<stdint.h>
#include<stdlib.h>

/*======================================*/
/*      registers                       */
/*======================================*/
typedef struct REGISTER_STRUCT {
    // return value
    union 
    {
        uint64_t rax;
        uint32_t eax;
        uint16_t ax;
        struct 
        { 
            uint8_t al; 
            uint8_t ah; 
        };
    };
    // callee saved
    union 
    {
        uint64_t rbx;
        uint32_t ebx;
        uint16_t bx;
        struct 
        { 
            uint8_t bl;
            uint8_t bh;
        };
    };
} reg_t;
#endif // !CPU_DEFINE