//
//  HistoryAndFavoritesEditor.m
//  Bracez
//
//  Created by Eldan Ben Haim on 26/02/2021.
//

#import "HistoryAndFavoritesEditor.h"
#import "HistoryAndFavoritesControl.h"

@interface HistoryAndFavoritesEditor () <NSTableViewDataSource, NSTableViewDelegate> {
    HistoryAndFavoritesControl *_historyAndFavorites;
    
    NSMutableArray *_editedFavList;
    NSIndexSet *_selectedFavs;
}
@end

@implementation HistoryAndFavoritesEditor

-(instancetype)initWithHistoryAndFavorites:(HistoryAndFavoritesControl*)historyAndFavorites
{
    self = [super initWithWindowNibName:@"HistoryAndFavoritesEditor"];
    if(self) {
        _historyAndFavorites = historyAndFavorites;
        _editedFavList = [historyAndFavorites.favoritesList mutableCopy];
    }
    return self;
}

-(void)windowDidLoad {
    [super windowDidLoad];
    [self.favTable registerForDraggedTypes: [NSArray arrayWithObject: @"public.text"]];
}

-(void)setSelectedFavs:(NSIndexSet*)favs {
    [self willChangeValueForKey:@"hasFavSelection"];
    _selectedFavs = favs;
    [self didChangeValueForKey:@"hasFavSelection"];
}

-(NSIndexSet*)selectedFavs {
    return _selectedFavs;
}

-(BOOL)editSupported {
    return [(NSObject*)_historyAndFavorites.delegate respondsToSelector:@selector(hfControl:wantsEditForItem:inWindow:withCompletion:)];
}

-(BOOL)hasFavSelection {
    return [_selectedFavs count] > 0;
}

-(NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return _editedFavList.count;
}

-(NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSTableCellView *cellView = [tableView makeViewWithIdentifier:@"defaultCell" owner:tableView];
    cellView.textField.stringValue = [_historyAndFavorites nameForItem:[_editedFavList objectAtIndex:row]];
    return cellView;
}


-(id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row {
    NSData *favItem = [NSKeyedArchiver archivedDataWithRootObject:[_editedFavList objectAtIndex:row]
                                            requiringSecureCoding:YES
                                                            error:nil];
    NSPasteboardItem *pboardItem = [[NSPasteboardItem alloc] init];
    [pboardItem setPropertyList:@{ @"index": @(row), @"item": favItem } forType:@"public.text"];
     
    return pboardItem;
}

- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id<NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation {

    BOOL canDrop = row >= 0;
    if(canDrop) {
        return NSDragOperationMove;
    } else {
        return NSDragOperationNone;
    }
 }

-(BOOL)tableView:(NSTableView *)tableView acceptDrop:(id<NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation {
    
    if(dropOperation != NSDragOperationNone) {
        NSNumber *orgIndex = [info.draggingPasteboard propertyListForType:@"public.text"][@"index"];
        NSObject *item = [NSKeyedUnarchiver unarchivedObjectOfClass:[NSObject class]
                                                           fromData:[info.draggingPasteboard propertyListForType:@"public.text"][@"item"]
                                                              error:nil];
        
        [_editedFavList removeObjectAtIndex:orgIndex.unsignedIntValue];
        if(orgIndex.unsignedIntValue < row) {
            row --;
        }
        [_editedFavList insertObject:item atIndex:row];
    
        [self.favTable reloadData];
    }
    return YES;
}

- (IBAction)saveFavoritesClicked:(id)sender {
    _historyAndFavorites.favoritesList = _editedFavList;
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
}

- (IBAction)cancelClicked:(id)sender {
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)deleteClicked:(id)sender {
    [_editedFavList removeObjectsAtIndexes:self.selectedFavs];
    [self.favTable reloadData];
}
- (IBAction)editClicked:(id)sender {
    NSInteger editedIndex = self.selectedFavs.firstIndex;
    
    [_historyAndFavorites.delegate hfControl:_historyAndFavorites
                            wantsEditForItem:[_editedFavList objectAtIndex:editedIndex]
                                    inWindow:self.window
                              withCompletion:^(id  _Nonnull favObject, NSError * _Nullable error) {
        if(!error) {
            [self->_editedFavList setObject:favObject atIndexedSubscript:editedIndex];
        }
    }];
}

@end
