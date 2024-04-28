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
    union {
        uint64_t __cpu_flag_value;
        struct {
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
    core_t core;
}
@property (nonatomic, strong, readwrite) NSArray <Express *>* expresses;
@end

@implementation VM
- (instancetype)initWithExpresses:(NSArray <Express *>*)expresses {
    self = [super init];
    if (self) {
        self.expresses = expresses;
        [self resetMemory];
    }
    return self;
}

- (void)run {
    BOOL isRun = YES;
    while (isRun) {
        Express *express = self.expresses[core.rip];
        switch (express.op) {
            case INST_MOV:
            {
                uint64_t src = [self decodeNode:express.src];
                uint64_t dst = [self decodeNode:express.dst];
                if (express.src.type == IMM && express.dst.type == REG) {
                    [self regType:express.dst.reg1 value:src];
                } else if (express.src.type == REG && express.dst.type == REG) {
                    [self regType:express.dst.reg1 value:src];
                } else if (express.src.type == REG && express.dst.type >= MM_IMM) {
                    [self write64bits_dram_virtual:dst data:src];
                } else if (express.src.type >= MM_IMM && express.dst.type == REG) {
                    uint64_t value = [self read64bits_dram_virtual:src];
                    [self regType:express.dst.reg1 value:value];
                } else {
                    NSAssert(NO, @"mov 指令运算异常");
                }
                [self increasePC];
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_PUSH:
            {
                if (express.src.type != REG) {
                    NSAssert(NO, @"push 指令后必须是寄存器");
                }
                uint64_t src = [self decodeNode:express.src];
                
                // subq $8, %rsp     # %rsp    = %rsp - 8
                // movq %rbp, (%rsp) # *(%rsp) = %rbp
                core.reg.rsp -= 8;
                [self write64bits_dram_virtual:core.reg.rsp data:src];
                [self increasePC];
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_POP:
            {
                if (express.src.type != REG) {
                    NSAssert(NO, @"pop 指令后必须是寄存器");
                }
                // movq (%rsp), %rax
                // addq $8, %rsp
                uint64_t value = [self read64bits_dram_virtual:core.reg.rsp];
                [self regType:express.src.reg1 value:value];
                core.reg.rsp += 8;
                [self increasePC];
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_LEAVE:
            {
                // movq %rbp, %rsp
                core.reg.rsp = core.reg.rbp;
                // popq %rbp
                core.reg.rbp = [self read64bits_dram_virtual:core.reg.rsp];
                core.reg.rsp += 8;
                [self increasePC];
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_CALL:
            {
                uint64_t src = [self decodeNode:express.src];
                // 400540 <top>:
                //      400540: sub xxx, xxx
                //      ......
                //      400551: retq
                //
                //      ......
                //      40055b: callq 400540<top>
                //      400560: xxx
                
                // callq:
                //  push %rsp # %rsp --> 400560
                //  1. 将下一条指令的地址存放在 %rsp 中
                //  2. 跳转到函数 top 执行
                // retq:
                //  2. pop %rip # %rip --> 400560
                // 下一条指令的地址
                uint64_t next = core.rip + 1;
                // push %rsp
                core.reg.rsp -= 8;
                [self write64bits_dram_virtual:core.reg.rsp data:next];
                // 跳转到 src 执行代码
                core.rip = src;
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_RET:
            {
                // retq:
                //  pop %rip # %rip --> 400560
                core.rip = [self read64bits_dram_virtual:core.reg.rsp];
                core.reg.rsp += 8;
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_ADD:
            {
                uint64_t src = [self decodeNode:express.src];
                uint64_t dst = [self decodeNode:express.dst];
                if (express.src.type == REG && express.dst.type == REG) {
                    uint64_t val = src + dst;
                    // 在进行无符号数运算时，它记录了运算结果的最高有效位向更高位的进位值，或从更高位的借位值。
                    // 最高位 正数: 0 负数: 1
                    int val_sign = ((val >> 63) & 0x1);
                    int src_sign = (src >> 63) & 0x1;
                    int dst_sign = (dst >> 63) & 0x1;
                    // 两无符号数相加，结果变小 无符号溢出
                    core.flags.CF = (val < src);
                    // 记录相关指令执行后，其结果是否为0，如果结果为0，那么ZF标志位为1。
                    // 结果等于零
                    core.flags.ZF = (val == 0);
                    // SF 记录相关指令执行后，其结果是否为负数（最高位为1表示负数），
                    // 如果为负数，那么SF为1，如果不为负数，那么SF为0。
                    // 正数: 0 负数: 1
                    core.flags.SF = val_sign;
                    // 两个正数相加结果为负数，两个负数相加结果为正数
                    core.flags.OF = (src_sign == 0 && dst_sign == 0 && val_sign == 1) ||
                                    (src_sign == 1 && dst_sign == 1 && val_sign == 0);
                    [self regType:express.dst.reg1 value:val];
                } else {
                    NSAssert(NO, @"add 指令异常");
                }
                [self increasePC];
                break;
            }
            case INST_SUB:
            {
                uint64_t src = [self decodeNode:express.src];
                uint64_t dst = [self decodeNode:express.dst];
                if (express.src.type >= IMM && express.dst.type == REG) {
                    // src: imm
                    // dst: register (value: int64_t bit map)
                    // dst = dst - src = dst + (-src)
                    uint64_t val = dst + (~src + 1);
                    // 最高位 正数: 0 负数: 1
                    int val_sign = ((val >> 63) & 0x1);
                    int src_sign = (src >> 63) & 0x1;
                    int dst_sign = (dst >> 63) & 0x1;
                    
                    // 两个数相减 val = dst - src, val > dst
                    core.flags.CF = (val > dst);
                    // 结果等于零
                    core.flags.ZF = (val == 0);
                    // 正数: 0 负数: 1
                    core.flags.SF = val_sign;
                    // 1. 正数减去负数，结果却是负数
                    // 2. 负数减去正数，结果确是正数
                    core.flags.OF = (src_sign == 1 && dst_sign == 0 && val_sign == 1) ||
                                    (src_sign == 0 && dst_sign == 1 && val_sign == 0);
                    [self regType:express.dst.reg1 value:val];
                } else {
                    NSAssert(NO, @"sub 指令异常");
                }
                [self increasePC];
                break;
            }
            case INST_CMP:
            {
                uint64_t dval = 0, val = 0;
                if (express.src.type == IMM && express.dst.type >= MM_IMM) {
                    uint64_t dst = [self decodeNode:express.dst];
                    dval = [self read64bits_dram_virtual:dst];
                } else if (express.src.type == IMM && express.dst.type == REG) {
                    dval = [self regType:express.dst.reg1];
                }
                uint64_t src = [self decodeNode:express.src];
                val = dval + (~(src) + 1);

                int val_sign = ((val >> 63) & 0x1);
                int src_sign = (((src) >> 63) & 0x1);
                int dst_sign = ((dval >> 63) & 0x1);
                // 由于 AX 的值不等于 BX 的值，因此条件码寄存器中的零标志位被设置为 0。
                // set condition flags
                core.flags.CF = (val > dval); // unsigned

                core.flags.ZF = (val == 0);
                core.flags.SF = val_sign;

                core.flags.OF = (src_sign == 1 && dst_sign == 0 && val_sign == 1) || 
                                (src_sign == 0 && dst_sign == 1 && val_sign == 0);

                 // signed and unsigned value follow the same addition. e.g.
                 // 5 = 0000000000000101, 3 = 0000000000000011, -3 = 1111111111111101, 5 + (-3) = 0000000000000010
                [self increasePC];
                break;
            }
            case INST_JNE:
            {
                if (core.flags.ZF == 0) {
                    // ZF == 0 表示两个数不相等，则跳转
                    core.rip = [self decodeNode:express.src];
                } else {
                    [self increasePC];
                }
                core.flags.__cpu_flag_value = 0;
                break;
            }
            case INST_JMP:
            {
                core.rip = [self decodeNode:express.src];
                core.flags.__cpu_flag_value = 0;
                break;
            }
            default: 
            {
                NSAssert(NO, @"未知指令" );
                break;
            }
        }
    }
}

- (uint64_t)regType:(RegType)type {
    switch (type) {
        case RegType_rax: return core.reg.rax;
        case RegType_rsi: return core.reg.rsi;
        case RegType_rdi: return core.reg.rdi;
        default: break;
    }
    NSAssert(NO, @"not find RegType: %ld", type);
    return 0;
}

- (void)regType:(RegType)type value:(uint64_t)value {
    switch (type) {
        case RegType_rax: core.reg.rax = value; break;
        case RegType_rsi: core.reg.rsi = value; break;
        case RegType_rdi: core.reg.rdi = value; break;
        default: NSAssert(NO, @"set value not find RegType: %ld", type); break;
    }
}

- (uint64_t)decodeNode:(Node *)node {
    switch (node.type) {
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
        default: NSAssert(NO, @"decode error type: %ld", node.type);
    }
    return 0;
}

- (void)increasePC {
    core.rip += 1;
}
#pragma mark - memory

- (uint64_t)readRegType:(RegType)type {
    uint64_t reg = [self regType:type];
    return [self read64bits_dram_virtual:reg];
}

- (uint64_t)read64bits_dram_virtual:(uint64_t)vaddr {
    return [self read64bits_dram:[self va2pa:vaddr]];
}

- (uint64_t)read64bits_dram:(uint64_t)paddr {
    uint64_t val = 0x0;

    val += (((uint64_t)memory[paddr + 0 ]) << 0);
    val += (((uint64_t)memory[paddr + 1 ]) << 8);
    val += (((uint64_t)memory[paddr + 2 ]) << 16);
    val += (((uint64_t)memory[paddr + 3 ]) << 24);
    val += (((uint64_t)memory[paddr + 4 ]) << 32);
    val += (((uint64_t)memory[paddr + 5 ]) << 40);
    val += (((uint64_t)memory[paddr + 6 ]) << 48);
    val += (((uint64_t)memory[paddr + 7 ]) << 56);
    return val;
}

- (void)write64bits_dram_virtual:(uint64_t)vaddr data:(uint64_t)data {
    [self write64bits_dram:[self va2pa:vaddr] data:data];
}

- (void)write64bits_dram:(uint64_t)paddr data:(uint64_t)data {
    memory[paddr + 0] = (data >> 0 ) & 0xff;
    memory[paddr + 1] = (data >> 8 ) & 0xff;
    memory[paddr + 2] = (data >> 16) & 0xff;
    memory[paddr + 3] = (data >> 24) & 0xff;
    memory[paddr + 4] = (data >> 32) & 0xff;
    memory[paddr + 5] = (data >> 40) & 0xff;
    memory[paddr + 6] = (data >> 48) & 0xff;
    memory[paddr + 7] = (data >> 56) & 0xff;
}

- (uint64_t)va2pa:(uint64_t)vaddr {
    return vaddr % MM_LEN;
}

- (void)resetMemory {
    for (int i = 0; i < MM_LEN; i++) {
        memory[i] = '\0';
    }
}
@end
