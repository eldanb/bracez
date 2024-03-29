//
//  SyntaxHilightJsonVisitor.m
//  Bracez
//
//  Created by Eldan Ben Haim on 16/10/2020.
//

#import "SyntaxHilightJsonVisitor.h"

SyntaxHighlightJsonVisitor::SyntaxHighlightJsonVisitor(NSMutableAttributedString *aTextStorage, const NSRange &aHighlightRange) : textStorage(aTextStorage), hilightRange(aHighlightRange)
{
  loadColors();
}
   
bool SyntaxHighlightJsonVisitor::visitNode(Node *aNode)
{
  return visitNode((const Node *)aNode);
}

bool SyntaxHighlightJsonVisitor::visitObjectMemberNode(ObjectNode::Member *aMember)
{
  return visitObjectMemberNode((const ObjectNode::Member *)aMember);
}
   
bool SyntaxHighlightJsonVisitor::visitNode(const Node *aNode)
{
  if(dynamic_cast<const ObjectNode*>(aNode))
  {
     unsigned long lOfs = aNode->getAbsTextRange().start.getAddress();
     
     const ObjectNode *lObjNode = dynamic_cast<const ObjectNode*>(aNode);
     int lChildCount = lObjNode->getChildCount();
     
     for(int lChildIdx = 0; lChildIdx<lChildCount; lChildIdx++)
     {
        const ObjectNode::Member *lMember = lObjNode->getChildMemberAt(lChildIdx);
        NSRange lKeyRange = NSIntersectionRange(NSMakeRange((lMember->nameRange.start+lOfs).getAddress(), lMember->nameRange.length()), hilightRange);
        if(lKeyRange.length>0)
        {
           [textStorage addAttribute:NSForegroundColorAttributeName value:[colors keyColor] range:lKeyRange];
        }
     }
  } else {
     NSColor *lNodeColor = [colors colorForNodeType:aNode->getNodeTypeId()];
     
      if(lNodeColor) {
         json::TextRange lNodeRange = aNode->getAbsTextRange();
          if(lNodeRange.end.infinite()) {
              lNodeRange.end = TextCoordinate((unsigned int)(hilightRange.location + hilightRange.length));
          }
         NSRange lRange = NSIntersectionRange(NSMakeRange(lNodeRange.start.getAddress(), lNodeRange.length()),
                                                hilightRange);
         if(lRange.length!=0)
         {
            [textStorage addAttribute:NSForegroundColorAttributeName value:lNodeColor range:lRange];
         }
      }
  }
  return true;
}

bool SyntaxHighlightJsonVisitor::visitObjectMemberNode(const ObjectNode::Member *aMember)
{
  return true;
}
   
void SyntaxHighlightJsonVisitor::loadColors()
{
  colors = (NodeTypeToColorTransformer*)[NSValueTransformer valueTransformerForName:@"NodeTypeToColorTransformer"];
}
