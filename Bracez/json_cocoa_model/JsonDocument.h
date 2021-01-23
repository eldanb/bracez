//
//  JsonDocument.h
//  JsonMockup
//
//  Created by Eldan on 15/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "JsonCocoaNode.h"
#include "json_file.h"
#include "LinesAndBookmarks.h"
#include "JsonFileListenerObjCBridge.h"

using namespace json;

extern NSString *JsonDocumentBookmarkChangeNotification;

class BookmarksListenerBridge;

@interface JsonDocument : NSDocument<ObjCJsonFileChangeListener, NSTextStorageDelegate> {
}

@property NSTextStorage *textStorage;

-(id)init;
- (void)close;

- (NSString *)windowNibName;

-(BOOL)isSemanticModelTextChangeInProgress;
-(BOOL)isSemanticModelDirty;

-(JsonCocoaNode*)rootNode;
-(NSIndexPath*)findPathContaining:(unsigned int)aDocOffset;
-(void)translateCoordinate:(TextCoordinate)aCoord toRow:(NSUInteger*)aRow col:(NSUInteger*)aCol;
-(UInt32)characterIndexForStartOfLine:(UInt32)lineNumber;

-(LinesAndBookmarks&)bookmarks;
-(const MarkerList<ParseErrorMarker>*)errors;

-(NSArray*)problems; // This is errors + schema errors, cocoa'ed
-(NSArray*)bookmarkMarkers; // This is boomarks, cocoa'ed

-(JsonFile*)jsonFile;
-(NSIndexPath*)pathFromJsonPathString:(NSString*)jsonPathString;

-(void)reindentStartingAt:(TextCoordinate)aOffsetStart len:(TextLength)aLen;

-(void)notifyErrorsChanged:(json::JsonFile*)aSender;
-(void)notifyBookmarksUpdated;
@end
