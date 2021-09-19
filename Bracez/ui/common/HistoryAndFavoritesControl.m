//
//  HistoryAndFavoritesConrol.m
//  Bracez
//
//  Created by Eldan Ben Haim on 23/02/2021.
//

#import "HistoryAndFavoritesControl.h"
#import "HistoryAndFavoritesEditor.h"

#define MAX_HISTORY_LEN 10

// ********************************************

@interface TextFieldHistoryFavEditorDelegate : NSObject<HistoryAndFavoritesControlDelegate> {
}

+(TextFieldHistoryFavEditorDelegate*)shared;
@end

@implementation TextFieldHistoryFavEditorDelegate

+(TextFieldHistoryFavEditorDelegate*)shared {
    static dispatch_once_t donce;
    static TextFieldHistoryFavEditorDelegate* shared = nil;
    dispatch_once(&donce, ^() {
        shared = [[TextFieldHistoryFavEditorDelegate alloc] init];
    });
        
    return shared;
}


- (nonnull NSString *)hfControl:(nonnull HistoryAndFavoritesControl *)sender wantsNameForFavoriteItem:(nonnull id)favoriteItem {
    return favoriteItem;
}

- (void)hfControl:(nonnull HistoryAndFavoritesControl *)sender wantsNewFavoriteObjectWithCompletion:(nonnull void (^)(id _Nonnull, NSError * nullable))completionHandler {
    completionHandler(sender.boundField.stringValue, nil);
}

- (void)hfControl:(nonnull HistoryAndFavoritesControl *)sender recallItem:(nonnull id)recalledItem {
    sender.boundField.stringValue = recalledItem;
    NSDictionary *bindingInfo = [sender.boundField infoForBinding: NSValueBinding];
    [[bindingInfo valueForKey: NSObservedObjectKey] setValue: sender.boundField.stringValue
                                                  forKeyPath: [bindingInfo valueForKey: NSObservedKeyPathKey]];
}

-(id)newHistoryItemForHfControl:(HistoryAndFavoritesControl *)sender {
    NSString *ret = sender.boundField.stringValue;
    if(ret.length) {
        return ret;
    } else {
        return nil;
    }
}


@end

// ********************************************

@interface TextViewHistoryFavEditorDelegate : NSObject<HistoryAndFavoritesControlDelegate> {
}

+(TextViewHistoryFavEditorDelegate*)shared;
@end

@implementation TextViewHistoryFavEditorDelegate

+(TextViewHistoryFavEditorDelegate*)shared {
    static dispatch_once_t donce;
    static TextViewHistoryFavEditorDelegate* shared = nil;
    dispatch_once(&donce, ^() {
        shared = [[TextViewHistoryFavEditorDelegate alloc] init];
    });
        
    return shared;
}


- (nonnull NSString *)hfControl:(nonnull HistoryAndFavoritesControl *)sender wantsNameForFavoriteItem:(nonnull id)favoriteItem {
    return favoriteItem;
}

- (void)hfControl:(nonnull HistoryAndFavoritesControl *)sender wantsNewFavoriteObjectWithCompletion:(nonnull void (^)(id _Nonnull, NSError * nullable))completionHandler {
    completionHandler(sender.boundTextView.string, nil);
}

- (void)hfControl:(nonnull HistoryAndFavoritesControl *)sender recallItem:(nonnull id)recalledItem {
    NSDictionary *bindingInfo = [sender.boundTextView infoForBinding: NSValueBinding];
    [[bindingInfo valueForKey: NSObservedObjectKey] setValue: recalledItem
                                                  forKeyPath: [bindingInfo valueForKey: NSObservedKeyPathKey]];
    sender.boundTextView.string = recalledItem;
}


-(id)newHistoryItemForHfControl:(HistoryAndFavoritesControl *)sender {
    NSString *ret = sender.boundTextView.string;
    if(ret.length) {
        return ret;
    } else {
        return nil;
    }
}
@end

// ********************************************


@interface HistoryAndFavoritesControl () <NSMenuDelegate> {
    NSPopUpButton *button;
    
    NSMenu *favMenu;
    NSMenu *histMenu;
    
    NSMutableArray<NSString*> *historyItems;
    NSMutableArray<id> *favItems;
    
    NSTextField *_boundTextField;
    NSTextView *_boundTextView;
}

@end

@implementation HistoryAndFavoritesControl

