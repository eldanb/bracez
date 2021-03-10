//
//  NSString+WStringUtils.m
//  Bracez
//
//  Created by Eldan Ben Haim on 30/10/2020.
//

#import "NSString+WStringUtils.h"

@implementation NSString (WStringUtils)

+(NSString*)stringWithWchar:(wchar_t*)input {
    return [[NSString alloc] initWithBytes:input
                                    length:wcslen(input)*sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}

+(NSString*)stringWithWstring:(const std::wstring &)input {
    return [[NSString alloc] initWithBytes:input.c_str()
                                    length:input.length()*sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}

-(std::wstring)cStringWstring {
    NSUInteger len = self.length;
    std::wstring ret(len, 0);
    
    NSUInteger usedLen;
    [self getBytes:ret.begin().base()
         maxLength:len*sizeof(wchar_t)
        usedLength:&usedLen
          encoding:NSUTF32LittleEndianStringEncoding
           options:0
             range:NSMakeRange(0, self.length)
    remainingRange:0];

    return ret;
    //return (wchar_t*)[self cStringUsingEncoding:NSUTF32LittleEndianStringEncoding];
}

@end
