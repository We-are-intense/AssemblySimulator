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
@property (nonatomic, strong) NSMutableDictionary <NSString *, Express *>*labelDict;
@property (nonatomic, strong) NSMutableArray <Node *>*reLocationNodes;
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
        self.labelDict = [NSMutableDictionary dictionary];
        self.reLocationNodes = [NSMutableArray array];
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

- (Express *)parserWithInst:(NSString *)inst line:(NSInteger)line {
    [self resetBuffer];
    [inst getCharacters:buffer];
    Token *a = [self peekToken];
    while (a) {
        Token *b = [self peekNToken:1];
        /// 标签
        if (a.tokenType == TokenTypeString &&
            b.tokenType == TokenTypeColon) {
            Express *express =  [Express new];
            express.op = INST_LAB;
            express.line = line;
            express.labelString = a.token;
            self.labelDict[express.labelString] = express;
            [self nextToken];
            [self nextToken];
            return express;
        } else if (a.tokenType == TokenTypeString) {
            // 跳过 Inst
            Token *inst = [self nextToken];
            if (inst.tokenType != TokenTypeString) {
                NSAssert(NO, @"inst is null");
            }
            Express *express =  [Express new];
            express.op = [self.instMap[inst.token] integerValue];
            if (b.tokenType == TokenTypeEof) {
                return express;
            } else {
                express.src = [self parserNode];
                a = [self peekToken];
                if (a.tokenType == TokenTypeEof) {
                    return express;
                } else if (a.tokenType == TokenTypeComma) {
                    [self nextToken];// 跳过 ","
                    express.dst = [self parserNode];
                    return express;
                } else {
                    NSAssert(NO, @"parser node error");
                }
            }
        } else if (a.tokenType == TokenTypeEof) {
            break;
        } else {
            NSAssert(NO, @"parser error");
        }
    }
    return nil;
}

- (void)reLocation {
    
    
}
#pragma mark - parser token
- (Node *)parserNode {
    /*
     pushq %rbp
     mov (%rsi)             , %rax
     mov (%rsi, %rdi)       , %rax
     mov (    , %rsi, s)    , %rax
     mov (%rsi, %rdi, s)    , %rax
     
     mov $0x12              , %rax
     mov 0x12               , %rax
     mov 0x12(%rsi)         , %rax
     mov 0x12(%rsi, %rdi)   , %rax
     mov 0x12(, %rsi, s)    , %rax
     mov 0x12(%rsi, %rdi, s), %rax
     callq sub
     */
    
    Token *a = [self peekToken];
    if (a.tokenType == TokenTypeString) {
        /// callq sub
        [self nextToken];
        Node *node = [Node new];
        node.labelString = a.token;
        [self.reLocationNodes addObject:node];
        return node;
    } else if (a.tokenType == TokenTypePersent) {
        /// 1. % rbp (, | eof)
        ///    a  b     c
        Node *node = [Node new];
        node.reg1 = [self parserReg];
        /// 1. % rbp (, | eof)
        ///             a
        node.type = REG;
        [self isCommaOrEnd];
        return node;
    } else if (a.tokenType == TokenTypeDollar) {
        /// 2. $0x12
        [self nextToken]; // 跳过 $
        a = [self peekToken];
        if (a.tokenType != TokenTypeHex &&
            a.tokenType != TokenTypeDecimal) {
            NSAssert(NO, @"parse $0x12 failed can not find hex or decimal");
        }
        [self nextToken]; // 跳过 number
        Node *node = [Node new];
        node.imm = [self parseImm:a.token];
        node.type = IMM;
        [self isCommaOrEnd];
        return node;
    } else if (a.tokenType == TokenTypeDecimal ||
               a.tokenType == TokenTypeHex) {
        /// 3. 0x12 或者 0x12(...
        Node *node = [Node new];
        node.imm = [self parseImm:a.token];
        [self nextToken]; // 跳过 number
        [self parserLRPWithNode:node hasImm:YES];
        [self isCommaOrEnd];
        return node;
    } else {
        /// 4. (...
        Node *node = [Node new];
        [self parserLRPWithNode:node hasImm:YES];
        [self isCommaOrEnd];
        return node;
    }
    return nil;
}