-(void)awakeFromNib {
    [super awakeFromNib];
    
    button = [[NSPopUpButton alloc] initWithFrame:self.bounds pullsDown:YES];
    button.bezelStyle = NSBezelStyleRoundRect;
    button.bordered = NO;
    button.imagePosition = NSImageOnly;
    button.toolTip = self.toolTip;
    [(id)button.cell setArrowPosition:NSPopUpArrowAtBottom];
    
    NSMenu *menu = [[NSMenu alloc] init];
    menu.delegate = self;
    
    // Button image
    NSMenuItem *title = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    title.image = [NSImage imageWithSystemSymbolName:@"star" accessibilityDescription:@"favorites"];
    [menu addItem:title];
    
    // Favorites list
    NSMenuItem *fav = [[NSMenuItem alloc] initWithTitle:@"Favorites" action:nil keyEquivalent:@""];
    [menu addItem:fav];
    favMenu = [[NSMenu alloc] init];
    fav.submenu = favMenu;
    
    // Favorite actions
    NSMenuItem *addFavorite = [[NSMenuItem alloc] initWithTitle:@"Add to Favorites"
                                                         action:@selector(addFavorite:)
                                                  keyEquivalent:@""];
    [addFavorite setTarget:self];
    [menu addItem:addFavorite];
    NSMenuItem *manageFavorites = [[NSMenuItem alloc] initWithTitle:@"Edit favorites..."
                                                         action:@selector(editFavorites:)
                                                  keyEquivalent:@""];
    [manageFavorites setTarget:self];
    [menu addItem:manageFavorites];

    // Sep
    NSMenuItem *sep = [NSMenuItem separatorItem];
    [menu addItem:sep];
    
    // History
    if(self.isHistoryEnabled) {
        NSMenuItem *hist = [[NSMenuItem alloc] initWithTitle:@"History" action:nil keyEquivalent:@""];
        [menu addItem:hist];
        histMenu = [[NSMenu alloc] init];
        hist.submenu = histMenu;
    }
    
    button.menu = menu;
    [self addSubview:button];
}


-(void) layout {
    button.frame = self.bounds;
}

-(NSString*)storageKey {
    return [NSString stringWithFormat:@"hflist.%@", self.historyListName];
}

-(void) loadLists {
    NSError *err;
    NSDictionary *hflistData;
    
    NSData *loadedData = [[NSUserDefaults standardUserDefaults] dataForKey:self.storageKey];
    if(loadedData && [loadedData isKindOfClass:[NSData class]]) {
        hflistData = [NSKeyedUnarchiver unarchivedObjectOfClass:[NSObject class]
                                                       fromData:loadedData
                                                          error:&err];
    }
    
    if(!hflistData) {
        hflistData = [[NSUserDefaults standardUserDefaults] dictionaryForKey:self.storageKey];
    }
    
    historyItems = [(NSArray*)(hflistData[@"history"]) mutableCopy];
    favItems = [(NSArray*)(hflistData[@"favorites"]) mutableCopy];
    
    if(!historyItems) {
        historyItems = [NSMutableArray array];
    }
    if(!favItems) {
        favItems = [NSMutableArray array];
    }
}

-(void)saveLists {
    NSDictionary *hflistData = @{
        @"history": historyItems,
        @"favorites": favItems
    };
    
    NSData *favListArchive = [NSKeyedArchiver archivedDataWithRootObject:hflistData
                                                   requiringSecureCoding:YES
                                                                   error:nil];
    [[NSUserDefaults standardUserDefaults] setObject:favListArchive forKey:self.storageKey];
}

-(void)addFavoriteValue:(id)favValue {
    NSString *newFavName = [self.delegate hfControl:self wantsNameForFavoriteItem:favValue];
    
    [self loadLists];
    NSIndexSet *existingItemIndex = [favItems indexesOfObjectsPassingTest:^BOOL(NSString * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        NSString *existingFavName = [self.delegate hfControl:self wantsNameForFavoriteItem:obj];
        return [existingFavName isEqualToString:newFavName];
    }];
    
    if(existingItemIndex.count > 0) {
        [favItems setObject:favValue atIndexedSubscript:existingItemIndex.firstIndex];
    } else {
        [favItems addObject:favValue];
    }
    
    [self saveLists];
}

