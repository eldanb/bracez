/*
 *  json_file.cpp
 *  JsonMockup
 *
 *  Created by Eldan on 6/5/10.
 *  Copyright 2010 Eldan Ben-Haim. All rights reserved.
 *
 */

#include "json_file.h"

#include <CoreServices/CoreServices.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "reader.h"

#include "stopwatch.h"

namespace json 
{
   
static string jsonize(const string &aSrc);

   
namespace priv
{
   class SpliceNotification : public Notification   
   {
   public:
      SpliceNotification(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen) : 
          offsetStart(aOffsetStart), len(aLen), newLen(aNewLen) {}
      
      SpliceNotification(const SpliceNotification &aOther) : 
         offsetStart(aOther.offsetStart), len(aOther.len), newLen(aOther.newLen) {}
      
      SpliceNotification *clone() const
      {
         return new SpliceNotification(*this);
      }
      
      void deliverToListener(JsonFile *aSender, JsonFileChangeListener *aListener) const
      {
          aListener->notifyTextSpliced(aSender, offsetStart, len, newLen);
      }
      
   private:
      TextCoordinate offsetStart;
      TextLength len;
      TextLength newLen;
   }  ;

   class ErrorsChangedNotification : public Notification   
   {
   public:
      ErrorsChangedNotification() {}
      ErrorsChangedNotification(const ErrorsChangedNotification &aOther) {}
      
      ErrorsChangedNotification *clone() const
      {
         return new ErrorsChangedNotification(*this);
      }
      
      void deliverToListener(JsonFile *aSender, JsonFileChangeListener *aListener) const
      {
         aListener->notifyErrorsChanged(aSender);
      }
   } ;  
   
   
   class DeferNotificationsInBlock
   {
   public:
      DeferNotificationsInBlock(JsonFile *aWho) : who(aWho) { who->beginDeferNotifications(); }
      ~DeferNotificationsInBlock() { who->endDeferNotifications(); }
      
   private:
      JsonFile *who;
   } ;
}
   
using namespace priv;

DocumentNode *Node::GetDocument() const
{
   const Node *lCurNode = this;
   while(lCurNode->GetParent())
   {  
      lCurNode = lCurNode->GetParent();
   }
   
   return const_cast<DocumentNode*>(dynamic_cast<const DocumentNode*>(lCurNode));
}
      
const TextRange &Node::GetTextRange() const
{
   return textRange;
}
   
   
string Node::GetDocumentText() const
{
   TextRange lAbsRange = GetAbsTextRange();
   return GetDocument()->GetOwner()->getText().substr(lAbsRange.start, lAbsRange.length());
}


TextRange Node::GetAbsTextRange() const
{
   int lOfs = 0;
   const Node *lCurNode = GetParent();
   while(lCurNode)
   {
      lOfs += lCurNode->GetTextRange().start;
      lCurNode = lCurNode->GetParent();
   }
   
   return TextRange(textRange.start+lOfs, textRange.end+lOfs);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int ContainerNode::FindChildContaining(const TextCoordinate &aDocOffset) const
{
   int lRet = FindChildEndingAfter(aDocOffset);
   if(lRet>=0 && GetChildAt(lRet)->GetTextRange().start<=aDocOffset)
   {
      return lRet;
   } else
   {
      return -1;
   }
}

int ContainerNode::GetIndexOfChild(Node *aChild)
{
   int lChildCount = GetChildCount();
   for(int lIdx = 0; lIdx<lChildCount; lIdx++)
   {
      if(GetChildAt(lIdx) == aChild)
      {
         return lIdx;
      }
   }
   
   return -1;
}

void ContainerNode::RemoveChildAt(int aIdx)
{
   Node *lChild;
   DetachChildAt(aIdx, &lChild);
   
   delete lChild;
}
      

void ContainerNode::AdjustChildRangeAt(int aIdx, int aDiff)
{
   Node *lNode = GetChildAt(aIdx);
      
   lNode->textRange.end += aDiff;
   lNode->textRange.start += aDiff;   
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ArrayNode::iterator ArrayNode::Begin()
{
   return elements.begin();
}

ArrayNode::iterator ArrayNode::End()
{
   return elements.end();
}

ArrayNode::const_iterator ArrayNode::Begin() const
{
   return elements.begin();
}

ArrayNode::const_iterator ArrayNode::End() const
{
   return elements.end();
}

int ArrayNode::GetChildCount() const
{
   return elements.size();
}

Node *ArrayNode::GetChildAt(int aIdx)
{
   return elements[aIdx];
}

const Node *ArrayNode::GetChildAt(int aIdx) const
{
   return elements[aIdx];
}

void ArrayNode::SetChildAt(int aIdx, Node *aNode)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());
   
   // Get old node 
   Node *lOldNode = elements[aIdx];

   // Get new node text and range
   std::string lNewNodeText;
   aNode->CalculateJsonTextRepresentation(lNewNodeText);

   int lNewStart = lOldNode->textRange.start;
   int lNewEnd = lNewStart + lNewNodeText.length();
      
   // Splice text in document
   TextRange lAbsTextRange = lOldNode->GetAbsTextRange();
   GetDocument()->GetOwner()->spliceJsonTextByDomChange(lAbsTextRange.start, lAbsTextRange.length(), lNewNodeText);
   
   // Update in child list and import to document
   elements[aIdx] = aNode;
   aNode->parent = this;
   aNode->textRange.start = lNewStart;
   aNode->textRange.end = lNewEnd;   
}

void ArrayNode::DetachChildAt(int aIdx, Node **aNode)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());

   Elements::iterator lIter = elements.begin() + aIdx;
   Elements::iterator lNextIter = lIter+1;
   
   // Splice text in document   
   TextRange lSpliceRange = (*lIter)->GetAbsTextRange();
   if(lNextIter!=elements.end())
   {
      lSpliceRange.end = (*lNextIter)->GetAbsTextRange().start;
   } else
   {
      lSpliceRange.end = GetAbsTextRange().end-1;
   }

   *aNode = *lIter;
   (*aNode)->parent = NULL;
   
   elements.erase(lIter);   
   
   GetDocument()->GetOwner()->spliceJsonTextByDomChange(lSpliceRange.start, lSpliceRange.length(), string(""));
}
   
