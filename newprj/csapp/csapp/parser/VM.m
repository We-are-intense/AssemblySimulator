//
//  VM.m
//  csapp
//
//  Created by erfeixia on 2024/4/14.
//

#import "VM.h"
typedef struct REGISTER_STRUCT {
    union {
        uint64_t rax; uint32_t eax; uint16_t ax;
        struct { uint8_t al; uint8_t ah; };
    };
    union {
        uint64_t rbx; uint32_t ebx; uint16_t bx;
        struct { uint8_t bl; uint8_t bh; };
    };
    union {
        uint64_t rcx; uint32_t ecx; uint16_t cx;
        struct { uint8_t cl; uint8_t ch; };
    };
    union {
        uint64_t rdx; uint32_t edx; uint16_t dx;
        struct { uint8_t dl; uint8_t dh; };
    };
    union { 
        uint64_t rsi; uint32_t esi; uint16_t si;
        struct { uint8_t sil; uint8_t sih; };
    };
    union {
        uint64_t rdi; uint32_t edi; uint16_t di;
        struct { uint8_t dil; uint8_t dih; };
    };
    union {
        uint64_t rbp; uint32_t ebp; uint16_t bp;
        struct { uint8_t bpl; uint8_t bph; };
    };
    union {
        uint64_t rsp; uint32_t esp; uint16_t sp;
        struct { uint8_t spl; uint8_t sph; };
    };
    union {
        uint64_t r8; uint32_t r8d; uint16_t r8w; uint8_t  r8b;
    };
    union {
        uint64_t r9; uint32_t r9d; uint16_t r9w; uint8_t  r9b;
    };
    union {
        uint64_t r10; uint32_t r10d; uint16_t r10w; uint8_t  r10b;
    };
    union {
        uint64_t r11; uint32_t r11d; uint16_t r11w; uint8_t  r11b;
    };
    union {
        uint64_t r12; uint32_t r12d; uint16_t r12w; uint8_t  r12b;
    };
    union {
        uint64_t r13; uint32_t r13d; uint16_t r13w; uint8_t  r13b;
    };
    union {
        uint64_t r14; uint32_t r14d; uint16_t r14w; uint8_t  r14b;
    };
    union {
        uint64_t r15; uint32_t r15d; uint16_t r15w; uint8_t  r15b;
    };
} reg_t;

typedef struct CPU_FLAGS_STRUCT {
    union
    {
        uint64_t __cpu_flag_value;
        struct
        {
            // carry flag: 进位标识，最近的操作产生了进位
            uint16_t CF;
            // zero flag: 零标识，最近的操作结果为零
            uint16_t ZF;
            // sign flag: 负号标识，最近的操作结果为负数
            uint16_t SF;
            // overflow flag: 溢出标识，最近的操作导致一个补码溢出: 正溢出或者负溢出
            uint16_t OF;
        };
    };
} cpu_flag_t;

typedef struct CORE_STRUCT {
    // program counter or instruction pointer
    union {
        uint64_t rip;
        uint32_t eip;
    };
    // cpu flags
    cpu_flag_t flags;
    // register files
    reg_t       reg;
} core_t;

#define MM_LEN 1000

@interface VM () {
    uint8_t memory[MM_LEN];
}
@property (nonatomic, strong, readwrite) NSArray <Express *>* expresses;
@property (nonatomic, assign) core_t core;
@end

@implementation VM
- (instancetype)initWithExpresses:(NSArray <Express *>*)expresses {
    self = [super init];
    if (self) {
        self.expresses = expresses;
    }
    return self;
}

- (void)run {
    BOOL isRun = YES;
    uint64_t rip = self.core.rip;
    while (isRun) {
        Express *express = self.expresses[rip];
        switch (express.op) {
            case INST_MOV:
            {
                break;
            }
            case INST_PUSH:
            {
                break;
            }
            case INST_POP:
            {
                break;
            }
            case INST_LEAVE:
            {
                break;
            }
            case INST_CALL:
            {
                break;
            }
            case INST_RET:
            {
                break;
            }
            case INST_ADD:
            {
                break;
            }
            case INST_SUB:
            {
                break;
            }
            case INST_CMP:
            {
                break;
            }
            case INST_JNE:
            {
                break;
            }
            case INST_JMP:
            {
                break;
            }
            default:
                break;
        }
    }
}

- (uint64_t)regType:(RegType)type {
    switch (type) {
        case RegType_rax: return self.core.reg.rax;
        case RegType_rsi: return self.core.reg.rsi;
        case RegType_rdi: return self.core.reg.rdi;
        default: break;
    }
    NSAssert(NO, @"not find RegType: %ld", type);
    return 0;
}

- (void)regType:(RegType)type value:(uint64_t)value {
    core_t core = self.core;
    switch (type) {
        case RegType_rax: core.reg.rax = value; break;
        case RegType_rsi: core.reg.rsi = value; break;
        case RegType_rdi: core.reg.rdi = value; break;
        default: NSAssert(NO, @"set value not find RegType: %ld", type); break;
    }
}

- (uint64_t)decodeNode:(Node *)node {
    switch (node.odType) {
        case REG:
        {
            return [self regType:node.reg1];
        }
        case MM_REG:
        {
            return [self readRegType:node.reg1];
        }
        case IMM:
        {
            return node.imm;
        }
        case MM_IMM:
        {
            return [self read64bits_dram_virtual:node.imm];
        }
        case MM_IMM_REG:/// 0x123(%rax)
        {
            uint64_t value = [self regType:node.reg1] + node.imm;
            return [self read64bits_dram_virtual:value];
        }
        case MM_REG1_REG2:
        {
            uint64_t value = [self regType:node.reg1] + [self regType:node.reg2];
            return [self read64bits_dram_virtual:value];
        }
        case MM_IMM_REG1_REG2:
        {
            uint64_t value = [self regType:node.reg1] + [self regType:node.reg2] + node.imm;
            return [self read64bits_dram_virtual:value];
        }
        case MM_REG2_S:
        {
            uint64_t value = [self regType:node.reg2] * node.s;
            return [self read64bits_dram_virtual:value];
        }
        case MM_IMM_REG2_S:
        {
            uint64_t value = [self regType:node.reg2] * node.s + node.imm;
            return [self read64bits_dram_virtual:value];
        }
        case MM_REG1_REG2_S:
        {
            uint64_t value = [self regType:node.reg1] + [self regType:node.reg2] * node.s;
            return [self read64bits_dram_virtual:value];
        }
        case MM_IMM_REG1_REG2_S:
        {
            uint64_t value = [self regType:node.reg1] + [self regType:node.reg2] * node.s + node.imm;
            return [self read64bits_dram_virtual:value];
        }
        default: NSAssert(NO, @"decode error type: %ld", node.odType);
    }
    return 0;
}

#pragma mark - memory

- (uint64_t)readRegType:(RegType)type {
    uint64_t reg = [self regType:type];
    return [self read64bits_dram_virtual:reg];
}

- (uint64_t)read64bits_dram:(uint64_t)paddr {
    return 0;
}

- (uint64_t)read64bits_dram_virtual:(uint64_t)paddr {
    return 0;
}

- (void)write64bits_dram:(uint64_t)paddr data:(uint64_t)data {
    
}

- (void)write64bits_dram_virtual:(uint64_t)paddr data:(uint64_t)data {
    
}

@end
