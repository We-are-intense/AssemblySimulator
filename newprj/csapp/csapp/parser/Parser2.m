//
//  Parser2.m
//  csapp
//
//  Created by erfeixia on 2024/5/8.
//

/*
 label = character:
 statement =  inst none
            | inst express none
            | inst express, express none

 express =    imm
            | num
            | reg
            | mm_reg
            | mm_imm_reg
            | mm_reg1_reg2
            | mm_imm_reg1_reg2
            | mm_reg2_s
            | mm_imm_reg2_s
            | mm_reg1_reg2_s
            | mm_imm_reg1_reg2_s
 
 inst = character
 mm_imm_reg1_reg2_s = num mm_reg1_reg2_s
 mm_reg1_reg2_s = (reg, reg, s)
 mm_imm_reg2_s = num mm_reg2_s
 mm_reg2_s = (, reg, s)
 mm_imm_reg1_reg2 = num mm_reg1_reg2
 mm_reg1_reg2 = (reg, reg)
 mm_imm_reg = num mm_reg
 mm_reg = (reg)
 imm = $num
 reg = %character
 
 s = 1 | 2 | 4 | 8
 num = -? (decimal | hex)
 decimal = (0-9)*
 hex = 0x(0-1 | a-f | A-F)*
 character = (a-z | A-Z)*
 none = \n
 */
#import "Parser2.h"
#import "Token.h"

#define maxBufferSize 64


@interface Parser2 ()
@property (nonatomic, assign) int start;
@property (nonatomic, strong) NSMutableArray <Token *>*tokens;
@property (nonatomic, strong) NSMutableArray <NSString *>*chars;
@property (nonatomic, strong) NSDictionary *instMap;
@property (nonatomic, strong) NSDictionary *regMap;
@end

@implementation Parser2 {
    unichar buffer[maxBufferSize];
    unichar tokenChar[maxBufferSize];
}

- (instancetype)init {
    self = [super init];
    if (self) {
        self.tokens = [NSMutableArray array];
        self.chars = [NSMutableArray array];
        self.instMap = @{
            @"movq" : @(INST_MOV),
            @"movl" : @(INST_MOV),
            @"pushq" : @(INST_PUSH),
            @"popq" : @(INST_POP),
            @"leaveq" : @(INST_LEAVE),
            @"callq" : @(INST_CALL),
            @"retq" : @(INST_RET),
            @"addl" : @(INST_ADD),
            @"subl" : @(INST_SUB),
            @"cmpq" : @(INST_CMP),
            @"jneq" : @(INST_JNE),
            @"jmpq" : @(INST_JMP),
            @"xorl" : @(INST_XOR),
        };
        self.regMap = @{
            @"rax" : @(RegType_rax),
            @"rsi" : @(RegType_rsi),
            @"rdi" : @(RegType_rdi),
            @"rbp" : @(RegType_rbp),
            @"rsp" : @(RegType_rsp),
            @"edi" : @(RegType_edi),
            @"esi" : @(RegType_esi),
            @"eax" : @(RegType_eax),
        };
    }
    return self;
}

