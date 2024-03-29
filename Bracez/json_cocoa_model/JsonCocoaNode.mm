//
//  JsonCocoaNode.m
//  JsonMockup
//
//  Created by Eldan on 6/5/10.
//  Copyright 2010 Eldan Ben-Haim. All rights reserved.
//

#import "JsonCocoaNode.h"
#include "reader.h"
#include "NSString+WStringUtils.h"

#include <codecvt>
#include <locale>

using convert_type = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_type, wchar_t> wide_utf8_converter;


using namespace json;

static int computeChildCount(json::Node *element) {
    if(element &&
       (element->getNodeTypeId() == ntObject ||
        element->getNodeTypeId() == ntArray))
    {
       ContainerNode *lContainer = (ContainerNode*)element;
       return lContainer->getChildCount();
    } else {
        return 0;
   }
}

@implementation JsonCocoaNode


+(JsonCocoaNode*) nodeForElement:(json::Node*)aProxiedElement withName:(NSString*)aName
{
    if(!aProxiedElement) {
        return nil;
    }
    
   return [[JsonCocoaNode alloc] initWithName:aName element:aProxiedElement];
}

-(id)initWithName:(NSString*)aName element:(json::Node*)aProxiedElement
{
   self = [super init];
   
   if(self)
   {
      name = aName;
      proxiedElement = aProxiedElement;
      childCount = computeChildCount(proxiedElement);
   }
   
   return self;
}

-(void)setNodeValue:(NSValue*)aValue
{
   NSString *lValString = [aValue description];
   
   int lIdx = proxiedElement->getParent()->getIndexOfChild(proxiedElement);
   
   Node *lNewNode;
   if([lValString isEqualToString:@"true"])
   {
      lNewNode = new BooleanNode(true);
   } else
   if([lValString isEqualToString:@"false"])
   {
      lNewNode = new BooleanNode(false);
   } else
   if([lValString isEqualToString:@"null"])
   {
      lNewNode = new NullNode();
   } else
   if([lValString characterAtIndex:0]=='"')
   {
       
      StringNode *lNode;
      Reader::Read(lNode, lValString.cStringWstring);
      
      lNewNode = lNode;
   } else
   {
      std::wstring str = lValString.cStringWstring;
      const wchar_t *cstr = str.c_str();
      const wchar_t *cstr_end;
      double lDoubleVal = wcstod(cstr, (wchar_t**)&cstr_end);
      
      if(!*cstr_end)
      {
         lNewNode = new NumberNode(lDoubleVal);
      } else
      {
         lNewNode = new StringNode(cstr);
      }
   }
   
   proxiedElement->getParent()->setChildAt(lIdx, lNewNode);
}

-(id) nodeValue
{
   if(!proxiedElement)
      return nil;
      
   switch(proxiedElement->getNodeTypeId())
   {
      case ntNull:
         return @"null";

      case ntBoolean:
         return ((BooleanNode*)proxiedElement)->getValue()?@"true":@"false";

      case ntObject:
         return @"[object]";

      case ntArray:
         return @"[array]";

      default:
         {
            std::wstring lTxt;
            proxiedElement->calculateJsonTextRepresentation(lTxt);
             return [NSString stringWithWstring:lTxt];
         }
   }
}

-(NSString*) longNodeValue
{
    std::wstring lTxt;
    proxiedElement->calculateJsonTextRepresentation(lTxt, 255);
    return [NSString stringWithWstring:lTxt];
}

-(int)nodeType
{
   return proxiedElement->getNodeTypeId();
}

-(json::Node*)proxiedElement
{
   return proxiedElement;
}


-(NSString*) nodeName
{
   return name;
}

-(void) setName:(NSString*)nodeName
{
    json::ObjectNode *objContainer = dynamic_cast<json::ObjectNode *>(proxiedElement->getParent());
    if(objContainer) {
        int indexInParent = objContainer->getIndexOfChild(proxiedElement);
        objContainer->renameMemberAt(indexInParent, wide_utf8_converter.from_bytes(nodeName.UTF8String));
        name = nodeName;
    }
}

-(BOOL) nameEditable {
    return dynamic_cast<json::ObjectNode *>(proxiedElement->getParent()) != NULL;
}

-(int)countOfChildren
{
    return childCount;
}

-(JsonCocoaNode*)objectInChildrenAtIndex:(int)aIdx
{
   [self _prepChildren];   
   return [children objectAtIndex:aIdx];
}

-(void)removeObjectFromChildrenAtIndex:(int)aIdx
{
   json::ContainerNode *lContainerNode = dynamic_cast<json::ContainerNode*>(proxiedElement);
   if(lContainerNode)
   {
      lContainerNode->removeChildAt(aIdx);
      //[self _internalRemoveChildAtIndex:aIdx];
   }
}

-(void)moveToNode:(JsonCocoaNode*)aNewContainer
          atIndex:(int)aIndex
       fromParent:(JsonCocoaNode*)aParent;
{
   ContainerNode *lParentNode = proxiedElement->getParent();
   int lIdxInParent = lParentNode->getIndexOfChild(proxiedElement);
   
   // Adjust underlying document: (a) detach child from current position
   wstring lTxt = proxiedElement->getDocumentText();
   Node *lThis;
   lParentNode->detachChildAt(lIdxInParent, &lThis);
   
   // (b) Import to new position:
   Node *lNewParent = [aNewContainer proxiedElement];
   
   // Fixup index if we're sliding an entry of the same object.
   if(lNewParent == lParentNode && aIndex > lIdxInParent)
   {
      aIndex--;
   }
   
   // (b.1) New container is an object?
   ObjectNode *lObjNode = dynamic_cast<ObjectNode*> (lNewParent);
   if(lObjNode)
   {
      lObjNode->insertMemberAt(aIndex, name.cStringWstring.c_str(), lThis, &lTxt);
   } else 
   {
      // (b.2) New container is an array?
      ArrayNode *lArrNode = dynamic_cast<ArrayNode*> (lNewParent);
      lArrNode->InsertMemberAt(aIndex, lThis, &lTxt);      
   }
     
}