void ArrayNode::InsertMemberAt(int aIdx, Node *aElement, const string *aElementText)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());

   int lElemAddr;
   int lChangeAddr;
   
   bool lIsLast;
   
   // Append or insert member at members list, devise text offset for member.
   Elements::iterator lIter;
   if(aIdx < 0 || aIdx >= elements.size())
   {
      aIdx = elements.size();
      lElemAddr = textRange.length()-1;
      lIter = elements.end();
      lIsLast = true;
   } else {
      lIter = elements.begin()+aIdx;
      lElemAddr = (*lIter)->textRange.start;
      lIsLast = false;
   }
   
   lChangeAddr = lElemAddr;
   
   // Setup member  parent
   aElement->parent = this;
   
   // Calculate element text: start with name
   string lCalculatedText;   
   if(lIsLast && aIdx)
   {
      lCalculatedText = ", ";
      lElemAddr+=2;
   }
  
   // Next -- element text
   int lElementTextLen;
   if(aElementText)
   {
      lCalculatedText += *aElementText;
      lElementTextLen = aElementText->length();
   } else {
      string lTxt;
      aElement->CalculateJsonTextRepresentation(lTxt);
      lElementTextLen = lTxt.length();
      lCalculatedText += lTxt;
   }
   
   // Add comma if not last element
   if(!lIsLast)
   {
      lCalculatedText += ", ";
   } 
   
   // Update document text
   GetDocument()->GetOwner()->spliceJsonTextByDomChange(GetAbsTextRange().start+lChangeAddr, 0, lCalculatedText);
   
   // Add element to sequence and setup address
   elements.insert(lIter, aElement);
   
   aElement->textRange.start = lElemAddr;
   aElement->textRange.end = lElemAddr + lElementTextLen;  
}
   
struct ArrayMemberTextRangeOrdering 
{
   bool operator()(const Node *&aLeft, const Node *&aRight) const
   {
      return aLeft->GetTextRange().end < aRight->GetTextRange().end;
   }

   bool operator() (Node * const&aLeft, unsigned int aRight) const
   {
      return aLeft->GetTextRange().end < aRight;
   }
};


