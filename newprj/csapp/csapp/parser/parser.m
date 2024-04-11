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
/*
 
 
 
 
 
 */
typedef NS_ENUM(NSInteger, TokenType) {
    TokenTypeNone,
    ///< $0x123
    TokenTypeImm,
    ///< 0x123
    TokenTypeNum,
    ///< move
    TokenTypeInst,
    ///< rax
    TokenTypeReg,
    ///< (
    TokenTypeLeftB,
    ///< )
    TokenTypeRightB,
    ///< ,
    TokenTypeDot
};

#define maxBufferSize 64

@interface parser ()

@property (nonatomic, assign) int start;
@property (nonatomic, assign) od_type_t od_type;
@property (nonatomic, strong) NSMutableArray *tokens;
@end

@implementation parser {
    unichar buffer[maxBufferSize];
}

- (instancetype)init {
    self = [super init];
    if (self) {
        self.tokens = [NSMutableArray array];
    }
    return self;
}


- (id)parserWithInst:(NSString *)inst {
    if (inst.length == 0) {
        return nil;
    }
    [self resetBuffer];
    [self.tokens removeAllObjects];
    
    [inst getCharacters:buffer];
    char a = self.pre;
    NSString *token = nil;
    TokenType type = TokenTypeNone;
    while (a != '\0') {
        if (a >= 'a' && a <= 'z') {
            token = [self parseInst];
            type = TokenTypeInst;
        } else if (a >= '0' && a <= '9') {
            token = [self parseNumber];
            type = TokenTypeNum;
        } else if (a == '%') {
            token = [self parseRegister];
            type = TokenTypeReg;
        } else if (a == '(') {
            token = @"(";
            type = TokenTypeLeftB;
            [self next];
        } else if (a == ')') {
            token = @")";
            type = TokenTypeRightB;
            [self next];
        } else if (a == ',') {
            token = @",";
            type = TokenTypeDot;
            [self next];
        } else if (a == '$') {
            token = [self parseNumber];
            type = TokenTypeImm;
        } else if (a == ' ') {
            [self next];
        }
        a = self.pre;
        if (token && type != TokenTypeNone) {
            [self addToken:token tokenType:type];
        }
        type = TokenTypeNone;
        token = nil;
    }
    
    NSString *str = [self.tokens componentsJoinedByString:@"  "];
    printf("%s\n\n", str.UTF8String);
    return nil;
}
#pragma mark - Private Methods

- (char)next {
    if(self.start >= maxBufferSize || 
       buffer[self.start] == '\0') {
        return '\0';
    }
    return buffer[self.start++];
}

- (char)pre {
    if((self.start) >= maxBufferSize ||
       buffer[self.start] == '\0') {
        return '\0';
    }
    return buffer[self.start];
}

- (NSString *)parseInst {
    char a = self.pre;
    char inst[16] = {'\0'};
    int offset = 0;
    while (a != ' ' && a != '\0') {
        inst[offset++] = self.next;
        a = self.pre;
    }
    
    if (offset == 0) {
        NSAssert(NO, @"指令解析出错");
    }
    return [NSString stringWithFormat:@"%s", inst];
}

- (NSString *)parseNumber {
    char a = self.pre;
    char inst[64] = {'\0'};
    int offset = 0;
    while (a != ' ' && a != '\0' &&
           ((a >= '0' && a <= '9') || a == 'x' || a == 'X')) {
        inst[offset++] = self.next;
        a = self.pre;
    }
    if (offset == 0) {
        NSAssert(NO, @"数字解析出错");
    }
    return [NSString stringWithFormat:@"%s", inst];
}

- (NSString *)parseRegister {
    char a = self.pre;
    if (a != '%') {
        NSAssert(NO, @"寄存器解析出错，第一个字符不等于 '%%'");
    }
    // 跳过 %
    [self next];
    a = self.pre;
    char inst[16] = {'\0'};
    int offset = 0;
    while (a != ' ' && a != '\0' &&
           ((a >= 'a' && a <= 'z') ||
            (a >= 'A' && a >= 'Z'))) {
        inst[offset++] = self.next;
        a = self.pre;
    }
    if (offset == 0) {
        NSAssert(NO, @"寄存器解析出错");
    }
    return [NSString stringWithFormat:@"%s", inst];
}

- (void)addToken:(NSString *)token tokenType:(TokenType)tokenType {
    [self.tokens addObject:token];
}

- (void)resetBuffer {
    for (int i = 0; i < maxBufferSize; i++) {
        buffer[i] = '\0';
    }
    self.start = 0;
}

@end
