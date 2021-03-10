//
//  HistoryAndFavoritesConrol.m
//  Bracez
//
//  Created by Eldan Ben Haim on 23/02/2021.
//

#import "HistoryAndFavoritesControl.h"
#import "HistoryAndFavoritesEditor.h"

#define MAX_HISTORY_LEN 10

@interface HistoryAndFavoritesControl () <NSMenuDelegate> {
    NSPopUpButton *button;
    
    NSMenu *favMenu;
    NSMenu *histMenu;
    
    NSMutableArray<NSString*> *historyItems;
    NSMutableArray<NSString*> *favItems;
}

@end

@implementation HistoryAndFavoritesControl

-(void)awakeFromNib {
    button = [[NSPopUpButton alloc] initWithFrame:self.bounds pullsDown:YES];
    button.bezelStyle = NSBezelStyleRoundRect;
    button.bordered = NO;
    button.imagePosition = NSImageOnly;
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
    NSMenuItem *hist = [[NSMenuItem alloc] initWithTitle:@"History" action:nil keyEquivalent:@""];
    [menu addItem:hist];
    histMenu = [[NSMenu alloc] init];
    hist.submenu = histMenu;

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
    NSDictionary *hflistData = [[NSUserDefaults standardUserDefaults] dictionaryForKey:self.storageKey];
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
    
    [[NSUserDefaults standardUserDefaults] setObject:hflistData forKey:self.storageKey];
}

-(void)addFavoriteValue:(id)favValue {
    [self loadLists];
    [favItems addObject:favValue];
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

-(void)menuNeedsUpdate:(NSMenu *)menu {
    [self loadLists];
    
    [favMenu removeAllItems];
    if(favItems.count) {
        for(NSString *favItemTitle in favItems) {
            NSMenuItem *favMenuItem = [[NSMenuItem alloc] initWithTitle:favItemTitle
                                         action:@selector(selectHfItem:)
                                  keyEquivalent:@""];
            [favMenuItem setTarget:self];
            [favMenu addItem:favMenuItem];
        }
    } else {
        NSMenuItem *emptyItem = [[NSMenuItem alloc] initWithTitle:@"No Items" action:nil keyEquivalent:@""];
        emptyItem.enabled = NO;
        [favMenu addItem:emptyItem];
    }

    [histMenu removeAllItems];
    if(historyItems.count) {
        for(NSString *historyItemTitle in historyItems) {
            NSMenuItem *histMenuItem = [[NSMenuItem alloc] initWithTitle:historyItemTitle
                                         action:@selector(selectHfItem:)
                                  keyEquivalent:@""];
            [histMenuItem setTarget:self];
            [histMenu addItem:histMenuItem];
        }
    } else {
        NSMenuItem *emptyItem = [[NSMenuItem alloc] initWithTitle:@"No Items" action:nil keyEquivalent:@""];
        emptyItem.enabled = NO;
        [histMenu addItem:emptyItem];
    }
}


-(void)selectHfItem:(NSMenuItem*)sender {
    self.boundField.stringValue = sender.title;
}

-(void)addFavorite:(id)sender {
    [self addFavoriteValue:self.boundField.stringValue];
}

-(void)editFavorites:(id)sender {
    HistoryAndFavoritesEditor *editor = [[HistoryAndFavoritesEditor alloc] initWithHistoryAndFavorites:self];
    [self.window beginSheet:editor.window completionHandler:^(NSModalResponse returnCode) {
        
    }];
}

-(void)accumulateHistory {
    NSString *accumulatedItem = self.boundField.stringValue;
    if(!accumulatedItem.length) {
        return;
    }
    
    [self accumulateHistoryValue:accumulatedItem];
}

-(NSArray *)favoritesList {
    return favItems;
}

-(void)setFavoritesList:(NSArray *)favoritesList {
    [self loadLists];
    favItems = [favoritesList mutableCopy];
    [self saveLists];
}
@end
