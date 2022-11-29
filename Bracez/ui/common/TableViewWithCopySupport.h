//
//  TableViewWithCopySupport.h
//  Bracez
//
//  Created by Eldan Ben-Haim on 29/11/2022.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@protocol TableViewDataSourceWithCopySupport
-(void)copyToClipboardWithSelection:(NSIndexSet*)selection;
@end

@interface TableViewWithCopySupport : NSTableView

@end

NS_ASSUME_NONNULL_END
