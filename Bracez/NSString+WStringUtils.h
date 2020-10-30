//
//  NSString+WStringUtils.h
//  Bracez
//
//  Created by Eldan Ben Haim on 30/10/2020.
//

#import <Foundation/Foundation.h>
#include <string>

NS_ASSUME_NONNULL_BEGIN

@interface NSString (WStringUtils)

+(NSString*)stringWithWchar:(wchar_t*)input;
+(NSString*)stringWithWstring:(const std::wstring &)input;

-(wchar_t*)cStringWchar;

@end

NS_ASSUME_NONNULL_END