- (Express *)parserWithInst:(NSString *)inst {
    [self resetBuffer];
    [self.tokens removeAllObjects];
    [self.chars removeAllObjects];
    
    Token *a = [self peekToken];
    while (a) {
        Token *b = [self peekNToken:1];
        
        /// 标签
        if (a.tokenType == TokenTypeString &&
            b.tokenType == TokenTypeColon) {
            
            return nil;
        }
        
        if (a.tokenType == TokenTypeString) {
            // 跳过 Inst
            Token *inst = [self nextToken];
            if (inst.tokenType != TokenTypeString) {
                NSAssert(NO, @"inst is null");
            }
            Express *express =  [Express new];
            express.op = [self.instMap[inst.token] integerValue];
            if (b.tokenType == TokenTypeNone) {
                return express;
            } else {
                express.src = [self parserNode];
                a = [self peekToken];
                if (a.tokenType == TokenTypeNone) {
                    return express;
                } else if (a.tokenType == TokenTypeComma) {
                    [self nextToken];// 跳过 ","
                    express.dst = [self parserNode];
                    return express;
                } else {
                    NSAssert(NO, @"parser node error");
                }
            }
        }
    }
    return nil;
}
#pragma mark - parser token
- (Node *)parserNode {
    /// 1. imm = $num
    Token *a = [self peekToken];
    if (a.tokenType == TokenTypeDollar) {
        Token *b = [self peekNToken:1];
        if (b.tokenType == TokenTypeDecimal ||
            b.tokenType == TokenTypeHex) {
            [self nextToken]; /// 跳过 $
            [self nextToken]; /// 跳过 num
            Node *node = [Node new];
            node.type = IMM;
            node.imm = [self parseImm:b.token];
            return node;
        } else {
            NSAssert(NO, @"parser imm failed");
        }
    }
    
    if (a.tokenType == TokenTypeDecimal ||
        a.tokenType == TokenTypeHex) {
        Node *node = [Node new];
        node.imm = [self parseImm:a.token];
        [self nextToken];/// 跳过 0x123
        Token *b = [self peekToken];
        if (b.tokenType == TokenTypeComma ||
            b.tokenType == TokenTypeNone) {
            /// 0x123
            if (b.tokenType == TokenTypeComma) {
                [self nextToken];/// 跳过 ,
            }
            node.type = MM_IMM;
            return node;
        } else if (b.tokenType == TokenTypeLP) {
            [self nextToken];/// 跳过 (
            b = [self peekToken];
            if (b.tokenType == TokenTypePersent) {
                // -12(%rbp)
                node.reg1 = [self parserReg];
                b = [self peekToken];
                if (b.tokenType == TokenTypeRP) {
                    [self nextToken];
                    if ([self peekToken].tokenType == TokenTypeComma) {
                        [self nextToken];/// 跳过 ,
                    }
                    return node;
                } else if (b.tokenType == TokenTypeComma) {
                    //  0x12(%rsi , %rdi)
                    //  0x12(%rsi , %rdi, s)
                    //            |
                    [self nextToken];/// 跳过 ,
                    //  0x12(%rsi , %rdi)
                    //  0x12(%rsi , %rdi, s)
                    //              |
                    b = [self peekToken];
                    if (b.tokenType == TokenTypePersent) {
                        node.reg2 = [self parserReg];
                        b = [self peekToken];
                        if (b.tokenType == TokenTypeComma) {
                            //  0x12(%rsi , %rdi , s)
                            //                   |
                            [self nextToken];/// 跳过 ,
                            b = [self peekToken];
                            if (b.tokenType == TokenTypeDecimal ||
                                b.tokenType == TokenTypeHex) {
                                //  0x12(%rsi , %rdi , s)
                                //                     |
                                node.s = [self parseImm:b.token];
                                [self nextToken];
                                b = [self peekToken];
                                if (b.tokenType == TokenTypeRP) {
                                    //  0x12(%rsi , %rdi , s )
                                    //                       |
                                    [self nextToken]; // 跳过 )
                                    return node;
                                } else {
                                    
                                }
                                
                            }
                        } else  {
                            //  0x12(%rsi , %rdi )
                            //                   |
                        }
                        
                        
                    } else {
                        NSAssert(NO, @"parser node reg2 failed");
                    }
                }
                
                
                
                
                
            } else if (b.tokenType == TokenTypeComma) {
                /// movl 0x12(, %rsi, s), %rax
                
            } else {
                NSAssert(NO, @"parser node failed can not match");
            }
        }
    }
    return nil;
}

- (RegType)parserReg {
    
    return RegType_none;
}

#pragma mark - token
- (Token *)nextToken {
    if (self.tokens.count != 0) {
        Token *token = self.tokens.firstObject;
        [self.tokens removeObject:token];
        return token;
    }
    return [self getAToken];
}

- (Token *)peekToken {
    if (self.tokens.count != 0) {
        Token *token = self.tokens.firstObject;
        return token;
    }
    return [self peekNToken:0];
}

