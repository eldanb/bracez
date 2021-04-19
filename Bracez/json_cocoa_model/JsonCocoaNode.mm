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
   }
   
   return self;
}

-(void)setNodeValue:(NSValue*)aValue
{
   [self willChangeValueForKey:@"nodeType"];
   
   NSString *lValString = [aValue description];
   
   int lIdx = proxiedElement->GetParent()->GetIndexOfChild(proxiedElement);
   
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
       
      wstringstream str(lValString.cStringWstring);
      StringNode *lNode;
      Reader::Read(lNode, str);
      
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
   
   proxiedElement->GetParent()->SetChildAt(lIdx, lNewNode);
   proxiedElement = lNewNode;
   
   [self didChangeValueForKey:@"nodeType"];
}

-(id) nodeValue
{
   if(!proxiedElement)
      return nil;
      
   switch(proxiedElement->GetNodeTypeId())
   {
      case ntNull:
         return @"null";

      case ntBoolean:
         return ((BooleanNode*)proxiedElement)->GetValue()?@"true":@"false";

      case ntObject:
         return @"[object]";

      case ntArray:
         return @"[array]";

      default:
         {
            std::wstring lTxt;
            proxiedElement->CalculateJsonTextRepresentation(lTxt);
             return [NSString stringWithWstring:lTxt];
         }
   }
}

-(int)nodeType
{
   return proxiedElement->GetNodeTypeId();
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
    json::ObjectNode *objContainer = dynamic_cast<json::ObjectNode *>(proxiedElement->GetParent());
    if(objContainer) {
        int indexInParent = objContainer->GetIndexOfChild(proxiedElement);
        objContainer->RenameMemberAt(indexInParent, wide_utf8_converter.from_bytes(nodeName.UTF8String));
        name = nodeName;
    }
}

-(BOOL) nameEditable {
    return dynamic_cast<json::ObjectNode *>(proxiedElement->GetParent()) != NULL;
}

-(int)countOfChildren
{
   [self _prepChildren];   
   return (int)[children count];
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
      lContainerNode->RemoveChildAt(aIdx);
      [self _internalRemoveChildAtIndex:aIdx];
   }
}

-(void)moveToNode:(JsonCocoaNode*)aNewContainer
          atIndex:(int)aIndex
       fromParent:(JsonCocoaNode*)aParent;
{
   ContainerNode *lParentNode = proxiedElement->GetParent();
   int lIdxInParent = lParentNode->GetIndexOfChild(proxiedElement);
   
   // Adjust underlying document: (a) detach child from current position
   wstring lTxt = proxiedElement->GetDocumentText();
   Node *lThis;
   lParentNode->DetachChildAt(lIdxInParent, &lThis);
   
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
      lObjNode->InsertMemberAt(aIndex, name.cStringWstring.c_str(), lThis, &lTxt);
   } else 
   {
      // (b.2) New container is an array?
      ArrayNode *lArrNode = dynamic_cast<ArrayNode*> (lNewParent);
      lArrNode->InsertMemberAt(aIndex, lThis, &lTxt);      
   }
   
   [aParent _internalRemoveChildAtIndex:lIdxInParent];
   [aNewContainer _internalInsertChild:[JsonCocoaNode nodeForElement:lThis withName:name] atIndex:aIndex];
}

-(void)_internalInsertChild:(JsonCocoaNode*)aChild atIndex:(int)aIdx
{
   [self willChangeValueForKey:@"children"];
   
   json::ContainerNode *lContainerNode = dynamic_cast<json::ContainerNode*>(proxiedElement);
   
   [children insertObject:aChild atIndex:aIdx];
   
   if(lContainerNode->GetNodeTypeId() == ntArray)
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

   if(lContainerNode->GetNodeTypeId() == ntArray)
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
      switch(proxiedElement->GetNodeTypeId())
      {
         case ntObject:
         {
            ObjectNode *lObject = (ObjectNode *)proxiedElement;
            for(ObjectNode::iterator lIter = lObject->Begin();
               lIter != lObject->End();
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
            for(ArrayNode::iterator lIter = lJsonArray->Begin();
               lIter != lJsonArray->End();
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
   return proxiedElement->GetAbsTextRange();
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
    [self willChangeValueForKey:@"children"];
    [self willChangeValueForKey:@"nodeName"];
    [self willChangeValueForKey:@"nodeType"];
    [self willChangeValueForKey:@"nodeValue"];
    
    proxiedElement = aProxiedElement;
    children = nil;
    
    [self didChangeValueForKey:@"nodeValue"];
    [self didChangeValueForKey:@"nodeType"];
    [self didChangeValueForKey:@"nodeName"];
    [self didChangeValueForKey:@"children"];
}

-(void)reloadChildren {
    [self willChangeValueForKey:@"children"];
    children = nil;
    [self didChangeValueForKey:@"children"];
}

@end
