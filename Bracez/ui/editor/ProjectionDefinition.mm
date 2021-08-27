//
//  ProjectionDefinition.m
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import "ProjectionDefinition.h"


@implementation ProjectionFieldDefinition

-(instancetype)init {
    self = [super init];
    if(self) {
        self.fieldTitle = @"Column";
        self.expression = @"$";
    }
    
    return self;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone {
    ProjectionFieldDefinition *copy = [[[self class] alloc] init];
    copy.fieldTitle = self.fieldTitle;
    copy.expression = self.expression;
    copy.formatterType = self.formatterType;
    
    return copy;
}

+(BOOL)supportsSecureCoding {
    return YES;
}

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
    [coder encodeObject:self.fieldTitle forKey:@"fieldTitle"];
    [coder encodeObject:self.expression forKey:@"expression"];
    [coder encodeObject:self.formatterType forKey:@"formatterType"];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
    self = [super init];
    if(self) {
        self.fieldTitle = [coder decodeObjectOfClass:[NSString class] forKey:@"fieldTitle"];
        self.expression = [coder decodeObjectOfClass:[NSString class] forKey:@"expression"];
        self.formatterType = [coder decodeObjectOfClass:[NSString class] forKey:@"formatterType"];
    }
    return self;
}

-(BOOL)isEqual:(id)other {
    if(![other isKindOfClass:[ProjectionFieldDefinition class]]) {
        return NO;
    }
    
    ProjectionFieldDefinition *otherDef = other;
    return [self.fieldTitle isEqualTo:otherDef.fieldTitle] &&
           [self.expression isEqualTo:otherDef.expression] &&
           ([self.formatterType isEqualTo:otherDef.formatterType] ||
                (self.formatterType == nil && otherDef.formatterType == nil)); 
}

@end


@interface ProjectionDefinition () {
    NSMutableArray *_fieldDefinitions;
}

@end

@implementation ProjectionDefinition

-(instancetype)init {
    self = [super init];
    _fieldDefinitions = [NSMutableArray arrayWithCapacity:3];
    return self;
}

+(instancetype)newDefinition {
    ProjectionDefinition *ret = [[ProjectionDefinition alloc] init];
    ret.projectionName = @"Untitled Projection";
    ret.rowSelector = @"$[*]";

    ProjectionFieldDefinition *fieldDef = [[ProjectionFieldDefinition alloc] init];
    fieldDef.fieldTitle = @"Result";
    fieldDef.expression = @"$";

    [ret addProjection:fieldDef];
    
    return ret;
}

-(void)addProjection:(ProjectionFieldDefinition*)definition {
    [_fieldDefinitions addObject:definition];
}

-(void)removeProjectionAtIndex:(NSUInteger)index {
    [_fieldDefinitions removeObjectAtIndex:index];
}

-(NSArray<ProjectionFieldDefinition*> *)fieldDefinitions {
    return _fieldDefinitions;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone {
    ProjectionDefinition *copy = [[[self class] alloc] init];
    
    copy.projectionName = self.projectionName;
    copy.rowSelector = self.rowSelector;
    
    for(ProjectionFieldDefinition *fieldDef in self.fieldDefinitions) {
        [copy addProjection:[fieldDef copy]];
    }
    
    return copy;
}

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
    [coder encodeObject:self.projectionName forKey:@"projectionName"];
    [coder encodeObject:self.rowSelector forKey:@"rowSelector"];
    [coder encodeObject:_fieldDefinitions forKey:@"fieldDefinitions"];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
    self = [super init];
    if(self) {
        self.projectionName = [coder decodeObjectOfClass:[NSString class] forKey:@"projectionName"];
        self.rowSelector = [coder decodeObjectOfClass:[NSString class] forKey:@"rowSelector"];
        _fieldDefinitions = [[coder decodeArrayOfObjectsOfClass:[ProjectionFieldDefinition class]
                                                         forKey:@"fieldDefinitions"] mutableCopy];
    }
    return self;
}

+(BOOL)supportsSecureCoding {
    return YES;
}

-(BOOL)isEqual:(id)other {
    if(![other isKindOfClass:[ProjectionDefinition class]]) {
        return NO;
    }
    
    ProjectionDefinition *otherDef = other;
    if(![self.rowSelector isEqualTo:otherDef.rowSelector]) {
        return NO;
    }
    
    if(![self.fieldDefinitions isEqualToArray:otherDef.fieldDefinitions]) {
        return NO;
    }
    
    return YES;
}

@end