- (Token *)peekNToken:(int)n {
    if (n < 0) return nil;
    
    Token *token = nil;
    if (n < self.tokens.count) token = self.tokens[n];
    
    while (token == nil) {
        token = [self getAToken];
        if (token == nil) return nil;
        [self.tokens addObject:token];
        if (n < self.tokens.count) token = self.tokens[n];
    }
    return token;
}

- (Token *)getAToken {
    char a = [self peekChar];
    while (a != '\0') {
        if (((a >= 'a' && a <= 'z') ||
             (a >= 'A' && a <= 'Z'))) {
            return [self parseString];
        } else if (a == '-' || (a >= '0' && a <= '9')) {
            if (a == '-') {
                // - 0 x 1 2 3
                // 0 1 2 3
                char b = [self peekNChar:1];
                char c = [self peekNChar:2];
                if (b == '0' && (c == 'x' || c == 'X')) {
                    return [self parseHex];
                } else {
                    return [self parseDecimal];
                }
            } else {
                // 0 x 1 2 3
                // 0 1 2 3
                char b = [self peekNChar:1];
                if (a == '0' && (b == 'x' || b == 'X')) {
                    return [self parseHex];
                } else {
                    return [self parseDecimal];
                }
            }
        } else if (a == '$') {
            return [self parseDollar];
        } else if (a == ':') {
            return [self parseColon];
        } else if (a == '(') {
            return [self parseLP];
        } else if (a == ')') {
            return [self parseRP];
        } else if (a == ',') {
            return [self parseComma];
        } else if (a == '%') {
            return [self parsePersent];
        } else if (a == '\n') {
            Token *token = [Token new];
            token.token = @"\n";
            token.tokenType = TokenTypeNone;
            return token;
        } else {
            [self nextChar];
            a = [self peekChar];
        }
    }
    return nil;
}

- (Token *)parseString {
    char a = [self peekChar];
    char buffer[64] = {'\0'};
    int offset = 0;
    while ((a >= 'a' && a <= 'z') ||
           (a >= 'A' && a <= 'Z')) {
        buffer[offset] = a;
        [self nextChar];
        a = [self peekChar];
    }
    if (buffer[0] == '\0') {
        NSAssert(NO, @"parse string failed");
    }
    Token *token = [Token new];
    token.token = [NSString stringWithFormat:@"%s", buffer];
    token.tokenType = TokenTypeString;
    return token;
}

- (Token *)parseHex {
    char a = [self peekChar];
    BOOL positive = YES;// 正数
    if (a == '-') {
        [self nextChar];
        a = [self peekChar];
    }
    
    char b = [self peekNChar:1];
    if (a == '0' && (b == 'x' || b == 'X')) {
        [self nextChar];
        [self nextChar];
        char buffer[64] = {'\n'};
        a = [self peekChar];
        int index = 0;
        while ((a >= '0' && a <= '9') ||
               (a >= 'a' && a <= 'f') ||
               (a >= 'A' && a <= 'F')) {
            buffer[index++] = a;
            [self nextChar];
            a = [self peekChar];
        }
        if (buffer[0] == '\0') {
            NSAssert(NO, @"parse hex failed");
        }
        Token *token = [Token new];
        token.token = [NSString stringWithFormat:@"%@%s", positive ? @"" : @"-", buffer];
        token.tokenType = TokenTypeHex;
        return token;
    } else {
        NSAssert(NO, @"parse hex failed");
    }
    return nil;
}

- (Token *)parseDecimal {
    char a = [self peekChar];
    BOOL positive = YES;// 正数
    if (a == '-') {
        [self nextChar];
        a = [self peekChar];
    }
    
    char buffer[64] = {'\n'};
    int index = 0;
    while (a >= '0' && a <= '9') {
        buffer[index++] = a;
        [self nextChar];
        a = [self peekChar];
    }
    if (buffer[0] == '\0') {
        NSAssert(NO, @"parse hex failed");
    }
    Token *token = [Token new];
    token.token = [NSString stringWithFormat:@"%@%s", positive ? @"" : @"-", buffer];
    token.tokenType = TokenTypeDecimal;
    return token;
}

