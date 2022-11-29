//
//  TableViewWithCopySupport.m
//  Bracez
//
//  Created by Eldan Ben-Haim on 29/11/2022.
//

#import "TableViewWithCopySupport.h"

@implementation TableViewWithCopySupport

-(void)copy:(id)sender {
    if([self.dataSource conformsToProtocol:@protocol(TableViewDataSourceWithCopySupport)]) {
        [(id<TableViewDataSourceWithCopySupport>)self.dataSource copyToClipboardWithSelection:self.selectedRowIndexes];
    }
}
@end
