//
//  ProjectionTableDataSource.h
//  Bracez
//
//  Created by Eldan Ben Haim on 21/07/2021.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#import "JsonDocument.h"

#import "ProjectionDefinition.h"

NS_ASSUME_NONNULL_BEGIN

@class ProjectionTableDataSource;

@protocol ProjectionTableDataSourceDelegate

-(void)projectionTableDataSource:(ProjectionTableDataSource*)sender didSelectNode:(Node*)node;

@end

@interface ProjectionTableDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>

-(instancetype)initWithDefinition:(ProjectionDefinition*)definition projectedDocument:(JsonDocument*)jsonFile;
-(void)configureTableViewColumns:(NSTableView*)tableView;
-(void)reloadData;
-(void)selectNode:(JsonCocoaNode*)node;

@property (readonly) ProjectionDefinition* projectionDefinition;
@property (weak) IBOutlet id<ProjectionTableDataSourceDelegate> delegate;
@property (weak) IBOutlet NSTableView *tableView;
@end

NS_ASSUME_NONNULL_END
