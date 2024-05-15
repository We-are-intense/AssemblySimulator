//
//  Parser2.m
//  csapp
//
//  Created by erfeixia on 2024/5/8.
//

/*
 label = character:
 statement =  inst
            | inst express
            | inst express, express
 inst = character
 express =    imm
            | mm_imm
            | reg
            | mm_reg
            | mm_imm_reg
            | mm_reg1_reg2
            | mm_imm_reg1_reg2
            | mm_reg2_s
            | mm_imm_reg2_s
            | mm_reg1_reg2_s
            | mm_imm_reg1_reg2_s
 
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
 
 
 
 
 */
#import "Parser2.h"
#import "Token.h"

#define maxBufferSize 64


@interface Parser2 ()
@property (nonatomic, assign) int start;
@property (nonatomic, strong) NSMutableArray <Token *>*tokens;
@property (nonatomic, strong) NSMutableArray <NSString *>*chars;
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
    }
    return self;
}

- (Express *)parserWithInst:(NSString *)inst {
    [self resetBuffer];
    
    
    return nil;
}

#pragma mark - Private Methods
- (NSString *)mm_reg {
    char a = [self peekChar];
    if (a != '(') {
        NSAssert(NO, @"parse mm_reg failed left not equal (");
    }
    [self nextChar];
    NSString *str = [self reg];
    if (str == nil) {
        NSAssert(NO, @"parse mm_reg str failed");
    }
    a = [self peekChar];
    if (a != ')') {
        NSAssert(NO, @"parse mm_reg failed left not equal )");
    }
    [self nextChar];
    return str;
}
- (NSString *)imm {
    char a = [self peekChar];
    if (a != '$') {
        NSAssert(NO, @"parse imm failed not equal $");
    }
    [self nextChar];
    NSString *str = [self num];
    if (str == nil) {
        NSAssert(NO, @"parse imm str failed");
    }
    return str;
}

- (NSString *)reg {
    char a = [self peekChar];
    if (a != '%') {
        NSAssert(NO, @"parse reg failed not equal %%");
    }
    [self nextChar];
    NSString *str = [self character];
    if (str == nil) {
        NSAssert(NO, @"parse reg str failed");
    }
    return str;
}
- (NSString *)s {
    char a = [self peekChar];
    if (a == '1' || a == '2' || a == '4' || a == '8') {
        
    } else {
        NSAssert(NO, @"parse s failed");
    }
    [self nextChar];
    return [NSString stringWithFormat:@"%c", a];
}

- (NSString *)num {
    char a = [self peekChar];
    NSString *sysmbol = @"";
    if (a == '-') {
        [self nextChar];
        sysmbol = @"-";
    }
    a = [self peekChar];
    char b = [self peekNChar:1];
    NSString *str = nil;
    if (a == '0' && (b == 'x' || b == 'X')) {
        str = [self hex];
    } else {
        str = [self decimal];
    }
    
    if (str == nil) {
        NSAssert(NO, @"parse num failed");
    }
    return [NSString stringWithFormat:@"%@%@", sysmbol, str];
}
- (NSString *)decimal {
    char a = [self peekChar];
    char str[32] = {'\0'};
    int index = 0;
    while (a != '\0' && (a >= '0' && a <= '9')) {
        str[index++] = [self nextChar];
        a = [self peekChar];
    }
    if (index == 0) {
        NSAssert(NO, @"parse decimal failed");
    }
    return [NSString stringWithFormat:@"%s", str];
}
- (NSString *)hex {
    char a = [self peekChar];
    char b = [self peekNChar:1];
    
    if (a == '0' && (b == 'x' || b == 'X')) {
    } else {
        NSAssert(NO, @"parse hex failed");
    }
    [self nextChar];
    [self nextChar];
    
    a = [self peekChar];
    
    char str[32] = {'\0'};
    int index = 0;
    while (a != '\0' && ((a >= '0' && a <= '9') ||
                         (a >= 'a' && a <= 'f') ||
                         (a >= 'A' && a <= 'F'))) {
        str[index++] = [self nextChar];
        a = [self peekChar];
    }
    if (index == 0) {
        NSAssert(NO, @"parse decimal failed");
    }
    return [NSString stringWithFormat:@"%s", str];
}
- (NSString *)character {
    char a = [self peekChar];
    char str[32] = {'\0'};
    int index = 0;
    while (a != '\0' && ((a >= 'a' && a <= 'z') ||
                         (a >= 'A' && a <= 'Z'))) {
        str[index++] = [self nextChar];
        a = [self peekChar];
    }
    if (index == 0) {
        NSAssert(NO, @"parse character failed");
    }
    return [NSString stringWithFormat:@"%s", str];
}

- (void)resetBuffer {
    for (int i = 0; i < maxBufferSize; i++) {
        buffer[i] = '\0';
        tokenChar[i] = '\0';
    }
    self.start = 0;
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
    
    return nil;
}
/*
 typedef NS_ENUM(NSInteger, TokenType) {
     TokenTypeNone   , //--> mov
     TokenTypeString , //--> mov
     TokenTypeHex    , //--> 0x123
     TokenTypeDecimal, //--> 123
     TokenTypeDollar , //--> $
     TokenTypeColon  , //--> :
     TokenTypeLP     , //--> (
     TokenTypeRP     , //--> )
     TokenTypeComma  , //--> ,
     TokenTypePersent, //--> %
 };
 */
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
    
    return nil;
}

- (Token *)parseDecimal {

    return nil;
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

@end
