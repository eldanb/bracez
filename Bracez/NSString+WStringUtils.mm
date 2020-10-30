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

-(wchar_t*)cStringWchar {
    return (wchar_t*)[self cStringUsingEncoding:NSUTF32LittleEndianStringEncoding];
}

@end
