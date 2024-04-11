//
//  main.m
//  csapp
//
//  Created by erfeixia on 2024/4/2.
//

#import <Foundation/Foundation.h>
#import "parser.h"
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        parser *p = [parser new];
        NSArray *insts = @[
            @"mov 0x123, %rax",
            @"mov (%rsi), %rax",
            @"mov 0x12(%rsi), %rax",
            @"mov (%rsi, %rdi), %rax",
            @"mov 0x12(%rsi, %rdi), %rax",
            @"mov (, %rsi, 2), %rax",
            @"mov 0x12(, %rsi, 4), %rax",
            @"mov (%rsi, %rdi, 1), %rax",
            @"mov 0x12(%rsi, %rdi, 8), %rax"
        ];
        for (NSString *inst in insts) {
            [p parserWithInst:inst];
        }
    }
    return 0;
}
