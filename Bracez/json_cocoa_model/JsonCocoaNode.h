//
//  JsonCocoaNode.h
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "json_file.h"

@interface JsonCocoaNode : NSObject {

   json::Node *proxiedElement;
   NSString *name;
   NSMutableArray *children;
}

+(JsonCocoaNode*) nodeForElement:(json::Node*)aProxiedElement withName:(NSString*)aName;

-(id)initWithName:(NSString*)aName element:(json::Node*)aProxiedElement;

-(int)nodeType;
-(id) nodeValue;

-(NSString*) nodeName;
-(void) setName:(NSString*)nodeName;
-(BOOL) nameEditable;

-(BOOL) isContainer;

-(json::Node*)proxiedElement;

-(void)setNodeValue:(NSValue*)aValue;

-(int)countOfChildren;
-(JsonCocoaNode*)objectInChildrenAtIndex:(int)aIdx;
-(void)removeObjectFromChildrenAtIndex:(int)aIdx;
-(void)insertObject:(id)aObject inChildrenAtIndex:(int)aIdx;
-(void)replaceObjectInChildrenAtIndex:(int)aIdx withObject:(id)aObject;

-(int)indexOfChildWithName:(NSString*)aName;

-(void)moveToNode:(JsonCocoaNode*)aNewContainer atIndex:(int)aIndex fromParent:(JsonCocoaNode*)aParent;

-(json::TextRange)textRange;

-(void)_prepChildren;
-(void)_updateNameFromIndex:(int)aIdx;
-(void)_internalRemoveChildAtIndex:(int)aIdx;
-(void)_internalInsertChild:(JsonCocoaNode*)aChild atIndex:(int)aIdx;


-(void)reloadChildren;
@end
