//
//  Token.h
//  csapp
//
//  Created by erfeixia on 2024/5/15.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

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

@interface Token : NSObject
@property (nonatomic,   copy) NSString *token;
@property (nonatomic, assign) TokenType tokenType;
@end

NS_ASSUME_NONNULL_END
