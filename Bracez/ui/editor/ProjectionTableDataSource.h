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

@interface ProjectionTableDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>

-(instancetype)initWithDefinition:(ProjectionDefinition*)definition projectedDocument:(JsonDocument*)jsonFile;
-(void)configureTableViewColumns:(NSTableView*)tableView;
-(void)reloadData;

@property (readonly) ProjectionDefinition* projectionDefinition;

@end

NS_ASSUME_NONNULL_END
