//
//  JsonTreeDataSource.h
//  JsonMockup
//
//  Created by Eldan on 7/29/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Foundation/Foundation.h>


extern NSString * const JsonNodePboardType;

@class JSONWindow;

@interface JsonTreeDataSource : NSObject <NSOutlineViewDataSource> {
}

-(instancetype)initWithWindow:(JSONWindow*)aWindow;

@end
