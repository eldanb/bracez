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

@interface JsonDocument () {
    BOOL _isSemanticModelTextChangeInProgress;
    BOOL _isSemanticModelDirty;
    BOOL _isSemanticModelUpdateInProgress;
    
    JsonFile *file;
     
    LinesAndBookmarks linesAndBookmarks;
    
    JsonFileListenerObjCBridge *jsonSemanticListenerBridge;
    BookmarksListenerBridge *bookmarksListenerBridge;
      
    NSArray *problemWrappers;
    NSArray *bookmarkWrappers;

    NSObject *_lastRequestedSemanticModelRefresh;
    
    JsonCocoaNode *_cocoaNode;
}

@end

class BookmarksListenerBridge : public BookmarksChangeListener
{
public:
   BookmarksListenerBridge(JsonDocument *aDoc) : document(aDoc)  { }
   
   void bookmarksChanged(LinesAndBookmarks *aSender)
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
       
      self.textStorage = [[NSTextStorage alloc] init];
      self.textStorage.delegate = self;

      jsonSemanticListenerBridge = new JsonFileListenerObjCBridge(self);
      bookmarksListenerBridge = new BookmarksListenerBridge(self);
      linesAndBookmarks.addListener(bookmarksListenerBridge);

      [self updateSemanticModel:new JsonFile()];
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
      
      for(LineMarkerList::const_iterator lIter = linesAndBookmarks.begin();
          lIter != linesAndBookmarks.end();
          lIter++)
      {
          unsigned long bookmarkLine = lIter->getCoordinate().getAddress();
          TextCoordinate lineStart = linesAndBookmarks.getLineFirstCharacter( bookmarkLine );
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

-(LinesAndBookmarks&)bookmarks
{
   return linesAndBookmarks;
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
   linesAndBookmarks.getCoordinateRowCol(aCoord, lRow, lCol);
   
   *aRow = lRow;
   *aCol = lCol;
}

-(NSUInteger)characterIndexForFirstCharOfLine:(UInt32)lineNumber {
    return (lineNumber-1)<linesAndBookmarks.numLines() ?
                linesAndBookmarks.getLineFirstCharacter(lineNumber).getAddress() :
                -1;
}

-(void)updateSemanticModel:(JsonFile*)aFile
{
    _isSemanticModelUpdateInProgress = true;
  [self willChangeValueForKey:@"rootNode"];
  [self willChangeValueForKey:@"problems"];
    
    if(file) {
        delete file;
        file = NULL;
    }
    
  file = aFile;
    _cocoaNode = nil;
    
    if(file) {
        file->addListener(jsonSemanticListenerBridge);
    }
  
    problemWrappers = nil;
  [self didChangeValueForKey:@"problems"];
  [self didChangeValueForKey:@"rootNode"];
  
  _isSemanticModelDirty = NO;
    _isSemanticModelUpdateInProgress = false;
    
  [self updateSyntaxInRange:NSMakeRange(0, self.textStorage.string.length)];
}

-(void)notifyJsonTextSpliced:(JsonFile*)aSender from:(TextCoordinate)aOldOffset length:(TextLength)aOldLength newLength:(TextLength)aNewLength
{
   _isSemanticModelTextChangeInProgress = YES;

    
    NSString *lNewText = [NSString stringWithWstring:aSender->getText().substr(aOldOffset.getAddress(), aNewLength)];
    [self.textStorage replaceCharactersInRange:NSMakeRange(aOldOffset.getAddress(), aOldLength)
                                    withString:lNewText];
   
   
    _isSemanticModelTextChangeInProgress = NO;
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
            if(!file->spliceTextWithWorkLimit(TextCoordinate(editedRange.location),
                                               editedRange.length-delta,
                                               updatedRegion.cStringWstring,
                                               MAX_LOCAL_EDIT_LEN)) {
                _isSemanticModelDirty = YES;
                [self slowParseFileContent];
            }
            _isSemanticModelUpdateInProgress = false;
        }

        linesAndBookmarks.updateLineOffsetsAfterSplice(TextCoordinate(editedRange.location),
                                                       editedRange.length-delta,
                                                       editedRange.length,
                                                       updatedRegion.cStringWstring.c_str());

        if(!_isSemanticModelDirty) {
            [self updateSyntaxInRange:editedRange];
        }
    }
}


-(void)reindentStartingAt:(TextCoordinate)aOffsetStart len:(TextLength)aLen {
    std::wstring jsonText = _textStorage.string.cStringWstring;
    
    int row, col;
    linesAndBookmarks.getCoordinateRowCol(aOffsetStart, row, col);
    aOffsetStart = aOffsetStart - (col - 1);
    
    JsonIndentFormatter fixer(jsonText, *file, linesAndBookmarks, aOffsetStart, aLen,
                              [BracezPreferences sharedPreferences].indentSize);
    const std::wstring &indent = fixer.getIndented();
        
    [_textStorage replaceCharactersInRange:NSMakeRange(aOffsetStart, aLen) withString:[NSString stringWithWstring:indent]];
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
    linesAndBookmarks.getCoordinateRowCol(containerStartColAddr, containerStartRow, containerStartCol);
    return containerStartCol-1 + [BracezPreferences sharedPreferences].indentSize;
}

-(int)suggestCloserIndentAt:(TextCoordinate)where getLineStart:(TextCoordinate*)lineStart {
    int row, col;
    linesAndBookmarks.getCoordinateRowCol(where, row, col);
    *lineStart = linesAndBookmarks.getLineStart(row);
        
    const json::Node *n = file->FindNodeContaining(where, NULL);
    const json::ContainerNode *container = dynamic_cast<const json::ContainerNode*>(n);
    if(container) {
        TextCoordinate containerStartColAddr = getContainerStartColumnAddr(container);
        int containerStartCol, containerStartRow;
        linesAndBookmarks.getCoordinateRowCol(containerStartColAddr, containerStartRow, containerStartCol);
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

-(void)slowParseFileContent {
    NSObject *modelRefreshTag = [[NSObject alloc] init];
    _lastRequestedSemanticModelRefresh = modelRefreshTag;
    
    std::wstring lstr = self.textStorage.string.cStringWstring;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        JsonFile *tfile = new JsonFile();
        tfile->setText(lstr);

        //sleep(3);
        dispatch_async(dispatch_get_main_queue(), ^{
            if(modelRefreshTag == self->_lastRequestedSemanticModelRefresh) {
                [self updateSemanticModel:tfile];
            } else {
                delete tfile;
            }
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
    JsonCocoaNode *parentNode = nil;
    
    for(auto iter = nodePath.begin(); iter != nodePath.end(); iter++) {
        parentNode = node;
        node = [node objectInChildrenAtIndex:*iter];
    }
    
    [node reloadFromElement: ((json::ContainerNode*)parentNode.proxiedElement)->GetChildAt(nodePath.back())];
}

@end