int ArrayNode::FindChildEndingAfter(const TextCoordinate &aDocOffset) const
{
   const_iterator lContainingElement = lower_bound(elements.begin(), elements.end(), aDocOffset, ArrayMemberTextRangeOrdering());
   
   // Found member ending past offset?
   if(lContainingElement == elements.end()) 
      return -1;
         
   return lContainingElement - elements.begin();
}


void ArrayNode::DomAddElementNode(Node *aElement)
{
   elements.push_back(aElement);
   aElement->parent = this;
}

NodeTypeId ArrayNode::GetNodeTypeId() const
{
   return ntArray;
}

void ArrayNode::CalculateJsonTextRepresentation(std::string &aDest) const
{
   // TODO
}

void ArrayNode::accept(NodeVisitor *aVisitor) const
{
   aVisitor->visitNode(this);
   for(Elements::const_iterator lIter = elements.begin(); lIter!=elements.end(); lIter++)
   {
      (*lIter)->accept(aVisitor);
   }
}

void ArrayNode::accept(NodeVisitor *aVisitor)
{
   aVisitor->visitNode(this);
   for(Elements::iterator lIter = elements.begin(); lIter!=elements.end(); lIter++)
   {
      (*lIter)->accept(aVisitor);
   }   
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode::iterator ObjectNode::Begin()
{
   return members.begin();
}

ObjectNode::iterator ObjectNode::End()
{
   return members.end();
}
      
ObjectNode::const_iterator ObjectNode::Begin() const
{
   return members.begin();
}

ObjectNode::const_iterator ObjectNode::End() const
{
   return members.end();
}

int ObjectNode::GetChildCount() const
{
   return members.size();
}
      
Node *ObjectNode::GetChildAt(int aIdx)
{
   return members[aIdx].node;
}

const Node *ObjectNode::GetChildAt(int aIdx) const
{
   return members[aIdx].node;
}
 
int ObjectNode::GetIndexOfMemberWithName(const string &name) const {
    for(Members::const_iterator iter = members.begin();
        iter != members.end();
        iter++) {
        if(iter->name == name) {
            return iter - members.begin();
        }
    }
    
    return -1;
}

const ObjectNode::Member *ObjectNode::GetChildMemberAt(int aIdx) const
{
   return &members[aIdx];
}

    
void ObjectNode::SetChildAt(int aIdx, Node *aNode)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());

   // Get old node 
   Node *lOldNode = members[aIdx].node;

   
   // Get new node text and range
   std::string lNewNodeText;
   aNode->CalculateJsonTextRepresentation(lNewNodeText);
   
   int lNewStart = lOldNode->textRange.start;
   int lNewEnd = lNewStart + lNewNodeText.length();
   
   // Splice text in document
   TextRange lAbsTextRange = lOldNode->GetAbsTextRange();
   GetDocument()->GetOwner()->spliceJsonTextByDomChange(lAbsTextRange.start, lAbsTextRange.length(), lNewNodeText);
   
   // Update in child list and import to document
   members[aIdx].node = aNode;
   aNode->parent = this;
   aNode->textRange.start = lNewStart;
   aNode->textRange.end = lNewEnd;   
   
   // Delete old node
   delete lOldNode;
}

void ObjectNode::AdjustChildRangeAt(int aIdx, int aDiff)
{
   Member &lMember = members[aIdx];
   
   lMember.node->textRange.end += aDiff;
   lMember.node->textRange.start += aDiff;
   
   lMember.nameRange.start += aDiff;
   lMember.nameRange.end += aDiff;
}

struct ObjectMemberTextRangeOrdering 
{
   bool operator()(const ObjectNode::Member &aLeft, const ObjectNode::Member &aRight) const
   {
      return aLeft.node->GetTextRange().end < aRight.node->GetTextRange().end;
   }

   bool operator() (const ObjectNode::Member &aLeft, unsigned int aRight) const
   {
      return aLeft.node->GetTextRange().end < aRight;
   }
};

int ObjectNode::FindChildEndingAfter(const TextCoordinate &aDocOffset) const
{
   const_iterator lContainingElement = lower_bound(members.begin(), members.end(), aDocOffset, ObjectMemberTextRangeOrdering());
   
   // Found member ending past offset?
   if(lContainingElement == members.end()) 
      return -1;

//   printf("Child ending after %d is '%s\n", aDocOffset, lContainingElement->name.c_str());
   return lContainingElement - members.begin();
}

