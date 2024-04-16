//
//  Parser.m
//  csapp
//
//  Created by erfeixia on 2024/4/9.
//

#import "Parser.h"
#import "Node.h"

typedef NS_ENUM(NSInteger, TokenType) {
    TokenTypeNone,
    TokenTypeImm, ///< $0x123
    TokenTypeNum, ///< 0x123
    TokenTypeInst, ///< move
    TokenTypeReg, ///< rax
    TokenTypeLeftB, ///< (
    TokenTypeRightB, ///< )
    TokenTypeDot, ///< ,
    TokenTypeEOF ///< end
};

typedef NS_ENUM(NSInteger, StateType) {
    StateTypeNone,
    StateTypeInst,
    StateTypeStart,
    StateType_0, ///< rax
    StateType_1, ///< $0x123
    StateType_2, ///< 0x123, 后续可能为0x123(...
    StateType_3, ///< 1: (xxx 2: 0x123(...
    StateType_4, ///< 1: (%rax... 2: 0x123(%rax...
    StateType_5, ///< 1: (, ... 2: 0x123(%rax, ...
    StateType_6, ///<
    StateType_7, ///<
    StateType_8, ///<
    StateType_9, ///<
    StateTypeEnd
};

#define maxBufferSize 64

@interface Parser ()

@property (nonatomic, assign) int start;
@property (nonatomic, assign) OdType od_type;
@property (nonatomic, strong) NSMutableArray *tokens;

@property (nonatomic, assign) StateType stateType;
@property (nonatomic, strong) NSMutableArray *stateTypes;
@property (nonatomic, strong) NSArray *stateTransactions;
@property (nonatomic, strong) NSArray *parseStates;
@property (nonatomic, strong) NSDictionary *instMap;
@property (nonatomic, strong) NSDictionary *regMap;
@end

@implementation Parser {
    unichar buffer[maxBufferSize];
}

- (instancetype)init {
    self = [super init];
    if (self) {
        self.tokens = [NSMutableArray array];
        self.stateTypes = [NSMutableArray array];
        self.stateTransactions = @[
            @[@(StateTypeInst), @(TokenTypeInst), @(StateTypeStart)],
            @[@(StateTypeStart), @(TokenTypeReg), @(StateType_0)],
            @[@(StateTypeStart), @(TokenTypeImm), @(StateType_1)],
            @[@(StateTypeStart), @(TokenTypeNum), @(StateType_2)],
            @[@(StateTypeStart), @(TokenTypeLeftB), @(StateType_3)],
            @[@(StateType_0), @(TokenTypeDot), @(StateTypeStart)],
            @[@(StateType_0), @(TokenTypeEOF), @(StateTypeEnd)],
            @[@(StateType_1), @(TokenTypeDot), @(StateTypeStart)],
            @[@(StateType_1), @(TokenTypeEOF), @(StateTypeEnd)],
            @[@(StateType_2), @(TokenTypeDot), @(StateTypeStart)],
            @[@(StateType_2), @(TokenTypeEOF), @(StateTypeEnd)],
            @[@(StateType_2), @(TokenTypeLeftB), @(StateType_3)],
            @[@(StateType_3), @(TokenTypeReg), @(StateType_4)],
            @[@(StateType_3), @(TokenTypeDot), @(StateType_5)],
            @[@(StateType_4), @(TokenTypeDot), @(StateType_5)],
            @[@(StateType_4), @(TokenTypeRightB), @(StateType_9)],
            @[@(StateType_5), @(TokenTypeReg), @(StateType_6)],
            @[@(StateType_6), @(TokenTypeDot), @(StateType_7)],
            @[@(StateType_6), @(TokenTypeRightB), @(StateType_9)],
            @[@(StateType_7), @(TokenTypeNum), @(StateType_8)],
            @[@(StateType_8), @(TokenTypeRightB), @(StateType_9)],
            @[@(StateType_9), @(TokenTypeDot), @(StateTypeStart)],
            @[@(StateType_9), @(TokenTypeEOF), @(StateTypeEnd)],
        ];
        self.parseStates = @[
            @[@[@(StateTypeInst)], @(INST)], ///< INST
            @[@[@(StateType_8),@(StateType_4),@(StateType_2)], @(MM_IMM_REG1_REG2_S)], ///< MM_IMM_REG1_REG2_S
            @[@[@(StateType_8),@(StateType_4)], @(MM_REG1_REG2_S)], ///< MM_REG1_REG2_S
            @[@[@(StateType_8),@(StateType_5),@(StateType_2)], @(MM_IMM_REG2_S)], ///< MM_IMM_REG2_S
            @[@[@(StateType_8),@(StateType_5)], @(MM_REG2_S)], ///< MM_REG2_S
            @[@[@(StateType_6),@(StateType_2)], @(MM_IMM_REG1_REG2)], ///< MM_IMM_REG1_REG2
            @[@[@(StateType_6)], @(MM_REG1_REG2)], ///< MM_REG1_REG2
            @[@[@(StateType_4),@(StateType_2)], @(MM_IMM_REG)], ///< MM_IMM_REG
            @[@[@(StateType_4)], @(MM_REG)], ///< MM_REG
            @[@[@(StateType_2)], @(MM_IMM)], ///< MM_IMM
            @[@[@(StateType_1)], @(IMM)], ///< IMM
            @[@[@(StateType_0)], @(REG)], ///< REG
        ];

        self.instMap = @{
            @"mov" : @(INST_MOV),
            @"push" : @(INST_PUSH),
            @"pop" : @(INST_POP),
            @"leave" : @(INST_LEAVE),
            @"call" : @(INST_CALL),
            @"ret" : @(INST_RET),
            @"add" : @(INST_ADD),
            @"sub" : @(INST_SUB),
            @"cmp" : @(INST_CMP),
            @"jne" : @(INST_JNE),
            @"jmp" : @(INST_JMP),
        };
        
        self.regMap = @{
            @"rax" : @(RegType_rax),
            @"rsi" : @(RegType_rsi),
            @"rdi" : @(RegType_rdi),
        };
    }
    return self;
}