- (Token *)parseDollar {
    char a = [self peekChar];
    if (a != '$') {
        NSAssert(NO, @"parse dollar failed");
    }
    [self nextToken];
    Token *token = [Token new];
    token.token = @"$";
    token.tokenType = TokenTypeDollar;
    return token;
}

- (Token *)parseColon {
    char a = [self peekChar];
    if (a != ':') {
        NSAssert(NO, @"parse colon failed");
    }
    [self nextToken];
    Token *token = [Token new];
    token.token = @":";
    token.tokenType = TokenTypeColon;
    return token;
}
- (Token *)parseLP {
    char a = [self peekChar];
    if (a != '(') {
        NSAssert(NO, @"parse LP failed");
    }
    [self nextToken];
    Token *token = [Token new];
    token.token = @"(";
    token.tokenType = TokenTypeLP;
    return token;
}
- (Token *)parseRP {
    char a = [self peekChar];
    if (a != ')') {
        NSAssert(NO, @"parse RP failed");
    }
    [self nextToken];
    Token *token = [Token new];
    token.token = @")";
    token.tokenType = TokenTypeRP;
    return token;
}
- (Token *)parseComma {
    char a = [self peekChar];
    if (a != ',') {
        NSAssert(NO, @"parse RP failed");
    }
    [self nextToken];
    Token *token = [Token new];
    token.token = @",";
    token.tokenType = TokenTypeComma;
    return token;
}
- (Token *)parsePersent {
    char a = [self peekChar];
    if (a != '%') {
        NSAssert(NO, @"parse RP failed");
    }
    [self nextToken];
    Token *token = [Token new];
    token.token = @"%";
    token.tokenType = TokenTypePersent;
    return token;
}

#pragma mark - char
- (char)nextChar {
    if (self.chars.count != 0) {
        char a = [self charsAtIndex:0];
        [self.chars removeObjectAtIndex:0];
        return a;
    }
    return [self getAChar];
}

- (char)peekChar {
    if (self.chars.count != 0) {
        char a = [self charsAtIndex:0];
        return a;
    }
    return [self peekNChar:0];
}

- (char)peekNChar:(int)n {
    if (n < 0) return '\0';
    
    char a = '\0';
    if (n < self.chars.count) return [self charsAtIndex:n];
    while (a == '\0') {
        a = [self getAChar];
        if (a == '\0') return '\0';
        [self.chars addObject:[NSString stringWithFormat:@"%c", a]];
        if (n < self.chars.count) a = [self charsAtIndex:n];
    }
    return a;
}

- (char)getAChar {
    if(self.start >= maxBufferSize ||
       buffer[self.start] == '\0') {
        return '\0';
    }
    return buffer[self.start++];
}


- (char)charsAtIndex:(int)index {
    if (index < 0 || index >= self.chars.count)
        return '\0';
    
    NSString *str = self.chars[index];
    unichar a[1] = {'\0'};
    [str getCharacters:a];
    return a[0];
}
#pragma mark - Private Methods
- (void)resetBuffer {
    for (int i = 0; i < maxBufferSize; i++) {
        buffer[i] = '\0';
        tokenChar[i] = '\0';
    }
    self.start = 0;
}
- (NSInteger)parseImm:(NSString *)imm {
    NSAssert(imm, @"Imm 字符串 不能为空");
    if ([imm hasPrefix:@"0x"] || [imm hasPrefix:@"0X"]) {
        NSAssert(imm.length > 2, @"解析 16字符串 to num error only 0x ");
        imm = [imm substringFromIndex:2];
        const char *char_str = [imm cStringUsingEncoding:NSASCIIStringEncoding];
        NSInteger hexNum;
        sscanf(char_str, "%lx", &hexNum);
        return hexNum;
    } else {
        return [imm integerValue];
    }
}
@end
