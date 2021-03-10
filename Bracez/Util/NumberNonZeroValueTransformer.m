//
//  NumberNonZeroValueTransformer.m
//  Bracez
//
//  Created by Eldan Ben Haim on 21/10/2020.
//

#import "NumberNonZeroValueTransformer.h"

@implementation NumberNonZeroValueTransformer

-(id)transformedValue:(id)value {
    return [NSNumber numberWithBool:[value intValue] != 0];
}

@end
