//
//  JsonTreeDataSource.mm
//  JsonMockup
//
//  Created by Eldan on 7/29/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonTreeDataSource.h"
#import "JsonCocoaNode.h"

NSString * const JsonNodePboardType=@"JsonNode";

@implementation JsonTreeDataSource
- (BOOL) outlineView: (NSOutlineView *)ov isItemExpandable: (id)item { return NO; }
- (NSInteger) outlineView: (NSOutlineView *)ov numberOfChildrenOfItem:(id)item { return 0; }
- (id) outlineView: (NSOutlineView *)ov child:(long)index ofItem:(id)item { return nil; }
- (id)   outlineView: (NSOutlineView *)ov objectValueForTableColumn:(NSTableColumn*)col byItem:(id)item { return nil; }

- (BOOL) outlineView: (NSOutlineView *)ov
          acceptDrop: (id<NSDraggingInfo>)info
                item: (id)item
          childIndex: (NSInteger)index
{
   NSDictionary *lDropInfo = [[info draggingPasteboard] propertyListForType:JsonNodePboardType];
   
   __weak JsonCocoaNode **lNode = (__weak JsonCocoaNode**)[[lDropInfo objectForKey:@"draggedObject"] bytes];
   __weak  JsonCocoaNode **lParentNode = (__weak JsonCocoaNode**)[[lDropInfo objectForKey:@"draggedParent"] bytes];
   
   if(lNode && *lNode && lParentNode && *lParentNode)
   {
      [*lNode moveToNode:[item representedObject] atIndex:(int)index fromParent:*lParentNode];
   }
   
   return YES;
}


- (BOOL)outlineView:(NSOutlineView *)inOutlineView writeItems:(NSArray *)inItems toPasteboard:(NSPasteboard *)inPboard
{
   [inPboard declareTypes:[NSArray arrayWithObject:JsonNodePboardType] owner:nil];
   
   NSTreeNode *lDraggedTreeNode = [inItems objectAtIndex:0];
   JsonCocoaNode *lDraggedJsonNode = [lDraggedTreeNode representedObject];

   NSTreeNode *lDraggedParentTreeNode = [lDraggedTreeNode parentNode];
   JsonCocoaNode *lDraggedParentJsonNode = [lDraggedParentTreeNode representedObject];
   
   
   [inPboard setPropertyList:[NSDictionary dictionaryWithObjectsAndKeys:[NSData dataWithBytes:&lDraggedJsonNode length:sizeof(lDraggedJsonNode)], @"draggedObject",
                                                                        [NSData dataWithBytes:&lDraggedParentJsonNode length:sizeof(lDraggedJsonNode)], @"draggedParent",
                                                                        nil, nil]
              forType:JsonNodePboardType];
   return YES;
}

- (NSDragOperation)outlineView:(NSOutlineView *)inOutlineView validateDrop:(id <NSDraggingInfo>)inInfo proposedItem:(id)inItem proposedChildIndex:(NSInteger)inIndex
{
   if([[inItem representedObject] isContainer])
   {
      return NSDragOperationEvery;
   } else 
   {
      return NSDragOperationNone;
   }
}

@end