- (void)parserLRPWithNode:(Node *)node
                   hasImm:(BOOL)hasImm {
    /*
     ( % rsi )
     ( ,  % rsi, s )
     ( % rsi , % rdi )
     ( % rsi , % rdi , s )
     */
    Token *a = [self peekToken];
    if (a.tokenType == TokenTypeLP) {
        [self nextToken];// 跳过 (
    }
    /*
     ( % rsi ) MM_REG
     ( ,  % rsi, s ) MM_REG2_S
     ( % rsi , % rdi ) MM_REG1_REG2
     ( % rsi , % rdi , s ) MM_REG1_REG2_S
       a  b  c  d e  f  g h i
     */
    a = [self peekToken];
    if (a.tokenType == TokenTypeComma) {
        [self nextToken];// 跳过 ,
        node.reg2 = [self parserReg];
        // ( ,  % rsi , s ) MM_REG2_S
        //            a b c d
        a        = [self peekToken];
        if (a.tokenType == TokenTypeComma) {
            [self nextToken];// 跳过 ,
        } else {
            NSAssert(NO, @"( ,%%rsi,s) parse second comma failed");
        }
        // ( ,  % rsi , s ) MM_REG2_S
        //              a b c d
        a        = [self peekToken];
        if (a.tokenType == TokenTypeHex ||
            a.tokenType == TokenTypeDecimal) {
            node.s = [a.token intValue];
            [self nextToken];
        } else {
            NSAssert(NO, @"(,%%rsi,s) parse s failed");
        }
        node.type = hasImm ? MM_IMM_REG2_S : MM_REG2_S;
        // ( ,  % rsi , s ) MM_REG2_S
        //                a b c d
        [self nextToken];// 跳过 )
        return;
    } else {
        node.reg1 = [self parserReg];
    }
    a = [self peekToken];
    if (a.tokenType == TokenTypeRP) {
        [self nextToken];// 跳过 )
        node.type = hasImm ? MM_IMM_REG : MM_REG;
        return;
    }
    a = [self peekToken];
    /*
     ( % rsi , % rdi ) MM_REG1_REG2
     ( % rsi , % rdi , s ) MM_REG1_REG2_S
             a  b  c  d e  f  g h i
     */
    if (a.tokenType == TokenTypeComma) {
        [self nextToken];// 跳过 ,
    } else {
        NSAssert(NO, @"( %%rsi,%%rdi) first comma failed");
    }
    /*
     ( % rsi , % rdi ) MM_REG1_REG2
     ( % rsi , % rdi , s ) MM_REG1_REG2_S
               a  b  c  d e  f  g h i
     */
    node.reg2 = [self parserReg];
    /*
     ( % rsi , % rdi ) MM_REG1_REG2
     ( % rsi , % rdi , s ) MM_REG1_REG2_S
                     a  b  c  d e  f  g h i
     */
    a = [self peekToken];
    if (a.tokenType == TokenTypeRP) {
        [self nextToken];// 跳过 )
        node.type = hasImm ? MM_IMM_REG1_REG2 : MM_REG1_REG2;
        return;
    }
    /*
     ( % rsi , % rdi ) MM_REG1_REG2
     ( % rsi , % rdi , s ) MM_REG1_REG2_S
                     a  b  c  d e  f  g h i
     */
    if (a.tokenType == TokenTypeComma) {
        [self nextToken];// 跳过 ,
    } else {
        NSAssert(NO, @"( %%rsi,%%rdi,s) second comma parse failed");
    }
    a = [self peekToken];
    node.s = [a.token intValue];
    [self nextToken];// 跳过 s
    a = [self peekToken];
    if (a.tokenType == TokenTypeRP) {
        [self nextToken];// 跳过 )
    } else {
        NSAssert(NO, @"( %%rsi,%%rdi,s) parse RP failed");
    }
    node.type = hasImm ? MM_IMM_REG1_REG2_S : MM_REG1_REG2_S;
}


- (RegType)parserReg {
    Token *a = [self peekToken];
    Token *b = [self peekNToken:1];
    
    if (a.tokenType != TokenTypePersent ||
        b.tokenType != TokenTypeString) {
        NSAssert(NO, @"parse reg failed");
    }
    [self nextToken];// 跳过 %
    [self nextToken];// 跳过 string
    RegType type = [self.regMap[b.token.lowercaseString] integerValue];
    NSAssert(type != RegType_none, @"REG，reg 不能为 None");
    return type;
}

- (BOOL)isCommaOrEnd {
    Token *a = [self peekToken];
    if (a.tokenType == TokenTypeComma ||
        a.tokenType == TokenTypeEof) {
        return YES;
    }
    NSAssert(NO, @"%%rbp (, | eof) can not find , or eof");
    return NO;
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
            token.tokenType = TokenTypeEof;
            return token;
        } else {
            [self nextChar];
            a = [self peekChar];
        }
    }
    Token *token = [Token new];
    token.token = @"\0";
    token.tokenType = TokenTypeEof;
    return token;
}

- (Token *)parseString {
    char a = [self peekChar];
    char buffer[64] = {'\0'};
    int offset = 0;
    while ((a >= 'a' && a <= 'z') ||
           (a >= 'A' && a <= 'Z')) {
        buffer[offset++] = a;
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
    [self nextChar];
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
    [self nextChar];
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
    [self nextChar];
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
    [self nextChar];
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
    [self nextChar];
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
    [self nextChar];
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
    [self.tokens removeAllObjects];
    [self.chars removeAllObjects];
    [self.labelDict removeAllObjects];
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
