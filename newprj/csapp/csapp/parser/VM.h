//
//  VM.h
//  csapp
//
//  Created by erfeixia on 2024/4/14.
//

#import <Foundation/Foundation.h>
#import "Express.h"
NS_ASSUME_NONNULL_BEGIN

@interface VM : NSObject

@property (nonatomic, strong, readonly) NSArray <Express *>* expresses;

- (instancetype)initWithExpresses:(NSArray <Express *>*)expresses;

- (void)run;
@end

NS_ASSUME_NONNULL_END
