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
#include "marker_list.h"


NSString *JsonDocumentBookmarkChangeNotification =
    @"com.zigsterz.braces.jsondocument.bookmarkchange";

@interface JsonDocument () {
    BOOL _isSemanticModelTextChangeInProgress;
    BOOL _isSemanticModelDirty;
    
    JsonFile *file;
     
    LinesAndBookmarks linesAndBookmarks;
    
    JsonFileListenerObjCBridge *jsonSemanticListenerBridge;
    BookmarksListenerBridge *bookmarksListenerBridge;
      
    NSArray *problemWrappers;
    NSArray *bookmarkWrappers;

    NSObject *_lastRequestedSemanticModelRefresh;
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
   JsonDocument *document;
   
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

- (void)close
{
   [super close]; 
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
          TextCoordinate lineStart = linesAndBookmarks.getLineStart( (*lIter) - 1);
          NSString *textStorageString = self.textStorage.string;
          
          NSString *lineContent = [textStorageString substringWithRange:NSIntersectionRange(NSMakeRange(0, textStorageString.length),
                                                                                            NSMakeRange(lineStart+1, 150))];
          NSString *trimmedLineContent = [lineContent stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
          NSString *lineDesc = [trimmedLineContent substringWithRange:NSIntersectionRange(NSMakeRange(0, trimmedLineContent.length),
                                                                                          NSMakeRange(0, 50))];
          
          [lMarkerArray addObject:[[JsonMarker alloc] initWithLine:(TextCoordinate)*lIter
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
   return [JsonCocoaNode nodeForElement:file->getDom()->GetChildAt(0) withName:@"root"];
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

-(NSIndexPath*)findPathContaining:(unsigned int)aDocOffset
{
   JsonPath lPath;
   file->FindPathContaining(aDocOffset, lPath);
   
   return [self indexPathFromJsonPath:lPath];
}

-(NSIndexPath*)pathFromJsonPathString:(NSString*)jsonPathString {
    JsonPath lPath;
    if(file->FindPathForJsonPathString(jsonPathString.cStringWstring.c_str(), lPath)) {
        return [self indexPathFromJsonPath:lPath];
    } else {
        return nil;
    }
}

-(void)translateCoordinate:(TextCoordinate)aCoord toRow:(NSUInteger*)aRow col:(NSUInteger*)aCol
{
   unsigned long lRow;
   unsigned long lCol;
   linesAndBookmarks.getCoordinateRowCol(aCoord, lRow, lCol);
   
   *aRow = lRow;
   *aCol = lCol;
}

-(void)updateSemanticModel:(JsonFile*)aFile
{
  [self willChangeValueForKey:@"rootNode"];
  [self willChangeValueForKey:@"problems"];
    
    if(file) {
        delete file;
        file = NULL;
    }
    
  file = aFile;
    
    if(file) {
        file->addListener(jsonSemanticListenerBridge);
    }
  
    problemWrappers = nil;
  [self didChangeValueForKey:@"problems"];
  [self didChangeValueForKey:@"rootNode"];
  
  _isSemanticModelDirty = NO;

  [self updateSyntaxInRange:NSMakeRange(0, self.textStorage.string.length)];
}

-(void)notifyJsonTextSpliced:(JsonFile*)aSender from:(TextCoordinate)aOldOffset length:(TextLength)aOldLength newLength:(TextLength)aNewLength
{
   _isSemanticModelTextChangeInProgress = YES;

    
    NSString *lNewText = [NSString stringWithWstring:aSender->getText().substr(aOldOffset, aNewLength)];
    [self.textStorage replaceCharactersInRange:NSMakeRange(aOldOffset, aOldLength)
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
        
        if(!_isSemanticModelTextChangeInProgress) {
            /*
                NSRange spliceRange = NSMakeRange(editedRange.location, editedRange.length-delta);
                NSString *updatedText = [text substringWithRange:editedRange];
             
                // Try to handle change in a fast local way
            */

            _isSemanticModelDirty = YES;
            [self slowParseFileContent];
        }

        if(!_isSemanticModelDirty) {
            [self updateSyntaxInRange:editedRange];
        }
        
        linesAndBookmarks.updateLineOffsetsAfterSplice(editedRange.location, editedRange.length-delta, editedRange.length,
                                                       textStorage.string.UTF8String);
    }
}

class JsonIndentationFixer {
public:
    JsonIndentationFixer(const std::wstring &text, TextCoordinate aOffsetStart, TextLength aLen)
        : jsonText(text) {
        int textLen = jsonText.length();
        startOffset = aOffsetStart;

        if(startOffset+aLen > textLen) {
            aLen = textLen - startOffset;
        }
        endOffset = startOffset + aLen;
    }
    
    const std::wstring &getIndented() {
        if(!output.length()) {
            reindent();
        }
        
        return output;
    }
    
private:
    void detectIndentLevelsAtStart() {
        bool inIndent = true;
        int lineIndentLen = 0;
        while(inputOffset < startOffset) {
            const wchar_t curChar = readCharacter();
            
            if(curChar == '\n') {
                inIndent = true;
                continue;
            }
            
            if(!inString)
            {
                if(inIndent && !isspace(curChar)) {
                    inIndent = false;
                    lineIndentLen = inputCol;
                }
                
                switch(curChar) {
                    case L'"':
                        inString = true;
                        break;
                        
                    case L'{':
                    case L'[':
                        indentLevelStack.push_back(lineIndentLen + 3);
                        break;
                        
                    case L'}':
                    case L']':
                        if(!indentLevelStack.empty()) {
                            indentLevelStack.pop_back();
                        }
                        break;
                }
            } else {
                switch(curChar) {
                    case L'\\':
                        inEscape = true;
                        break;
                    
                    case L'"':
                        if(!inEscape) {
                            inString = false;
                        } else {
                            inEscape = false;
                        }
                        break;
                        
                    default:
                        inEscape = false;
                        break;
                }
            }
        }
    }
        
    void reindent() {
        if(startOffset > jsonText.length()) {
            return;
        }
        
        inputCol = 0;
        inputOffset = 0;
        outputCol = 0;
        
        prevNewLine = false;
        inEscape = false;
        inString = false;

        detectIndentLevelsAtStart();

        int peekedInputCol = peekInputColumn();
        outputCol = peekedInputCol;
        if(peekedInputCol == 0) {
            skipInputWhitespace();
        }
                
        while(inputOffset < endOffset) {
            wchar_t curCharacter = readCharacter();
            if(curCharacter == '\n') {
                outputCharacter(curCharacter);
                skipInputWhitespace();
            } else
            if(!inString) {
                switch(curCharacter) {
                    case L'{':
                    case L'[': {
                        outputCharacter(curCharacter);
                        indentLevelStack.push_back((indentLevelStack.empty() ? 0 : indentLevelStack.back()) + 3);
                        copyAndEnsureNewLine();
                        break;
                    }
                    
                    case L'}':
                    case L']': {
                        if(!indentLevelStack.empty()) {
                            indentLevelStack.pop_back();
                        }
                        
                        if(outputCol != 0) {
                            outputCharacter(L'\n');
                        }
                        
                        outputCharacter(curCharacter);
                        break;
                    }
                        
                    case L',':
                        outputCharacter(curCharacter);
                        copyAndEnsureNewLine();
                        break;
                        
                    default:
                        outputCharacter(curCharacter);
                        break;
                }
            }
            else {
                if(curCharacter == L'\\') {
                    inEscape = true;
                } else
                if(curCharacter == L'"' && !inEscape) {
                    inString = false;
                } else {
                    inEscape = false;
                }
                
                outputCharacter(curCharacter);
            }
        }
    }
       
    // Read a character and update the
    // input col to reflect the col of the last read character
    wchar_t readCharacter() {
        const wchar_t ret = jsonText[inputOffset];
        
        if(prevNewLine) {
            inputCol = 0;
        } else {
            inputCol++;
        }
        
        prevNewLine = ret == L'\n';
        inputOffset++;
        
        return ret;
    }
    
    // Return the next character that will be read by
    // readCharacter
    wchar_t peekCharacter() {
        return jsonText[inputOffset];
    }

    // Return the column of the next character that will be read by a call to peekCharacter
    // This will be the 'current column' after a call to readCharacter.
    wchar_t peekInputColumn() {
        if(prevNewLine) {
            return 0;
        } else {
            return inputCol+1;
        }
    }

    void skipInputWhitespace() {
        wchar_t cchar = peekCharacter();
        while(cchar == ' ' || cchar == '\t') {
            readCharacter();
            cchar = peekCharacter();
        }
    }
    
    void outputIndent() {
        int indentSize = indentLevelStack.empty() ? 0 : indentLevelStack.back();
        outputCol += indentSize;
        
        while(indentSize--) {
            output.push_back(L' ');
        }
    }
    
    void copyAndEnsureNewLine() {
        while(inputOffset < endOffset) {
            wchar_t curChar = peekCharacter();
            
            if(curChar == L'\n') {
                outputCharacter(L'\n');
                readCharacter();
                skipInputWhitespace();
                return;
            } else
            if(isspace(curChar)) {
                readCharacter();
            } else {
                break;
            }
        }
        
        outputCharacter(L'\n');
    }

    void outputCharacter(wchar_t character) {
        if(!outputCol) {
            outputIndent();
        }
        
        output.push_back(character);
        
        outputCol++;
        
        if(character == L'\n') {
            outputCol = 0;
        }
    }
private:
    const std::wstring &jsonText;
    TextCoordinate startOffset;
    TextCoordinate endOffset;
    
    std::vector<int> indentLevelStack;
    TextCoordinate inputOffset;
    TextCoordinate inputCol;

    TextCoordinate outputCol;
    
    bool prevNewLine;
    bool inEscape;
    bool inString;
    
    std::wstring output;
};

-(void)reindentStartingAt:(TextCoordinate)aOffsetStart len:(TextLength)aLen {
    std::wstring jsonText = _textStorage.string.cStringWstring;
    JsonIndentationFixer fixer(jsonText, aOffsetStart, aLen);
    const std::wstring &indent = fixer.getIndented();
        
    [_textStorage replaceCharactersInRange:NSMakeRange(aOffsetStart, aLen) withString:[NSString stringWithWstring:indent]];
}



-(void)updateSyntaxInRange:(NSRange)editedRange {
    NSData *lFontData =[[NSUserDefaults standardUserDefaults] valueForKey:@"TextEditorFont"];
    NSFont *lFont = [NSUnarchiver unarchiveObjectWithData:lFontData];
       
    [self.textStorage beginEditing];

    [self.textStorage setAttributeRuns:[NSArray array]];

    [self.textStorage setFont:lFont];

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
            if(modelRefreshTag == _lastRequestedSemanticModelRefresh) {
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


@end
