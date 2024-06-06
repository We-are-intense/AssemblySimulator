//
//  main.m
//  csapp
//
//  Created by erfeixia on 2024/4/2.
//

#import <Foundation/Foundation.h>
#import "Parser.h"
#import "Parser2.h"
#import "VM.h"
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        if (argc != 2 || argv[1] == NULL) return 0;
        NSString *path = [NSString stringWithFormat:@"%s/csapp/parser/inst.txt", argv[1]];
        NSError *error = nil;
        NSString *fileContent = [NSString stringWithContentsOfFile:path
                                                          encoding:NSUTF8StringEncoding
                                                             error:&error];
        if (error) {
            NSLog(@"error: %@", error.localizedDescription);
            return 0;
        }
        NSArray *lines = [fileContent componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];
        
        Parser2 *p = [Parser2 new];
        NSMutableArray *expresses = [NSMutableArray array];
        for(int i = 0; i < lines.count; i++) {
            NSString *line = lines[i];
            NSLog(@"指令: %@", line);
            Express *express = [p parserWithInst:line line:i];
            if (express) {
                [expresses addObject:express];
            }
        }
        [p reLocation];
        
//        for (NSString *line in lines) {
//            // 对每一行进行处理
//            Express *express = [p parserWithInst:line];
//            [expresses addObject:express];
//        }
        
//        VM *vm = [[VM alloc] initWithExpresses:expresses];
//        [vm run];
    }
    return 0;
}
