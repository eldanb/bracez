//
//  JSONWindow.m
//
//  Created by Eldan on 7/5/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "JSONWindow.h"
#import "TextEditorGutterView.h"
#import "JsonTreeDataSource.h"
#import "NodeTypeToColorTransformer.h"
#import "JsonDocument.h"

@interface JSONWindow () {
    JsonTreeDataSource *_treeDataSource;
}

@end

@implementation JSONWindow
 
+(void)initialize
{
   [super initialize];
   [NSValueTransformer setValueTransformer:[[NodeTypeToColorTransformer alloc] initWithEnableKey:@"TreeViewSyntaxColoring"] forName:@"treeViewNodeColors"];
}

-(void)awakeFromNib
{                                               
    [textEditor setHorizontallyResizable:YES];
    [textEditor setAutoresizingMask:(NSViewWidthSizable|NSViewHeightSizable)];
    [[textEditor textContainer] setWidthTracksTextView:NO];
    [[textEditor textContainer] setContainerSize:NSMakeSize(MAXFLOAT, MAXFLOAT)];
    textEditor.automaticQuoteSubstitutionEnabled = NO;
    textEditor.enabledTextCheckingTypes = 0;
    [textEditor setMaxSize:NSMakeSize(MAXFLOAT, MAXFLOAT)];
    
    [textEditor.layoutManager replaceTextStorage:((JsonDocument*)self.delegate).textStorage];
    
   gutterView = [[TextEditorGutterView alloc] initWithScrollView:textEditorScroll];
   [gutterView setModel:(id<GutterViewModel>)selectionController];
   [textEditorScroll setVerticalRulerView:gutterView];

    _treeDataSource = [[JsonTreeDataSource alloc] init];
   [treeView setDataSource:_treeDataSource];
   [treeView registerForDraggedTypes:[NSArray arrayWithObject:@"JsonNode"]];
    [treeView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
   
   [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(defaultsChanged:) name:NSUserDefaultsDidChangeNotification object:nil];

   [self loadDefaults];
}


-(void)defaultsChanged:(NSNotification*)aNotification
{
   [self loadDefaults];
}

-(void)loadDefaults
{
   NSUserDefaults *lDefaults = [NSUserDefaults standardUserDefaults];

   [textEditorScroll setRulersVisible:[lDefaults boolForKey:@"GutterMaster"]];
   [gutterView setShowLineNumbers:[lDefaults boolForKey:@"GutterLineNumbers"]];
   [treeView setNeedsDisplay];
    
   NSData *lFontData =[[NSUserDefaults standardUserDefaults] valueForKey:@"TextEditorFont"];
   NSFont *lFont = [NSUnarchiver unarchiveObjectWithData:lFontData];
   textEditor.font = lFont;
}

-(void)removeJsonNode:(id)aSender
{
   [domController remove:aSender];
}


struct ForwardedActionInfo
{
   const char *selectorString;
   enum {
      faiDomController,
      faiSelectionController,
      faiSelf } forwardee;
   SEL entSelector;

} ;


struct ForwardedActionInfo glbForwardedActions[]  =  {
    { "toggleBookmark:", ForwardedActionInfo::faiSelectionController, 0  },
    { "nextBookmark:", ForwardedActionInfo::faiSelectionController, 0  },
    { "prevBookmark:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goFirstChild:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goNextSibling:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goPrevSibling:", ForwardedActionInfo::faiSelectionController, 0  },
    { "goParentNode:", ForwardedActionInfo::faiSelectionController, 0  },

    { "copyPath:", ForwardedActionInfo::faiSelectionController, 0  },

    { "navigateBack:", ForwardedActionInfo::faiSelectionController, 0  },
    { "navigateForward:", ForwardedActionInfo::faiSelectionController, 0  },

    { NULL, ForwardedActionInfo::faiSelf, 0 }
};

-(id)_getActionForwardingTarget:(SEL)aSelector
{
   int lIdx=0;
   while(glbForwardedActions[lIdx].selectorString)
   {
      if(!glbForwardedActions[lIdx].entSelector)
      {
         glbForwardedActions[lIdx].entSelector = NSSelectorFromString([NSString stringWithCString:glbForwardedActions[lIdx].selectorString encoding:NSUTF8StringEncoding]);
      }
      
      if(glbForwardedActions[lIdx].entSelector == aSelector)
      {
         break;
      }
      
      lIdx++;
   }

   
   switch(glbForwardedActions[lIdx].forwardee)
   {
       case ForwardedActionInfo::faiDomController:
         return domController;
       case ForwardedActionInfo::faiSelectionController:
         return selectionController;
      default:
         return self;
   }
      
   return self;
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
   id lResponder = [self _getActionForwardingTarget:aSelector];
   
   if(lResponder != self)
   {
      BOOL lRet= [lResponder respondsToSelector:aSelector];
      return lRet;
   } else {
      return [super respondsToSelector:aSelector];
   }
}

- (NSMethodSignature *)	methodSignatureForSelector:(SEL) aSelector
{
   id lResponder = [self _getActionForwardingTarget:aSelector];
   
   if(lResponder != self)
   {
      return [lResponder methodSignatureForSelector:aSelector];
   }
   else
   {
      return [super methodSignatureForSelector:aSelector];
   }
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
   id lResponder = [self _getActionForwardingTarget:[anInvocation selector]];

   if(lResponder != self)
   {
      [anInvocation invokeWithTarget:lResponder];
   }
   else
   {
      [super forwardInvocation:anInvocation];
   }
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
   id lResponder = [self _getActionForwardingTarget:[anItem action]];

   if(lResponder != self)
   {
      return [lResponder validateUserInterfaceItem:anItem];
   } else {
      return YES;
   }
}

-(void)indentSelectionAction:(id)sender {
    NSRange selectionRange = textEditor.selectedRange;
    if(selectionRange.length <= 0) {
        selectionRange = NSMakeRange(0, textEditor.textStorage.length);
    }
    
    [((JsonDocument*)self.delegate) reindentStartingAt:selectionRange.location len:selectionRange.length];
}
@end
