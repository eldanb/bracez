//
//  ProjectionDefinitionEditor.h
//  Bracez
//
//  Created by Eldan Ben Haim on 24/07/2021.
//

#import <Cocoa/Cocoa.h>
#import "ProjectionDefinition.h"

NS_ASSUME_NONNULL_BEGIN

@class JsonDocument;

@interface ProjectionDefinitionEditor : NSWindowController

@property (readonly) ProjectionDefinition *editedProjection;

-(instancetype)initWithDefinition:(ProjectionDefinition*)editedDefinition
                  previewDocument:(JsonDocument* __nullable)previewDocument;
@end

NS_ASSUME_NONNULL_END