- (Express *)parserWithInst:(NSString *)inst {
    if (inst.length == 0) {
        return nil;
    }
    [self resetBuffer];
    [self.tokens removeAllObjects];
    [self.stateTypes removeAllObjects];
    self.stateType = StateTypeInst;
    [self.stateTypes addObject:@(StateTypeInst)];
    [inst getCharacters:buffer];
    char a = self.pre;
    NSString *token = nil;
    TokenType type = TokenTypeNone;
    self.od_type = EMPTY;
    printf("(%s)\n", inst.UTF8String);
    printf("%s->", [self stateToString:self.stateType].UTF8String);
    Express *express = [[Express alloc] init];
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
            [self next];
            token = [self parseNumber];
            type = TokenTypeImm;
        } else if (a == ' ') {
            [self next];
        }
        a = self.pre;
        if (token && type != TokenTypeNone) {
            [self addToken:token tokenType:type express:express];
        }
        type = TokenTypeNone;
        token = nil;
    }
    [self addToken:nil tokenType:TokenTypeEOF express:express];
    printf("\n");
    NSString *str = [self.tokens componentsJoinedByString:@"  "];
    printf("%s\n\n", str.UTF8String);
    return express;
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

- (void)addToken:(NSString *)token 
       tokenType:(TokenType)tokenType
         express:(Express *)express {
    if (tokenType == TokenTypeImm ||
        tokenType == TokenTypeInst ||
        tokenType == TokenTypeNum ||
        tokenType == TokenTypeReg) {        
        [self.tokens addObject:token];
    }

    [self runStateWithEvent:tokenType];
    if (self.stateType != StateTypeEnd) {
        printf("[%s]", token ? token.UTF8String : "");
    }
    printf("->%s", [self stateToString:self.stateType].UTF8String);
    if (self.stateType != StateTypeEnd) {
        printf("->");
    }
    
    if (self.stateType == StateTypeStart ||
        self.stateType == StateTypeEnd) {
        [self parseNodeWithExpress:express];
    }
}

- (void)resetBuffer {
    for (int i = 0; i < maxBufferSize; i++) {
        buffer[i] = '\0';
    }
    self.start = 0;
}

- (NSString *)tokenTypeToString:(TokenType)type {
    switch (type) {
        case TokenTypeNone:   return @"None";
        case TokenTypeImm:    return @"Imm";
        case TokenTypeNum:    return @"Num";
        case TokenTypeInst:   return @"Inst";
        case TokenTypeReg:    return @"Reg";
        case TokenTypeLeftB:  return @"LB";
        case TokenTypeRightB: return @"RB";
        case TokenTypeDot:    return @"Dot";
        case TokenTypeEOF:    return @"EOF";
        default: break;
    }
    return @"";
}

- (NSString *)stateToString:(StateType)type {
    switch (type) {
        case StateTypeNone:  return @"None";
        case StateTypeInst:  return @"Inst";
        case StateTypeStart: return @"Start";
        case StateType_0:    return @"0";
        case StateType_1:    return @"1";
        case StateType_2:    return @"2";
        case StateType_3:    return @"3";
        case StateType_4:    return @"4";
        case StateType_5:    return @"5";
        case StateType_6:    return @"6";
        case StateType_7:    return @"7";
        case StateType_8:    return @"8";
        case StateType_9:    return @"9";
        case StateTypeEnd:   return @"End";
        default: break;
    }
    return @"";
}

#pragma mark - 状态机
- (void)runStateWithEvent:(TokenType)tokenType {
    StateType state = [self findTransaction:self.stateType event:tokenType];
    if (state == StateTypeNone) {
        NSAssert(NO, @"状态错误");
    }
    self.stateType = state;
    [self.stateTypes addObject:@(state)];
}
 
