//
//  JsonDocument.mm
//  JsonMockup
//
//  Created by Eldan on 15/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonDocument.h"
#import "JsonMarker.h"
#import "SyntaxHilightJsonVisitor.h"
#import "NSString+WStringUtils.h"
#include "BracezPreferences.h"
#include "marker_list.h"
#include "JsonIndentFormatter.hpp"
#include "stopwatch.h"

#define MAX_LOCAL_EDIT_LEN 1024

NSString *JsonDocumentBookmarkChangeNotification =
@"com.zigsterz.braces.jsondocument.bookmarkchange";

NSString *JsonDocumentSemanticModelUpdatedNotification =
@"com.zigsterz.braces.jsondocument.semanticmodelchange";

NSString *JsonDocumentSemanticModelUpdatedNotificationKeyReason =
@"reason";

NSString *JsonDocumentSemanticModelUpdatedNotificationReasonNodeInvalidation =
@"nodeInvalidation";

NSString *JsonDocumentSemanticModelUpdatedNotificationReasonReparse =
@"reparse";


@interface JsonDocument () {
    BOOL _isSemanticModelTextChangeInProgress;
    BOOL _isSemanticModelDirty;
    BOOL _isSemanticModelUpdateInProgress;
    
    JsonFile *file;
    
    BookmarksList bookmarks;
    
    JsonFileListenerObjCBridge *jsonSemanticListenerBridge;
    BookmarksListenerBridge *bookmarksListenerBridge;
    
    NSArray *problemWrappers;
    NSArray *bookmarkWrappers;
        
    JsonCocoaNode *_cocoaNode;
}

@end

class BookmarksListenerBridge : public BookmarksChangeListener
{
public:
    BookmarksListenerBridge(JsonDocument *aDoc) : document(aDoc)  { }
    
    void bookmarksChanged(BookmarksList *aSender)
    {
        [document notifyBookmarksUpdated];
    }
    
private:
    __weak JsonDocument *document;
    
} ;

@interface JsonDocument () <NSTextStorageDelegate> {
    NSMutableArray<void (^)(JsonFile *file)> *_pendingSemanticDataRequests;
}

@end

@implementation JsonDocument

-(id)init
{
    self = [super init];
    if(self)
    {
        _pendingSemanticDataRequests = [NSMutableArray array];
        
        _isSemanticModelUpdateInProgress = NO;
        _isSemanticModelDirty = NO;

        self.textStorage = [[NSTextStorage alloc] init];
        self.textStorage.delegate = self;
        
        jsonSemanticListenerBridge = new JsonFileListenerObjCBridge(self);
        bookmarksListenerBridge = new BookmarksListenerBridge(self);
        bookmarks.addListener(bookmarksListenerBridge);
        
        file = new JsonFile();
        file->addListener(jsonSemanticListenerBridge);
        
    }
    
    return self;
}

-(void)dealloc {
    if(file) {
        delete file;
        file = NULL;
    }
    
    if(bookmarksListenerBridge) {
        delete bookmarksListenerBridge;
        bookmarksListenerBridge = NULL;
    }
    
    if(jsonSemanticListenerBridge) {
        delete jsonSemanticListenerBridge;
        jsonSemanticListenerBridge = NULL;
    }
}


- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError
{
    NSString *dataAsString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    
    [self.textStorage.mutableString setString:dataAsString];
    
    [self updateChangeCount:NSChangeCleared];
    return YES;
}

- (NSString *)windowNibName
{
    return @"EditorWindow";
}

