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

#define maxBufferSize 64

@interface parser ()

@property (nonatomic, assign) int start;
@property (nonatomic, assign) od_type_t od_type;
@property (nonatomic, strong) NSMutableArray *tokens;
@end

@implementation parser {
    unichar buffer[maxBufferSize];
}

- (instancetype)init
{
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
    while (buffer[self.start] != '\0' && self.start < maxBufferSize) {
        char a = buffer[self.start];
        if (a >= 'a' && a <= 'z') {
            [self parseInst];
        } else if (a >= '0' && a <= '9') {
            [self parseNumber];
        } else if (a == '%') {
            [self parseRegister];
        } else if (a == '(') {
            [self.tokens addObject:@"("];
            self.start++;
        } else if (a == ',') {
            [self.tokens addObject:@","];
            self.start++;
        } else if (a == ' ') {
            self.start++;
        }
    }
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
    if((self.start + 1) >= maxBufferSize ||
       buffer[self.start + 1] == '\0') {
        return '\0';
    }
    return buffer[self.start + 1];
}

- (void)parseInst {
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
    [self.tokens addObject:[NSString stringWithFormat:@"%s", inst]];
}

- (void)parseNumber {
    char a = self.pre;
    char inst[64] = {'\0'};
    int offset = 0;
    while (a != ' ' && a != '\0' &&
           (a >= '0' || a <= '9' || a == 'x' || a == 'X')) {
        inst[offset++] = self.next;
        a = self.pre;
    }
    if (offset == 0) {
        NSAssert(NO, @"数字解析出错");
    }
    [self.tokens addObject:[NSString stringWithFormat:@"%s", inst]];
}

- (void)parseRegister {
    char a = self.pre;
    if (a != '%') {
        NSAssert(NO, @"寄存器解析出错，第一个字符不等于 '%%'");
    }
    // 跳过 %
    [self next];
    char inst[16] = {'\0'};
    int offset = 0;
    while (a != ' ' && a != '\0' &&
           (a >= 'a' || a <= 'z' || a >= 'A' || a >= 'Z')) {
        inst[offset++] = self.next;
        a = self.pre;
    }
    if (offset == 0) {
        NSAssert(NO, @"寄存器解析出错");
    }
    [self.tokens addObject:[NSString stringWithFormat:@"%s", inst]];
}

- (void)resetBuffer {
    for (int i = 0; i < maxBufferSize; i++) {
        buffer[i] = '\0';
    }
}

@end