-(void)accumulateHistoryValue:(id)accumulatedItem {
    [self loadLists];
    if([historyItems indexOfObject:accumulatedItem] == NSNotFound) {
        [historyItems insertObject:accumulatedItem atIndex:0];
        if(historyItems.count > MAX_HISTORY_LEN) {
            NSUInteger nToRemove = historyItems.count-MAX_HISTORY_LEN;
            [historyItems removeObjectsInRange:NSMakeRange(MAX_HISTORY_LEN, nToRemove)];
        }
    }
    [self saveLists];
}

-(BOOL)isHistoryEnabled {
    return [(NSObject*)self.delegate respondsToSelector:@selector(newHistoryItemForHfControl:)];
}

-(void)menuNeedsUpdate:(NSMenu *)menu {
    
    [self loadLists];
    
    [favMenu removeAllItems];
    if(favItems.count) {
        for(id favItem in favItems) {
            NSString *favItemTitle = [self nameForItem:favItem];
            NSMenuItem *favMenuItem = [[NSMenuItem alloc] initWithTitle:favItemTitle
                                         action:@selector(selectHfItem:)
                                  keyEquivalent:@""];
            favMenuItem.representedObject = favItem;
            [favMenuItem setTarget:self];
            [favMenu addItem:favMenuItem];
        }
    } else {
        NSMenuItem *emptyItem = [[NSMenuItem alloc] initWithTitle:@"No Items" action:nil keyEquivalent:@""];
        emptyItem.enabled = NO;
        [favMenu addItem:emptyItem];
    }

    if(self.isHistoryEnabled) {
        [histMenu removeAllItems];
        if(historyItems.count) {
            for(id historyItem in historyItems) {
                NSString *historyItemTitle = [self nameForItem:historyItem];
                NSMenuItem *histMenuItem = [[NSMenuItem alloc] initWithTitle:historyItemTitle
                                             action:@selector(selectHfItem:)
                                      keyEquivalent:@""];
                histMenuItem.representedObject = historyItem;
                [histMenuItem setTarget:self];
                [histMenu addItem:histMenuItem];
            }
        } else {
            NSMenuItem *emptyItem = [[NSMenuItem alloc] initWithTitle:@"No Items" action:nil keyEquivalent:@""];
            emptyItem.enabled = NO;
            [histMenu addItem:emptyItem];
        }
    }
}


-(void)selectHfItem:(NSMenuItem*)sender {
    [self.delegate hfControl:self recallItem:sender.representedObject];
}

-(void)addFavorite:(id)sender {
    [self.delegate hfControl:self
                            wantsNewFavoriteObjectWithCompletion:^(id  _Nonnull favObject, NSError * _Nullable error) {
        if(favObject) {
            [self addFavoriteValue:favObject];
        }
    }];
}

-(void)editFavorites:(id)sender {
    HistoryAndFavoritesEditor *editor = [[HistoryAndFavoritesEditor alloc] initWithHistoryAndFavorites:self];
    [self.window beginSheet:editor.window completionHandler:^(NSModalResponse returnCode) {
        
    }];
}

-(void)accumulateHistory {
    id accumulatedItem = [self.delegate newHistoryItemForHfControl:self];
    if(accumulatedItem) {
        [self accumulateHistoryValue:accumulatedItem];
    }
}

-(NSArray *)favoritesList {
    if(!favItems) {
        [self loadLists];
    }
    
    return favItems;
}

-(void)setFavoritesList:(NSArray *)favoritesList {
    [self loadLists];
    favItems = [favoritesList mutableCopy];
    [self saveLists];
}

-(NSString*)nameForItem:(id)favoriteItem {
    return [self.delegate hfControl:self wantsNameForFavoriteItem:favoriteItem];
}

-(void)setBoundTextView:(NSTextView * __nullable)boundTextView {
    _boundTextView = boundTextView;
    
    if(_boundTextView) {
        self.boundField = nil;
        self.delegate = [TextViewHistoryFavEditorDelegate shared];
    }
}

-(NSTextView*)boundTextView {
    return _boundTextView;
}

-(void)setBoundField:(NSTextField  * __nullable)boundField {
    _boundTextField = boundField;
    
    if(_boundTextField) {
        self.boundTextView = nil;
        self.delegate = [TextFieldHistoryFavEditorDelegate shared];
    }
}

-(NSTextField*)boundField {
    return _boundTextField;
}


@end