-(NSArray*)bookmarkMarkers
{
    if(!bookmarkWrappers)
    {
        NSMutableArray *lMarkerArray = [[NSMutableArray alloc] init];
        
        for(LineMarkerList::const_iterator lIter = bookmarks.begin();
            lIter != bookmarks.end();
            lIter++)
        {
            unsigned long bookmarkLine = lIter->getCoordinate().getAddress();
            TextCoordinate lineStart = file->getLineFirstCharacter( bookmarkLine );
            NSString *textStorageString = self.textStorage.string;
            
            NSString *lineContent = [textStorageString substringWithRange:NSIntersectionRange(NSMakeRange(0, textStorageString.length),
                                                                                              NSMakeRange(lineStart.getAddress(), 150))];
            NSString *trimmedLineContent = [lineContent stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
            NSString *lineDesc = [trimmedLineContent substringWithRange:NSIntersectionRange(NSMakeRange(0, trimmedLineContent.length),
                                                                                            NSMakeRange(0, 50))];
            
            [lMarkerArray addObject:[[JsonMarker alloc] initWithLine:(int)(lIter->getCoordinate().getAddress())
                                                         description:lineDesc
                                                          markerType:JsonMarkerTypeBookmark
                                                           parentDoc:self]];
        }
        
        bookmarkWrappers = lMarkerArray;
    }
    
    return bookmarkWrappers;
}

-(NSArray*)problems
{
    if(!problemWrappers)
    {
        NSMutableArray *lProbArray = [[NSMutableArray alloc] init];
        const MarkerList<ParseErrorMarker> &lParseErrors = file->getErrors();
        
        for(MarkerList<ParseErrorMarker>::const_iterator lIter = lParseErrors.begin();
            lIter != lParseErrors.end();
            lIter++)
        {
            [lProbArray addObject:[[JsonMarker alloc] initWithDescription:[NSString stringWithUTF8String:lIter->getErrorText().c_str()]
                                                               markerType:JsonMarkerTypeError
                                                                     code:0
                                                               coordinate:(TextCoordinate)*lIter
                                                                parentDoc:self]];
        }
        
        problemWrappers = lProbArray;
    }
    return problemWrappers;
}

-(JsonFile*)jsonFile
{
    return file;
}

-(BookmarksList&)bookmarks
{
    return bookmarks;
}

-(const MarkerList<ParseErrorMarker>*)errors;
{
    return &(file->getErrors());
}

-(JsonCocoaNode*)rootNode
{
    if(!_cocoaNode) {
        _cocoaNode = [JsonCocoaNode nodeForElement:file->getDom()->GetChildAt(0) withName:@"root"];
    }
    return _cocoaNode;
}

-(NSIndexPath*)indexPathFromJsonPath:(const JsonPath &)aPath
{
    NSIndexPath *lRet = [NSIndexPath indexPathWithIndex:0];
    
    JsonCocoaNode *lNode = [self rootNode];
    for(JsonPath::const_iterator iter=aPath.begin(); iter!=aPath.end(); iter++)
    {
        int lIdx = *iter;
        lRet = [lRet indexPathByAddingIndex:lIdx];
        lNode = [lNode objectInChildrenAtIndex:lIdx];
    }
    
    return lRet;
}

-(NSIndexPath*)findPathContaining:(TextCoordinate)aDocOffset
{
    JsonPath lPath;
    if(file->FindPathContaining(aDocOffset, lPath))
    {
        return [self indexPathFromJsonPath:lPath];
    } else {
        return nil;
    }
}

-(NSIndexPath*)pathFromJsonPathString:(NSString*)jsonPathString {
    JsonPath lPath;
    if(file->FindPathForJsonPathString(jsonPathString.cStringWstring.c_str(), lPath)) {
        return [self indexPathFromJsonPath:lPath];
    } else {
        return nil;
    }
}

-(void)translateCoordinate:(TextCoordinate)aCoord toRow:(int*)aRow col:(int*)aCol
{
    int lRow;
    int lCol;
    file->getCoordinateRowCol(aCoord, lRow, lCol);
    
    *aRow = lRow;
    *aCol = lCol;
}

-(NSUInteger)characterIndexForFirstCharOfLine:(UInt32)lineNumber {
    return (lineNumber-1)<file->numLines() ?
            file->getLineFirstCharacter(lineNumber).getAddress() :
    -1;
}

-(NSUInteger)lineCount {
    return file->numLines();
}

-(void)notifyJsonTextSpliced:(JsonFile*)aSender
                        from:(TextCoordinate)aOldOffset
                      length:(TextLength)aOldLength
                   newLength:(TextLength)aNewLength
           affectedLineStart:(TextCoordinate)aOldLineStart
             numDeletedLines:(TextLength)aOldLineLength
               numAddedLines:(TextLength)aNewLineLength
{
    if(!_isSemanticModelUpdateInProgress) {
        _isSemanticModelTextChangeInProgress = YES;
        
        NSString *lNewText = [NSString stringWithWstring:aSender->getText().substr(aOldOffset.getAddress(), aNewLength)];
        [self.textStorage replaceCharactersInRange:NSMakeRange(aOldOffset.getAddress(), aOldLength)
                                        withString:lNewText];
        
        _isSemanticModelTextChangeInProgress = NO;
    }
    
    self.bookmarks.updateBookmarksByLineSplice(aOldLineStart, aOldLineLength, aNewLineLength);
}

-(void)notifyErrorsChanged:(json::JsonFile*)aSender
{
    [self willChangeValueForKey:@"problems"];
    problemWrappers = nil;
    [self didChangeValueForKey:@"problems"];
}

-(void)notifyBookmarksUpdated
{
    [self willChangeValueForKey:@"bookmarkMarkers"];
    bookmarkWrappers = nil;
    [self didChangeValueForKey:@"bookmarkMarkers"];
    
    [[NSNotificationCenter defaultCenter] postNotificationName:JsonDocumentBookmarkChangeNotification  object:self
                                                      userInfo:nil];
}

-(NSData *)dataOfType:(NSString *)typeName error:(NSError * _Nullable __autoreleasing *)outError {
    if([typeName isEqualToString:@"JsonFile"]) {
        return [self.textStorage.string dataUsingEncoding:NSUTF8StringEncoding];
    } else {
        NSLog(@"Invalid typename for dataOfType:%@", typeName);
        return nil;
    }
}

- (void)textStorage:(NSTextStorage *)textStorage didProcessEditing:(NSTextStorageEditActions)editedMask range:(NSRange)editedRange changeInLength:(NSInteger)delta API_AVAILABLE(macos(10.11), ios(7.0), tvos(9.0)) {
    if(editedMask & NSTextStorageEditedCharacters)
    {
        [self updateChangeCount:NSChangeDone];
        
        NSString *updatedRegion = [textStorage.string substringWithRange:editedRange];
        
        if(!_isSemanticModelTextChangeInProgress) {
            _isSemanticModelUpdateInProgress = true;
            
            bool spliceResult = !file->getErrors().size() &&
                                 file->spliceTextWithWorkLimit(TextCoordinate(editedRange.location),
                                                               editedRange.length-delta,
                                                               updatedRegion.cStringWstring,
                                                               MAX_LOCAL_EDIT_LEN);
            
            _isSemanticModelUpdateInProgress = false;

            if(!spliceResult) {
                _isSemanticModelDirty = YES;
                [self slowSpliceFileContentAt:TextCoordinate(editedRange.location)
                                       length:editedRange.length-delta
                                      newText:updatedRegion.cStringWstring];
            }
        }


        if(!_isSemanticModelDirty) {
            [self updateSyntaxInRange:editedRange];
        }
    }
}


-(void)reindentStartingAt:(TextCoordinate)aOffsetStart len:(TextLength)aLen suggestNewEndLocation:(TextCoordinate*)newEndLocation {
    std::wstring jsonText = _textStorage.string.cStringWstring;
    
    // Get line start
    int row, col;
    file->getCoordinateRowCol(aOffsetStart, row, col);
    aOffsetStart = aOffsetStart - (col - 1);
    aLen += (col - 1);
    
    JsonIndentFormatter fixer(jsonText, *file, aOffsetStart, aLen,
                              [BracezPreferences sharedPreferences].indentSize);
    const std::wstring &indent = fixer.getIndented();
    aLen = fixer.getIndentedLength();
    
    [_textStorage replaceCharactersInRange:NSMakeRange(aOffsetStart, aLen) withString:[NSString stringWithWstring:indent]];
    if(newEndLocation) {
        *newEndLocation = aOffsetStart + indent.length();
    }
}

-(int)suggestIdentForNewLineAt:(TextCoordinate)where {
    if(where == 0) {
        return 0;
    }
    
    const json::Node *n = file->FindNodeContaining(where, NULL, true);
    if(!n) {
        return 0;
    }
    
    const json::ContainerNode *container = dynamic_cast<const json::ContainerNode*>(n);
    if(!container) {
        container = n->GetParent();
    }
    
    TextCoordinate containerStartColAddr = getContainerStartColumnAddr(container);
    int containerStartCol, containerStartRow;
    file->getCoordinateRowCol(containerStartColAddr, containerStartRow, containerStartCol);
    return containerStartCol-1 + [BracezPreferences sharedPreferences].indentSize;
}

-(int)suggestCloserIndentAt:(TextCoordinate)where getLineStart:(TextCoordinate*)lineStart {
    int row, col;
    file->getCoordinateRowCol(where, row, col);
    *lineStart = file->getLineStart(row);
    
    const json::Node *n = file->FindNodeContaining(where, NULL);
    const json::ContainerNode *container = dynamic_cast<const json::ContainerNode*>(n);
    if(container) {
        TextCoordinate containerStartColAddr = getContainerStartColumnAddr(container);
        int containerStartCol, containerStartRow;
        file->getCoordinateRowCol(containerStartColAddr, containerStartRow, containerStartCol);
        return containerStartCol-1;
    } else {
        return col;
    }
}

-(void)updateSyntaxInRange:(NSRange)editedRange {
    
    NSFont *lFont = [[BracezPreferences sharedPreferences] editorFont];
    stopwatch lStopWatch("Update syntax coloring");
    
    [self.textStorage beginEditing];
    
    if(editedRange.location == 0 && editedRange.length == self.textStorage.length) {
        [self.textStorage setAttributeRuns:[NSArray array]];
        [self.textStorage setFont:lFont];
    } else {
        //[self.textStorage setAttributes:@{} range:editedRange];
    }
    
    SyntaxHighlightJsonVisitor visitor(self.textStorage, editedRange);
    file->getDom()->accept(&visitor);
    
    [self.textStorage endEditing];
}

-(void)slowSpliceFileContentAt:(TextCoordinate)aOffsetStart length:(TextLength)aLen newText:(const std::wstring &)aNewText {
    _isSemanticModelUpdateInProgress = true;

    std::shared_ptr<JsonFileSemanticModelReconciliationTask> reconcile = file->spliceTextWithDirtySemanticModel(aOffsetStart, aLen, aNewText);

    _isSemanticModelUpdateInProgress = false;

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        reconcile->executeInBackground();
        
        dispatch_async(dispatch_get_main_queue(), ^{
            self->file->applyReconciliationTask(reconcile);
            self->_isSemanticModelUpdateInProgress = true;
            [self willChangeValueForKey:@"rootNode"];
            [self willChangeValueForKey:@"problems"];

            self->_cocoaNode = nil;
            self->problemWrappers = nil;
            
            [self didChangeValueForKey:@"problems"];
            [self didChangeValueForKey:@"rootNode"];
            self->_isSemanticModelUpdateInProgress = false;
            self->_isSemanticModelDirty = NO;

            [self updateSyntaxInRange:NSMakeRange(0, self.textStorage.string.length)];
            [self notifySemanticModelUpdatedWithReason:JsonDocumentSemanticModelUpdatedNotificationReasonReparse];

        });
    });
}

