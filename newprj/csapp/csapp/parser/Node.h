//
//  Node.h
//  csapp
//
//  Created by erfeixia on 2024/4/12.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, OdType) {
    EMPTY,
    INST, /* 指令 mov*/
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
};

typedef NS_ENUM(NSInteger, RegType) {
    RegType_none,
    RegType_rax,
    RegType_rsi,
    RegType_rdi,
};

@interface Node : NSObject
@property (nonatomic, assign) OdType odType;
@property (nonatomic, assign) NSInteger imm;
@property (nonatomic, assign) NSInteger s;
@property (nonatomic, assign) RegType reg1;
@property (nonatomic, assign) RegType reg2;

- (void)reg:(NSString *)reg;

@end

NS_ASSUME_NONNULL_END
