//
//  ProjectionDefinition.h
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN


@interface ProjectionFieldDefinition : NSObject <NSCopying, NSSecureCoding>

-(BOOL)isEqual:(id __nullable)other;


@property NSString *fieldTitle;
@property NSString *expression;
@property NSString *formatterType;

@end


@interface ProjectionDefinition : NSObject <NSCopying, NSSecureCoding>

+(instancetype)newDefinition;

-(void)addProjection:(ProjectionFieldDefinition*)definition;
-(void)removeProjectionAtIndex:(NSUInteger)index;
-(void)insertProjection:(ProjectionFieldDefinition*)definition atIndex:(NSUInteger)index;

-(BOOL)isEqual:(id __nullable)other;

@property NSString *projectionName;
@property NSString *rowSelector;

@property (readonly) NSArray<ProjectionFieldDefinition*> *fieldDefinitions;


@end

NS_ASSUME_NONNULL_END