-(BOOL)isSemanticModelDirty {
    return _isSemanticModelDirty;
}

-(BOOL)isSemanticModelTextChangeInProgress {
    return _isSemanticModelTextChangeInProgress;
}

-(BOOL)isSemanticModelUpdateInProgress {
    return _isSemanticModelUpdateInProgress;
}


-(void)notifyNodeInvalidated:(json::JsonFile*)aSender nodePath:(const json::JsonPath&)nodePath {
    
    JsonCocoaNode *node = self.rootNode;
    
    for(auto iter = nodePath.begin(); iter != nodePath.end(); iter++) {
        node = [node objectInChildrenAtIndex:*iter];
    }
    
    [node reloadFromElement: ((json::ContainerNode*)node.proxiedElement->GetParent())->GetChildAt(nodePath.back())];
    
    [self notifySemanticModelUpdatedWithReason:JsonDocumentSemanticModelUpdatedNotificationReasonNodeInvalidation];
}

-(void)notifySemanticModelUpdatedWithReason:(NSString*)reason {
    [[NSNotificationCenter defaultCenter] postNotificationName:JsonDocumentSemanticModelUpdatedNotification
                                                        object:self
                                                      userInfo:@{ JsonDocumentSemanticModelUpdatedNotificationKeyReason: reason }];
}
@end