-(void)_internalInsertChild:(JsonCocoaNode*)aChild atIndex:(int)aIdx
{
   [self willChangeValueForKey:@"children"];
   
   json::ContainerNode *lContainerNode = dynamic_cast<json::ContainerNode*>(proxiedElement);
   
   [children insertObject:aChild atIndex:aIdx];
   
   if(lContainerNode->getNodeTypeId() == ntArray)
   {
      for(int lIdx=aIdx; lIdx<[self countOfChildren]; lIdx++)
      {
         JsonCocoaNode *lNode = [self objectInChildrenAtIndex:lIdx];
         [lNode _updateNameFromIndex:lIdx];
      }
   }
   
   [self didChangeValueForKey:@"children"];
}

-(void)_internalRemoveChildAtIndex:(int)aIdx
{
   [self willChangeValueForKey:@"children"];

   json::ContainerNode *lContainerNode = dynamic_cast<json::ContainerNode*>(proxiedElement);

   [children removeObjectAtIndex:aIdx];

   if(lContainerNode->getNodeTypeId() == ntArray)
   {
      for(int lIdx=aIdx; lIdx<[self countOfChildren]; lIdx++)
      {
         JsonCocoaNode *lNode = [self objectInChildrenAtIndex:lIdx];
         [lNode _updateNameFromIndex:lIdx];
      }
   }

   [self didChangeValueForKey:@"children"];
}

-(void)insertObject:(id)aObject inChildrenAtIndex:(int)aIdx
{
}

-(void)replaceObjectInChildrenAtIndex:(int)aIdx withObject:(id)aObject
{
}

-(BOOL) isContainer
{
   return dynamic_cast<ContainerNode*>(proxiedElement)!=NULL;
}

-(void)_updateNameFromIndex:(int)aIdx
{
   [self willChangeValueForKey:@"nodeName"];
   name = [NSString stringWithFormat:@"%d", aIdx];
   [self didChangeValueForKey:@"nodeName"];
}

-(void)_prepChildren
{
   if(children)
   {
      return;
   }
   
   NSMutableArray *lArray = [NSMutableArray array]; 
   
   if(proxiedElement)
   {
      switch(proxiedElement->getNodeTypeId())
      {
         case ntObject:
         {
            ObjectNode *lObject = (ObjectNode *)proxiedElement;
            for(ObjectNode::iterator lIter = lObject->begin();
               lIter != lObject->end();
               lIter++)
            {
                NSString *nodeName = [NSString stringWithWstring:lIter->name];

               [lArray addObject:[JsonCocoaNode nodeForElement:lIter->node.get()
                                                      withName:nodeName]];
            }         
            break;
         }
           
         case ntArray:
         {
            ArrayNode *lJsonArray = (ArrayNode*)proxiedElement;
            int lIdx = 0;
            for(ArrayNode::iterator lIter = lJsonArray->begin();
               lIter != lJsonArray->end();
               lIter++, lIdx++)
            {
               [lArray addObject:[JsonCocoaNode nodeForElement:lIter->get() withName:[NSString stringWithFormat:@"%d", lIdx]]];
            }         
            break;
         }  
         default:
            lArray = nil;
            break;
      }
   }
   children = lArray;
}

-(json::TextRange)textRange
{
   return proxiedElement->getAbsTextRange();
}

-(int)indexOfChildWithName:(NSString*)aName
{
   [self _prepChildren];
   
   int lRet = 0;
   
   NSEnumerator *lEnum = [children objectEnumerator];
   while(JsonCocoaNode *lChildNode = [lEnum nextObject])
   {
      if([[lChildNode nodeName] isEqual:aName])
      {
         return lRet;
      }
      lRet++;
   }
   
   return -1;
}

-(void)reloadFromElement:(json::Node*)aProxiedElement {
    int newChildCount = computeChildCount(aProxiedElement);
    
    bool childrenChanging =
        (children != nil) ||
        ((self.countOfChildren == 0) != (newChildCount == 0));
    
    if(childrenChanging) {
        [self willChangeValueForKey:@"children"];
    }
    
    [self willChangeValueForKey:@"nodeName"];
    [self willChangeValueForKey:@"nodeType"];
    [self willChangeValueForKey:@"nodeValue"];
    
    proxiedElement = aProxiedElement;
    children = nil;
    childCount = computeChildCount(proxiedElement);
    
    [self didChangeValueForKey:@"nodeValue"];
    [self didChangeValueForKey:@"nodeType"];
    [self didChangeValueForKey:@"nodeName"];
    
    if(childrenChanging) {
        [self didChangeValueForKey:@"children"];
    }
}

-(void)reloadChildren {
    if(children != nil) {
        [self willChangeValueForKey:@"children"];
        children = nil;
        [self didChangeValueForKey:@"children"];
    }
}

@end

