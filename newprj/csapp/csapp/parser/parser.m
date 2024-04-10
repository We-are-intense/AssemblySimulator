//
//  parser.m
//  csapp
//
//  Created by erfeixia on 2024/4/9.
//

#import "parser.h"
typedef enum OD_TYPE
{
    EMPTY,
    IMM, /* $0x123 立即数 */
    REG, /* rax 寄存器寻址 */
    MM_IMM, /* mov 0x123, %rax 绝对寻址 0x123 对应地址内容放到 %rax */
    MM_REG, /* mov (%rsi), %rax 间接寻址 */
    MM_IMM_REG, /* mov 0x12(%rsi), %rax M[Imm + REG] (基址 + 偏移量) 寻址 */
    MM_REG1_REG2, /* mov (%rsi, %rdi), %rax M[REG1 + REG2] 变址寻址 */
    MM_IMM_REG1_REG2, /* mov 0x12(%rsi, %rdi), %rax M[Imm + REG1 + REG2] 变址寻址 */
    MM_REG2_S, /* mov (, %rsi, s), %rax M[REG2 * s] 比例变址寻址 */
    MM_IMM_REG2_S, /* mov 0x12(, %rsi, s), %rax M[Imm + REG2 * s] 比例变址寻址 */
    MM_REG1_REG2_S, /* mov (%rsi, %rdi, s), %rax M[REG1 + REG2 * s] 比例变址寻址 */
    MM_IMM_REG1_REG2_S /* mov 0x12(%rsi, %rdi, s), %rax M[Imm + REG1 + REG2 * s] 比例变址寻址 */
} od_type_t;

@implementation parser

- (void)parserWithInst:(NSString *)inst {
    
}

- (void)parseNumber {
    
}

@end
