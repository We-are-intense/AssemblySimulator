#include "code.h"
#include "cpu/register.h"

inst_t program[INST_LEN] = {
	{
        push_reg,
        {   REG,    0,  0,  (uint64_t *)&reg.rbp,   NULL  },
        {   EMPTY,  0,  0,  NULL,                   NULL  },
        "push   \%rbp"
    }
};