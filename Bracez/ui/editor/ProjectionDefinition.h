//
//  ProjectionDefinition.h
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN


@interface ProjectionFieldDefinition : NSObject <NSCopying, NSSecureCoding>

@property NSString *fieldTitle;
@property NSString *expression;
@property NSString *formatterType;

@end


@interface ProjectionDefinition : NSObject <NSCopying, NSSecureCoding>

+(instancetype)newDefinition;

-(void)addProjection:(ProjectionFieldDefinition*)definition;
-(void)removeProjectionAtIndex:(NSUInteger)index;

@property NSString *projectionName;
@property NSString *rowSelector;

@property (readonly) NSArray<ProjectionFieldDefinition*> *fieldDefinitions;

@end

NS_ASSUME_NONNULL_END


