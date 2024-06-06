//
//  Parser2.h
//  csapp
//
//  Created by erfeixia on 2024/5/8.
//

#import <Foundation/Foundation.h>
#import "Express.h"
NS_ASSUME_NONNULL_BEGIN

@interface Parser2 : NSObject
- (Express *)parserWithInst:(NSString *)inst 
                       line:(NSInteger)line;

- (void)reLocation;
@end

NS_ASSUME_NONNULL_END