- (StateType)findTransaction:(StateType)stateType event:(TokenType)tokenType {
    for (NSArray *trans in self.stateTransactions) {
        StateType from  = [trans[0] integerValue];
        TokenType event = [trans[1] integerValue];
        StateType to    = [trans[2] integerValue];
        
        if (stateType == from && event == tokenType) {
            return to;
        }
    }
    return StateTypeNone;
}

- (OdType)finallyState {
    for (int i = 0; i < self.parseStates.count; i++) {
        NSArray *fss = self.parseStates[i];
        NSAssert(fss.count == 2, @"final state error");
        if ([self statesContainSub:fss[0]]) {
            return (OdType)[fss[1] integerValue];
        }
    }
    return EMPTY;
}

- (BOOL)statesContainSub:(NSArray *)sub {
    if (![sub isKindOfClass:NSArray.class]) return NO;
    for (id state in sub) {
        if (![self.stateTypes containsObject:state]) {
            return NO;
        }
    }
    return YES;
}

- (void)parseNodeWithExpress:(Express *)express {
    OdType final = [self finallyState];
    Node *node = nil;
    if (final != INST) {
        node = [self expressNode:express];
        NSAssert(node, @"node 不能为空");
        node.type = final;
    }
    switch (final) {
        case REG:
        case MM_REG:
        {
            NSAssert(self.tokens.count == 1, @"IMM，tokens 数量必须为 1");
            node.reg1 = [self parseReg:self.tokens.firstObject];
            break;
        }
        case IMM:
        case MM_IMM:
        {
            NSAssert(self.tokens.count == 1, @"IMM，tokens 数量必须为 1");
            node.imm = [self parseImm:self.tokens.firstObject];
            break;
        }
        case MM_IMM_REG:/// 0x123(%rax)
        {
            NSAssert(self.tokens.count == 2, @"MM_IMM_REG，tokens 数量必须为 2");
            node.imm = [self parseImm:self.tokens[0]];
            node.reg1 = [self parseReg:self.tokens[1]];
            break;
        }
        case MM_REG1_REG2:
        {
            NSAssert(self.tokens.count == 2, @"MM_REG1_REG2，tokens 数量必须为 2");
            node.reg1 = [self parseReg:self.tokens[0]];
            node.reg2 = [self parseReg:self.tokens[1]];
            break;
        }
        case MM_IMM_REG1_REG2:
        {
            NSAssert(self.tokens.count == 3, @"MM_IMM_REG1_REG2，tokens 数量必须为 3");
            node.imm = [self parseImm:self.tokens[0]];
            node.reg1 = [self parseReg:self.tokens[1]];
            node.reg2 = [self parseReg:self.tokens[2]];
            break;
        }
        case MM_REG2_S:
        {
            NSAssert(self.tokens.count == 2, @"MM_REG2_S，tokens 数量必须为 2");
            node.reg2 = [self parseReg:self.tokens[0]];
            node.s = [self parseImm:self.tokens[1]];
            break;
        }
        case MM_IMM_REG2_S:
        {
            NSAssert(self.tokens.count == 3, @"MM_IMM_REG2_S，tokens 数量必须为 3");
            node.imm = [self parseImm:self.tokens[0]];
            node.reg2 = [self parseReg:self.tokens[1]];
            node.s = [self parseImm:self.tokens[2]];
            break;
        }
        case MM_REG1_REG2_S:
        {
            NSAssert(self.tokens.count == 3, @"MM_REG1_REG2_S，tokens 数量必须为 3");
            node.reg1 = [self parseReg:self.tokens[0]];
            node.reg2 = [self parseReg:self.tokens[1]];
            node.s = [self parseImm:self.tokens[2]];
            break;
        }
        case MM_IMM_REG1_REG2_S:
        {
            NSAssert(self.tokens.count == 4, @"MM_REG1_REG2_S，tokens 数量必须为 4");
            node.imm = [self parseImm:self.tokens[0]];
            node.reg1 = [self parseReg:self.tokens[1]];
            node.reg2 = [self parseReg:self.tokens[2]];
            node.s = [self parseImm:self.tokens[3]];
            break;
        }
        case INST:
        {
            NSAssert(self.tokens.count == 1, @"Inst，tokens 中的数量必须为 1");
            NSString *inst = [self.tokens.firstObject lowercaseString];
            InstType op = [self.instMap[inst] integerValue];
            NSAssert(op != INST_None, @"REG，寄存器不能为 None");
            express.op = op;
            break;
        }
        default: break;
    }
    [self.tokens removeAllObjects];
    [self.stateTypes removeAllObjects];
}

- (RegType)parseReg:(NSString *)reg {
    RegType type = [self.regMap[reg.lowercaseString] integerValue];
    NSAssert(type != RegType_none, @"REG，reg 不能为 None");
    return type;
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

- (Node *)expressNode:(Express *)express {
    if (!express.src) {
        express.src = [Node new];
        return express.src;
    } else if (!express.dst) {
        express.dst = [Node new];
        return express.dst;
    }
    NSAssert(NO, @"node 错误， src 和 dst 都已经解析过了");
    return nil;
}

@end
