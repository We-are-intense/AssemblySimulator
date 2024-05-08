//
//  Express.h
//  csapp
//
//  Created by erfeixia on 2024/4/13.
//

#import <Foundation/Foundation.h>
#import "Node.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, InstType) {
    INST_None,
    INST_MOV,           // 0
    INST_PUSH,          // 1
    INST_POP,           // 2
    INST_LEAVE,         // 3
    INST_CALL,          // 4
    INST_RET,           // 5
    INST_ADD,           // 6
    INST_SUB,           // 7
    INST_CMP,           // 8
    INST_JNE,           // 9
    INST_JMP,           // 10
    INST_XOR,           // 11
};

@interface Express : NSObject
@property (nonatomic, assign) InstType op;
@property (nonatomic, strong) Node *src;
@property (nonatomic, strong) Node *dst;
@end

NS_ASSUME_NONNULL_END
