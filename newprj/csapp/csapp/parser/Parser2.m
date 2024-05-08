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
#define maxBufferSize 64


@interface Parser2 ()
@property (nonatomic, assign) int start;
@end

@implementation Parser2 {
    unichar buffer[maxBufferSize];
}
- (Express *)parserWithInst:(NSString *)inst {
    
    return nil;
}

#pragma mark - Private Methods
- (NSString *)mm_reg {
    char a = [self pre];
    if (a != '(') {
        NSAssert(NO, @"parse mm_reg failed left not equal (");
    }
    [self next];
    NSString *str = [self reg];
    if (str == nil) {
        NSAssert(NO, @"parse mm_reg str failed");
    }
    a = [self pre];
    if (a != ')') {
        NSAssert(NO, @"parse mm_reg failed left not equal )");
    }
    [self next];
    return str;
}
- (NSString *)imm {
    char a = [self pre];
    if (a != '$') {
        NSAssert(NO, @"parse imm failed not equal $");
    }
    [self next];
    NSString *str = [self num];
    if (str == nil) {
        NSAssert(NO, @"parse imm str failed");
    }
    return str;
}

- (NSString *)reg {
    char a = [self pre];
    if (a != '%') {
        NSAssert(NO, @"parse reg failed not equal %%");
    }
    [self next];
    NSString *str = [self character];
    if (str == nil) {
        NSAssert(NO, @"parse reg str failed");
    }
    return str;
}
- (NSString *)s {
    char a = [self pre];
    if (a == '1' || a == '2' || a == '4' || a == '8') {
        
    } else {
        NSAssert(NO, @"parse s failed");
    }
    [self next];
    return [NSString stringWithFormat:@"%c", a];
}

- (NSString *)num {
    char a = [self pre];
    NSString *sysmbol = @"";
    if (a == '-') {
        [self next];
        sysmbol = @"-";
    }
    a = [self pre];
    char b = [self pre1];
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
    char a = [self pre];
    char str[32] = {'\0'};
    int index = 0;
    while (a != '\0' && (a >= '0' && a <= '9')) {
        str[index++] = [self next];
        a = [self pre];
    }
    if (index == 0) {
        NSAssert(NO, @"parse decimal failed");
    }
    return [NSString stringWithFormat:@"%s", str];
}
- (NSString *)hex {
    char a = [self pre];
    char b = [self pre1];
    
    if (a == '0' && (b == 'x' || b == 'X')) {
    } else {
        NSAssert(NO, @"parse hex failed");
    }
    [self next];
    [self next];
    
    a = [self pre];
    
    char str[32] = {'\0'};
    int index = 0;
    while (a != '\0' && ((a >= '0' && a <= '9') ||
                         (a >= 'a' && a <= 'f') ||
                         (a >= 'A' && a <= 'F'))) {
        str[index++] = [self next];
        a = [self pre];
    }
    if (index == 0) {
        NSAssert(NO, @"parse decimal failed");
    }
    return [NSString stringWithFormat:@"%s", str];
}
- (NSString *)character {
    char a = [self pre];
    char str[32] = {'\0'};
    int index = 0;
    while (a != '\0' && ((a >= 'a' && a <= 'z') ||
                         (a >= 'A' && a <= 'Z'))) {
        str[index++] = [self next];
        a = [self pre];
    }
    if (index == 0) {
        NSAssert(NO, @"parse character failed");
    }
    return [NSString stringWithFormat:@"%s", str];
}


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

- (char)pre1 {
    int index = self.start + 1;
    if(index >= maxBufferSize ||
       buffer[index] == '\0') {
        return '\0';
    }
    return buffer[index];
}
@end