const string &ObjectNode::GetMemberNameAt(int aIdx) const
{
   return members[aIdx].name;
}
   
ObjectNode::Member &ObjectNode::InsertMemberAt(int aIdx, const string &aName, Node *aElement, const string *aElementText)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());

   int lNameAddr;
   int lChangeAddr;
   
   Member lMember(aName, aElement);
   bool lIsLast;

   // Append or insert member at members list, devise text offset for member.
   Members::iterator lIter;

   if(aIdx < 0 || aIdx >= members.size())
   {
      aIdx = members.size();
      lNameAddr = textRange.length()-1;
      lIter = members.end();
      lIsLast = true;
   } else {
      lIter = members.begin()+aIdx;
      lNameAddr = lIter->nameRange.start;
      lIsLast = false;
   }

   lChangeAddr = lNameAddr;

   // Setup member and parent
   lMember.name = aName;
   aElement->parent = this;
   
   // Calculate element text: start with name
   string lCalculatedText;
   int lNameLen;

   lCalculatedText = jsonize(lMember.name);
   lNameLen = lCalculatedText.length();
   
   if(lIsLast && aIdx)
   {
      lCalculatedText = ", " + lCalculatedText;
      lNameAddr+=2;
   }
   
   lCalculatedText += ": ";
   
   // (Update node start address to skip name)
   int lElemAddr = lChangeAddr + lCalculatedText.length();
   
   // Next -- element text
   int lElementTextLen;
   if(aElementText)
   {
      lCalculatedText += *aElementText;
      lElementTextLen = aElementText->length();
   } else {
      string lTxt;
      aElement->CalculateJsonTextRepresentation(lTxt);
      lCalculatedText += lTxt;
      lElementTextLen = lTxt.length();
   }

   if(!lIsLast)
   {
      lCalculatedText += ", ";
   } 
  
   // Update document text
   GetDocument()->GetOwner()->spliceJsonTextByDomChange(GetAbsTextRange().start+lChangeAddr, 0, lCalculatedText);
 
   // Add element and fixup addresses after splicing
   aElement->textRange.start = lElemAddr;
   aElement->textRange.end = lElemAddr + lElementTextLen;
   lMember.nameRange.start = lNameAddr;
   lMember.nameRange.end = lNameAddr + lNameLen;

   members.insert(lIter, lMember);

   return *lIter;
}
   
ObjectNode::Member &ObjectNode::DomAddMemberNode(const string &aName, Node *aElement)
{
   Member lMemberInfo;
   lMemberInfo.name = aName;
   lMemberInfo.node = aElement;
   
   members.push_back(lMemberInfo);
   
   lMemberInfo.node->parent = this;
   
   return *(members.end()-1);
}

void ObjectNode::DetachChildAt(int aIdx, Node **aNode)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());

   Members::iterator lIter = members.begin() + aIdx;
   Members::iterator lNextIter = lIter+1;
   
   // Splice text in document   
   TextRange lMyAbsRange = GetAbsTextRange();
   TextRange lSpliceRange = lMyAbsRange;
   
   lSpliceRange.start += lIter->nameRange.start;
   
   if(lNextIter!=members.end())
   {
      lSpliceRange.end = lNextIter->nameRange.start + lMyAbsRange.start;
   } else
   {
      lSpliceRange.end--;
   }
   
   *aNode = lIter->node;
   (*aNode)->parent = NULL;

   members.erase(lIter);   

   GetDocument()->GetOwner()->spliceJsonTextByDomChange(lSpliceRange.start, lSpliceRange.length(), "");
}

NodeTypeId ObjectNode::GetNodeTypeId() const
{
   return ntObject;
}

void ObjectNode::accept(NodeVisitor *aVisitor) const
{
   aVisitor->visitNode(this);
   for(Members::const_iterator lIter = members.begin(); lIter!=members.end(); lIter++)
   {
      lIter->node->accept(aVisitor);
   }
}

