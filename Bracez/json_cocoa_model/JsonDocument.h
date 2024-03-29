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
#include "BookmarksList.h"
#include "JsonFileListenerObjCBridge.h"

using namespace json;

extern NSString *JsonDocumentBookmarkChangeNotification;
extern NSString *JsonDocumentSemanticModelUpdatedNotification;
extern NSString *JsonDocumentSemanticModelUpdatedNotificationKeyReason;
extern NSString *JsonDocumentSemanticModelUpdatedNotificationReasonNodeInvalidation;
extern NSString *JsonDocumentSemanticModelUpdatedNotificationReasonReparse;

class BookmarksListenerBridge;

@interface JsonDocument : NSDocument<ObjCJsonFileChangeListener, NSTextStorageDelegate> {
}

@property NSTextStorage *textStorage;

-(id)init;

- (NSString *)windowNibName;

-(BOOL)isSemanticModelTextChangeInProgress;
-(BOOL)isSemanticModelDirty;
-(BOOL)isSemanticModelUpdateInProgress;

-(JsonCocoaNode*)rootNode;
-(NSIndexPath*)findPathContaining:(TextCoordinate)aDocOffset;
-(void)translateCoordinate:(TextCoordinate)aCoord toRow:(int*)aRow col:(int*)aCol;
-(NSUInteger)characterIndexForFirstCharOfLine:(UInt32)lineNumber;
-(NSUInteger)lineCount;

-(BookmarksList&)bookmarks;
-(const MarkerList<ParseErrorMarker>*)errors;

-(NSArray*)problems; // This is errors + schema errors, cocoa'ed
-(NSArray*)bookmarkMarkers; // This is boomarks, cocoa'ed

-(JsonFile*)jsonFile;
-(NSIndexPath*)pathFromJsonPathString:(NSString*)jsonPathString;

-(void)reindentStartingAt:(TextCoordinate)aOffsetStart
                      len:(TextLength)aLen
    suggestNewEndLocation:(TextCoordinate*)newEndLocation;

-(int)suggestIdentForNewLineAt:(TextCoordinate)where;
-(int)suggestCloserIndentAt:(TextCoordinate)where getLineStart:(TextCoordinate*)lineStart;

-(void)notifyErrorsChanged:(json::JsonFile*)aSender;
-(void)notifyNodeInvalidated:(json::JsonFile*)aSender nodePath:(const json::JsonPath&)nodePath;
-(void)notifyBookmarksUpdated;

-(BOOL)documentBusy;
@end
