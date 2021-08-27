//
//  ProjectionTableController.h
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#import "JsonDocument.h"

#import "ProjectionDefinition.h"

NS_ASSUME_NONNULL_BEGIN

@class ProjectionTableController;

@protocol ProjectionTableControllerDelegate

-(void)projectionTableController:(ProjectionTableController*)sender didSelectNode:(Node*)node;

@end

@interface ProjectionTableController : NSObject <NSTableViewDataSource, NSTableViewDelegate>

-(void)selectNode:(JsonCocoaNode*)node;
-(void)reloadData;

@property (weak) IBOutlet NSTableView *tableView;
@property (weak) IBOutlet JsonDocument *projectedDocument;
@property (weak) IBOutlet id<ProjectionTableControllerDelegate> delegate;
@property (weak) IBOutlet ProjectionDefinition *projectionDefinition;
@end

NS_ASSUME_NONNULL_END