void ObjectNode::accept(NodeVisitor *aVisitor)
{
   aVisitor->visitNode(this);
   for(Members::iterator lIter = members.begin(); lIter!=members.end(); lIter++)
   {
      lIter->node->accept(aVisitor);
   }
}
   

void ObjectNode::CalculateJsonTextRepresentation(std::string &aDest) const
{
   // TODO
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DocumentNode::DocumentNode(JsonFile *aOwner, Node *aChild)
   : rootNode(aChild), owner(aOwner)
{
   if(rootNode) {
       rootNode->parent = this;
       textRange.start = 0;
       textRange.end = rootNode->textRange.end;
   } else {
       textRange.start = 0;
       textRange.end = 0;
   }
}
      
NodeTypeId DocumentNode::GetNodeTypeId() const 
{
   return ntObject;
}

int DocumentNode::GetChildCount() const
{
   return rootNode ? 1 : 0;
}
   
Node *DocumentNode::GetChildAt(int aIdx)
{
   return rootNode;
}

const Node *DocumentNode::GetChildAt(int aIdx) const
{
   return rootNode;
}

void DocumentNode::SetChildAt(int aIdx, Node *aNode)
{
   // NOP
}

int DocumentNode::FindChildEndingAfter(const TextCoordinate &aDocOffset) const
{
   return 0;
}

void DocumentNode::accept(NodeVisitor *aVisitor) const
{
   aVisitor->visitNode(this);
   rootNode->accept(aVisitor);
}

void DocumentNode::accept(NodeVisitor *aVisitor)
{
   aVisitor->visitNode(this);
    if(rootNode) {
        rootNode->accept(aVisitor);
    }
}

void DocumentNode::CalculateJsonTextRepresentation(std::string &aDest) const
{
   // TODO
}

void DocumentNode::DetachChildAt(int aIdx, Node **aNode)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LeafNode::accept(NodeVisitor *aVisitor) const
{
   aVisitor->visitNode(this);
}

void LeafNode::accept(NodeVisitor *aVisitor)
{
   aVisitor->visitNode(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


NodeTypeId NullNode::GetNodeTypeId() const
{
   return ntNull;
}

void NullNode::CalculateJsonTextRepresentation(std::string &aDest) const
{
   aDest = "null";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



template<> 
void ValueNode<std::string, ntString>::CalculateJsonTextRepresentation(std::string &aDest) const
{
   stringstream lStream;
   
   lStream << '"';

   std::string::const_iterator it(value.begin()),
                               itEnd(value.end());
   for (; it != itEnd; ++it)
   {
      switch (*it)
      {
         case '"':         lStream << "\\\"";   break;
         case '\\':        lStream << "\\\\";   break;
         case '\b':        lStream << "\\b";    break;
         case '\f':        lStream << "\\f";    break;
         case '\n':        lStream << "\\n";    break;
         case '\r':        lStream << "\\r";    break;
         case '\t':        lStream << "\\t";    break;
         default:
            if (*it >= 0x20) {
               lStream << *it;
            }
            else {
                // UTF8
                unsigned int uc[4] = { (unsigned char)(*it) };
                int length = 0, i;
				  
                if (uc[0] < 0x80)              length = 1;
                else if ((uc[0] >> 5) == 0x6)  length = 2;
                else if ((uc[0] >> 4) == 0xe)  length = 3;
                else if ((uc[0] >> 3) == 0x1e) length = 4;
				  
                for (i = 1; (i < length) && (++it != itEnd); ++i)
                    uc[i] = (unsigned char)(*it);
				  
                if (i == length) {
                    unsigned int codepoint = 0;
                    if (length == 1) codepoint = uc[0];
                    else if (length == 2) codepoint = ((uc[0] << 6) & 0x7ff) + (uc[1] & 0x3f);
                    else if (length == 3) codepoint = ((uc[0] << 12) & 0xffff) + ((uc[1] << 6) & 0xfff) + (uc[2] & 0x3f);
                    else if (length == 4) codepoint = ((uc[0] << 18) & 0x1fffff) + ((uc[1] << 12) & 0x3ffff) + ((uc[2] << 6) & 0xfff) + (uc[3] & 0x3f);
                    lStream << std::hex << std::setfill('0');
                    if (codepoint > 0xffff) {
                        lStream << "\\u" << std::setw(4) << (0xd800 + ((codepoint - 0x10000) >> 10));
                        lStream << "\\u" << std::setw(4) << (0xDC00 + ((codepoint - 0x10000) & 0x3ff));
                    }
                    else {
                        lStream << "\\u" << std::setw(4) << (codepoint & 0xffff);
                    }
                }
                else {
                    lStream << '?';
                    if (it == itEnd) {
                        --it; // We have a uncode error and it was advanced too much
                    }
                }
            }
      }
   }

   lStream << '"';   

   aDest = lStream.str();
}


template<> 
void ValueNode<double, ntNumber>::CalculateJsonTextRepresentation(std::string &aDest) const
{
   stringstream lStream;

   lStream << std::setprecision(20) << value;

   aDest = lStream.str();
}

template<> 
void ValueNode<bool, ntBoolean>::CalculateJsonTextRepresentation(std::string &aDest) const
{
   aDest = value ? "true" : "false";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Exception::Exception(const std::string& sMessage) : runtime_error(sMessage)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class JsonFileParseListener : public Reader::ParseListener
{
public:
   JsonFileParseListener(JsonFile *aFile)
    : file(aFile) {   }
   
   void EndOfLine(TextCoordinate aCoord)
   {
   }

   void Error(TextCoordinate aWhere, int aCode, const string &aText)
   {
      file->errors.addMarker(ParseErrorMarker(aWhere, aCode, aText));
      printf("At %d, %d: %s\n", aWhere, aCode, aText.c_str());
   }
   
private:
   JsonFile *file;
} ;


JsonFile::JsonFile()
   : notificationsDeferred(0)
{
   jsonDom = new DocumentNode(this, new NullNode());
}
      
void JsonFile::setText(const string &aText)
{
   JsonFileParseListener listener(this);
   
   jsonText = aText;
   stringstream lStream(aText);
   
   stopwatch lStopWatch("Read Json");
   Node *lNode = NULL;
   Reader::Read(lNode, lStream, &listener);
   lStopWatch.stop();
   
   jsonDom = new DocumentNode(this, lNode);
}

const string &JsonFile::getText() const
{
   return jsonText;
}

DocumentNode *JsonFile::getDom()
{
   return jsonDom;
}

const DocumentNode *JsonFile::getDom() const
{
   return jsonDom;
}

bool JsonFile::FindPathForJsonPathString(std::string aPathString, JsonPath &aRet) {
    Node *lCurNode = jsonDom->GetChildAt(0);
    istringstream lPathStream(aPathString.c_str());
    string lPathComponent;
    while(getline(lPathStream, lPathComponent, '.')) {
        if(ObjectNode *lCurObjectNode = dynamic_cast<ObjectNode*>(lCurNode)) {
            int lMemberIdx = lCurObjectNode->GetIndexOfMemberWithName(lPathComponent);
            if(lMemberIdx < 0) {
                return false;
            }
            aRet.push_back(lMemberIdx);
            lCurNode = lCurObjectNode->GetChildAt(lMemberIdx);
        } else
        if(ArrayNode *lCurArrayNode = dynamic_cast<ArrayNode*>(lCurNode)) {
            char * p;
            int lArrayIndex = strtol(lPathComponent.c_str(), &p, 10);
            if(*p != 0) {
               return false;
            }
            aRet.push_back(lArrayIndex);
            lCurNode = lCurArrayNode->GetChildAt(lArrayIndex);
        } else {
            return false;
        }
    }
    
    return true;
}

bool JsonFile::FindPathContaining(unsigned int aDocOffset, JsonPath &path) const
{
   const ContainerNode *lCurElem = dynamic_cast<ContainerNode*>(jsonDom->GetChildAt(0));
   
   while( lCurElem )
   {
      aDocOffset -= lCurElem->GetTextRange().start;

      int lNextNav = lCurElem->FindChildContaining(aDocOffset);
      
      if(lNextNav<0)
         return true;
      
      path.push_back(lNextNav);
            
      lCurElem = dynamic_cast<const ContainerNode*>(lCurElem->GetChildAt(lNextNav));
   }
   
   return true;
}




void JsonFile::spliceJsonTextByDomChange(TextCoordinate aOffsetStart, TextLength aLen, const std::string &aNewText)
{
   // Don't send notifications till we're thru
   DeferNotificationsInBlock lDnib(this);

   stopwatch lSpliceTime("spliceJsonTextByDomChange");

   int lNewLen = aNewText.length();

   // Update text and line lengths
   jsonText.insert(aOffsetStart, aNewText);
   jsonText.erase(aOffsetStart+lNewLen, aLen);
   lSpliceTime.lap("Text update");
     
   notify(SpliceNotification(aOffsetStart, aLen, lNewLen));
   
   // Update tree offsets
   updateTreeOffsetsAfterSplice(aOffsetStart, aLen, lNewLen);
   
   // Update errors
   updateErrorsAfterSplice(aOffsetStart, aLen, lNewLen);

}

void JsonFile::updateTreeOffsetsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen)
{
   // Update offsets in json tree; start in top most level
   int lLenDiff = aNewLen - aLen;
   ContainerNode *lCurContainer = jsonDom;
   ContainerNode *lNextContainer = NULL;

   int lChangedOffset = aOffsetStart;
   do 
   {
      lChangedOffset -= lCurContainer->GetTextRange().start;
      int lChildCount = lCurContainer->GetChildCount();         

      // First first child that ends after change start
      int lCurProcessChild = lCurContainer->FindChildEndingAfter(lChangedOffset);
      if(lCurProcessChild==-1)
      {
         lNextContainer=NULL;
         break;
      }

      // Check if first child intersects with change offset
      Node *lNode = lCurContainer->GetChildAt(lCurProcessChild);
      if(lNode->GetTextRange().start > lChangedOffset)
      {
         // No intersection - we don't need to look in additional inner containers
         lNextContainer = NULL; 
      } else 
      {
         lNode->textRange.end += lLenDiff;
         if(aLen==0 && lNode->GetTextRange().start == lChangedOffset)
         {
            lNode->textRange.start += lLenDiff;           
         }

         // Got intersection; if this is a container, ask to handle it later
         lNextContainer = dynamic_cast<ContainerNode*>(lNode);         
         lCurProcessChild++;
      } 

      
      // Adjust all elements in current level that are strictly *after* the changed region
      while(lCurProcessChild < lChildCount)
      {
         lCurContainer->AdjustChildRangeAt(lCurProcessChild, lLenDiff);
         lCurProcessChild++;
      }
      
      lCurContainer = lNextContainer;
   } while(lCurContainer);
}


void JsonFile::updateErrorsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen)
{
   if(errors.spliceCoordinatesList(aOffsetStart, aLen, aNewLen))
   {
      notify(ErrorsChangedNotification());
   }
}
   

void JsonFile::addListener(JsonFileChangeListener *aListener)
{
   listeners.push_back(aListener);
}

void JsonFile::removeListener(JsonFileChangeListener *aListener)
{
   Listeners::iterator lIter = find(listeners.begin(), listeners.end(), aListener);
   if(lIter!=listeners.end())
   {
      listeners.erase(lIter);
   }
}


void JsonFile::beginDeferNotifications()
{
   notificationsDeferred++;
}

void JsonFile::endDeferNotifications()
{
   if(!--notificationsDeferred)
   {
      while(!deferredNotifications.empty())
      {
         Notification *lNotification = deferredNotifications.front();
         deliverNotification(*lNotification);
         delete lNotification;
         deferredNotifications.pop_front();
      }
   }
}

void JsonFile::deliverNotification(const Notification &aNotification)
{
   stopwatch lStopWatch("deliverNotification");
   
   // Update listeners
   for(Listeners::iterator lIter = listeners.begin(); lIter!=listeners.end(); lIter++)
   {
      aNotification.deliverToListener(this, *lIter);
   }   
}
   
void JsonFile::notify(const Notification &aNotification)
{
   if(notificationsDeferred)
   {
      deferredNotifications.push_back(aNotification.clone());
   } else 
   {
      deliverNotification(aNotification);
   }
}
   
static string jsonize(const string &aSrc)
{
   StringNode lNode(aSrc);
   string lRet;
   lNode.CalculateJsonTextRepresentation(lRet);
   return lRet;
}
   
}

