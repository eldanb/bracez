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

namespace priv
{


class SpliceNotification : public Notification
{
public:
    SpliceNotification(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen,
                       TextCoordinate aLineDelStart, TextLength aLineDelLen, TextLength aNumNewLines) :
    offsetStart(aOffsetStart), len(aLen), newLen(aNewLen),
    lineOffsetStart(aLineDelStart), lineLen(aLineDelLen), lineNewLen(aNumNewLines)
    {
        
    }
    
    SpliceNotification *clone() const
    {
        return new SpliceNotification(*this);
    }
    
    void deliverToListener(JsonFile *aSender, JsonFileChangeListener *aListener) const
    {
        aListener->notifyTextSpliced(aSender, offsetStart, len, newLen, lineOffsetStart, lineLen, lineNewLen);
    }
    
private:
    TextCoordinate offsetStart;
    TextLength len;
    TextLength newLen;
    
    TextCoordinate lineOffsetStart;
    TextLength lineLen;
    TextLength lineNewLen;
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


class NodeRefreshNotification : public Notification
{
public:
    NodeRefreshNotification(JsonPath &&path) :
    _path(path) {}
    
    NodeRefreshNotification(const NodeRefreshNotification &aOther)
    : _path(aOther._path)
    {
        
    }
    
    NodeRefreshNotification *clone() const
    {
        return new NodeRefreshNotification(*this);
    }
    
    void deliverToListener(JsonFile *aSender, JsonFileChangeListener *aListener) const
    {
        aListener->notifyNodeChanged(aSender, _path);
    }
    
private:
    JsonPath _path;
}  ;

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

DocumentNode *Node::getDocument() const
{
    const Node *lCurNode = this;
    while(lCurNode->getParent())
    {
        lCurNode = lCurNode->getParent();
    }
    
    return const_cast<DocumentNode*>(dynamic_cast<const DocumentNode*>(lCurNode));
}

const TextRange &Node::getTextRange() const
{
    return textRange;
}


wstring Node::getDocumentText() const
{
    TextRange lAbsRange = getAbsTextRange();
    return getDocument()->getOwner()->getText().substr(lAbsRange.start, lAbsRange.length());
}


TextRange Node::getAbsTextRange() const
{
    int lOfs = 0;
    const Node *lCurNode = getParent();
    while(lCurNode)
    {
        lOfs += lCurNode->getTextRange().start;
        lCurNode = lCurNode->getParent();
    }
    
    return TextRange(textRange.start+lOfs, textRange.end+lOfs);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ContainerNode::setChildAt(int aIdx, Node *aNode, bool fromReparse)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(getDocument()->getOwner());
    
    // Get old node
    Node *lOldNode = getChildAt(aIdx);
    
    // Get new node text and range
    if(!fromReparse) {
        TextCoordinate lNewStart = lOldNode->textRange.start;
        TextCoordinate lNewEnd;
        
        std::wstring lNewNodeText;
        aNode->calculateJsonTextRepresentation(lNewNodeText);
        
        lNewEnd = lNewStart + (int)lNewNodeText.length();
        
        aNode->textRange.start = lNewStart;
        aNode->textRange.end = lNewEnd;
        
        // Splice text in document
        TextRange lAbsTextRange = lOldNode->getAbsTextRange();
        getDocument()->getOwner()->spliceJsonTextByDomChange(lAbsTextRange.start, lAbsTextRange.length(), lNewNodeText);
        getDocument()->getOwner()->notifyUpdatedNode(this);
    }
    
    // Update in child list and import to document
    storeChildAt(aIdx, aNode);
    aNode->parent = this;
}

int ContainerNode::findChildContaining(const TextCoordinate &aDocOffset, bool strict) const
{
    // Fast path for cached child
    if(cachedLastRangeFoundChild != -1 && cachedLastRangeFoundChild < getChildCount()) {
        TextRange tr = getChildAt(cachedLastRangeFoundChild)->getTextRange();
        if( (tr.start < aDocOffset || (!strict && tr.start == aDocOffset)) &&
           (tr.end > aDocOffset) ) {
            return cachedLastRangeFoundChild;
        }
        
        if(aDocOffset >= tr.end && cachedLastRangeFoundChild+1 < getChildCount()) {
            TextRange tr2 = getChildAt(cachedLastRangeFoundChild+1)->getTextRange();
            if( (tr2.start < aDocOffset || (!strict && tr2.start == aDocOffset)) &&
               (tr2.end > aDocOffset) ) {
                cachedLastRangeFoundChild = cachedLastRangeFoundChild+1;
                return cachedLastRangeFoundChild;
            } else
                if(tr2.start > aDocOffset) {
                    return -1;
                }
        }
    }
    
    int lRet = findChildEndingAfter(aDocOffset);
    if(lRet>=0)
    {
        TextRange tr = getChildAt(lRet)->getTextRange();
        if(tr.start < aDocOffset || (!strict && tr.start == aDocOffset)) {
            cachedLastRangeFoundChild = lRet;
            return lRet;
        }
    }
    
    return -1;
}

int ContainerNode::getIndexOfChild(const Node *aChild) const
{
    int lChildCount = getChildCount();
    for(int lIdx = 0; lIdx<lChildCount; lIdx++)
    {
        if(getChildAt(lIdx) == aChild)
        {
            return lIdx;
        }
    }
    
    return -1;
}

void ContainerNode::removeChildAt(int aIdx)
{
    Node *lChild;
    detachChildAt(aIdx, &lChild);
    
    delete lChild;
}


void ContainerNode::adjustChildRangeAt(int aIdx, long aDiff)
{
    Node *lNode = getChildAt(aIdx);
    
    lNode->textRange.end += aDiff;
    lNode->textRange.start += aDiff;
}

bool ContainerNode::valueEquals(Node *other) const {
    ContainerNode *otherCont = dynamic_cast<ContainerNode*>(other);
    if(!otherCont) {
        return false;
    }
    
    if(otherCont->getChildCount() != getChildCount()) {
        return false;
    }
    
    for(int idx=0; idx<getChildCount(); idx++) {
        if(!getChildAt(idx)->valueEquals(otherCont->getChildAt(idx))) {
            return false;
        }
    }
    
    return true;
}

bool ContainerNode::valueLt(Node *other) const {
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ArrayNode::iterator ArrayNode::begin()
{
    return elements.begin();
}

ArrayNode::iterator ArrayNode::end()
{
    return elements.end();
}

ArrayNode::const_iterator ArrayNode::begin() const
{
    return elements.begin();
}

ArrayNode::const_iterator ArrayNode::end() const
{
    return elements.end();
}

int ArrayNode::getChildCount() const
{
    return (int)elements.size();
}

Node *ArrayNode::getChildAt(int aIdx)
{
    return elements[aIdx].get();
}

const Node *ArrayNode::getChildAt(int aIdx) const
{
    return elements[aIdx].get();
}

void ArrayNode::storeChildAt(int aIdx, Node *aNode) {
    elements[aIdx].reset(aNode);
}

void ArrayNode::detachChildAt(int aIdx, Node **aNode)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(getDocument()->getOwner());
    
    Elements::iterator lIter = elements.begin() + aIdx;
    Elements::iterator lNextIter = lIter+1;
    
    // Splice text in document
    TextRange lSpliceRange = (*lIter)->getAbsTextRange();
    if(lNextIter!=elements.end())
    {
        lSpliceRange.end = (*lNextIter)->getAbsTextRange().start;
    } else {
        if(lIter!=elements.begin()) {
            Elements::iterator lPrevIter = lIter-1;
            lSpliceRange.start = (*lPrevIter)->getAbsTextRange().end;
        }
    }
    
    *aNode = lIter->release();
    (*aNode)->parent = NULL;
    
    elements.erase(lIter);
    
    getDocument()->getOwner()->spliceJsonTextByDomChange(lSpliceRange.start, lSpliceRange.length(), wstring(L""));
    getDocument()->getOwner()->notifyUpdatedNode(this);
}

void ArrayNode::InsertMemberAt(int aIdx, Node *aElement, const wstring *aElementText)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(getDocument()->getOwner());
    
    unsigned long lElemAddr;
    unsigned long lChangeAddr;
    
    bool lIsLast;
    
    // Append or insert member at members list, devise text offset for member.
    Elements::iterator lIter;
    if(aIdx < 0 || aIdx >= elements.size())
    {
        aIdx = (int)elements.size();
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
    wstring lCalculatedText;
    if(lIsLast && aIdx)
    {
        lCalculatedText = L", ";
        lElemAddr+=2;
    }
    
    // Next -- element text
    size_t lElementTextLen;
    if(aElementText)
    {
        lCalculatedText += *aElementText;
        lElementTextLen = aElementText->length();
    } else {
        wstring lTxt;
        aElement->calculateJsonTextRepresentation(lTxt);
        lElementTextLen = lTxt.length();
        lCalculatedText += lTxt;
    }
    
    // Add comma if not last element
    if(!lIsLast)
    {
        lCalculatedText += L", ";
    }
    
    // Update document text
    getDocument()->getOwner()->spliceJsonTextByDomChange(getAbsTextRange().start+lChangeAddr, 0, lCalculatedText);
    getDocument()->getOwner()->notifyUpdatedNode(this);
    
    // Add element to sequence and setup address
    elements.insert(lIter, std::unique_ptr<Node>(aElement));
    
    aElement->textRange.start = TextCoordinate(lElemAddr);
    aElement->textRange.end = TextCoordinate(lElemAddr + (unsigned int)lElementTextLen);
}

struct ArrayMemberTextRangeOrdering 
{
    bool operator()(const std::unique_ptr<Node> &aLeft, const std::unique_ptr<Node> &aRight) const
    {
        return aLeft->getTextRange().end < aRight->getTextRange().end;
    }
    
    bool operator() (std::unique_ptr<Node> const &aLeft, TextCoordinate aRight) const
    {
        return aLeft->getTextRange().end < aRight;
    }
};


int ArrayNode::findChildEndingAfter(const TextCoordinate &aDocOffset) const
{
    const_iterator lContainingElement = lower_bound(elements.begin(), elements.end(), aDocOffset+1, ArrayMemberTextRangeOrdering());
    
    // Found member ending past offset?
    if(lContainingElement == elements.end())
        return -1;
    
    return (int)(lContainingElement - elements.begin());
}


void ArrayNode::domAddElementNode(Node *aElement)
{
    elements.push_back(std::unique_ptr<Node>(aElement));
    aElement->parent = this;
}

NodeTypeId ArrayNode::getNodeTypeId() const
{
    return ntArray;
}

void ArrayNode::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = L"[ ";
    for(auto iter = elements.begin(); iter != elements.end(); ) {
        std::wstring fragment;
        iter->get()->calculateJsonTextRepresentation(fragment, maxLenHint);
        
        aDest += fragment;
        
        iter ++;
        
        if(iter != elements.end()) {
            aDest += L", ";
        } else {
            aDest += L" ";
        }
        
        if(aDest.length() > maxLenHint) {
            return;
        }
    }
    
    aDest += L"]";
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

void ArrayNode::acceptInRange(NodeVisitor *aVisitor, TextRange &range) {
    TextRange adjustedRange = range.intersectWithAndRelativeTo(getTextRange());
    
    if(!adjustedRange.length()) {
        return;
    }
    
    aVisitor->visitNode(this);
    
    int firstChildToSearch = this->findChildEndingAfter(adjustedRange.start);
    if(firstChildToSearch < 0) {
        firstChildToSearch = 0;
    }
    
    for(Elements::iterator lIter = elements.begin() + firstChildToSearch;
        lIter!=elements.end() && (*lIter)->getTextRange().start < adjustedRange.end;
        lIter++)
    {
        (*lIter)->acceptInRange(aVisitor, adjustedRange);
    }
}


ArrayNode *ArrayNode::clone() const {
    ArrayNode *ret = new ArrayNode();
    for(Elements::const_iterator lIter = elements.begin(); lIter!=elements.end(); lIter++)
    {
        ret->domAddElementNode((*lIter)->clone());
    }
    
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode::iterator ObjectNode::begin()
{
    return members.begin();
}

ObjectNode::iterator ObjectNode::end()
{
    return members.end();
}

ObjectNode::const_iterator ObjectNode::begin() const
{
    return members.begin();
}

ObjectNode::const_iterator ObjectNode::end() const
{
    return members.end();
}

int ObjectNode::getChildCount() const
{
    return (int)members.size();
}

Node *ObjectNode::getChildAt(int aIdx)
{
    return members[aIdx].node.get();
}

const Node *ObjectNode::getChildAt(int aIdx) const
{
    return members[aIdx].node.get();
}

int ObjectNode::getIndexOfMemberWithName(const wstring &name) const {
    for(Members::const_iterator iter = members.begin();
        iter != members.end();
        iter++) {
        if(iter->name == name) {
            return (int)(iter - members.begin());
        }
    }
    
    return -1;
}

const ObjectNode::Member *ObjectNode::getChildMemberAt(int aIdx) const
{
    return &members[aIdx];
}

void ObjectNode::storeChildAt(int aIdx, Node *aNode) {
    members[aIdx].node.reset(aNode);
}

void ObjectNode::adjustChildRangeAt(int aIdx, long aDiff)
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
        return aLeft.node->getTextRange().end < aRight.node->getTextRange().end;
    }
    
    bool operator() (const ObjectNode::Member &aLeft, TextCoordinate aRight) const
    {
        return aLeft.node->getTextRange().end < aRight;
    }
};

int ObjectNode::findChildEndingAfter(const TextCoordinate &aDocOffset) const
{
    const_iterator lContainingElement = lower_bound(members.begin(), members.end(), aDocOffset+1, ObjectMemberTextRangeOrdering());
    
    // Found member ending past offset?
    if(lContainingElement == members.end())
        return -1;
    
    //   printf("Child ending after %d is '%s\n", aDocOffset, lContainingElement->name.c_str());
    return (int)(lContainingElement - members.begin());
}

const wstring &ObjectNode::getMemberNameAt(int aIdx) const
{
    return members[aIdx].name;
}

void ObjectNode::renameMemberAt(int aIdx, const wstring &aName) {
    TextRange orgRange = members[aIdx].nameRange;
    std::wstring orgName = members[aIdx].name;
    
    std::wstring jsonizedName = jsonizeString(aName);
    TextRange myAbs = getAbsTextRange();
    members[aIdx].name = aName;
    getDocument()->getOwner()->spliceJsonTextByDomChange(orgRange.start + myAbs.start, orgRange.length(),
                                                         jsonizedName);
    getDocument()->getOwner()->notifyUpdatedNode(this);
    
    
    // A bit of a hack:
    // Fix name range; it it was changed wrongly by spliceJsonTextByDomChange above.
    members[aIdx].nameRange.start = orgRange.start;
    members[aIdx].nameRange.end = members[aIdx].nameRange.start + (int)jsonizedName.length();
}

ObjectNode::Member &ObjectNode::insertMemberAt(int aIdx, const wstring &aName, Node *aElement, const wstring *aElementText)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(getDocument()->getOwner());
    
    unsigned long lNameAddr;
    unsigned long lChangeAddr;
    
    Member lMember(aName, aElement);
    bool lIsLast;
    
    // Append or insert member at members list, devise text offset for member.
    Members::iterator lIter;
    
    if(aIdx < 0 || aIdx >= members.size())
    {
        aIdx = (int)members.size();
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
    wstring lCalculatedText;
    unsigned long lNameLen;
    
    lCalculatedText = jsonizeString(lMember.name);
    lNameLen = lCalculatedText.length();
    
    if(lIsLast && aIdx)
    {
        lCalculatedText = L", " + lCalculatedText;
        lNameAddr+=2;
    }
    
    lCalculatedText += L": ";
    
    // (Update node start address to skip name)
    unsigned long lElemAddr = lChangeAddr + lCalculatedText.length();
    
    // Next -- element text
    unsigned long lElementTextLen;
    if(aElementText)
    {
        lCalculatedText += *aElementText;
        lElementTextLen = aElementText->length();
    } else {
        wstring lTxt;
        aElement->calculateJsonTextRepresentation(lTxt);
        lCalculatedText += lTxt;
        lElementTextLen = lTxt.length();
    }
    
    if(!lIsLast)
    {
        lCalculatedText += L", ";
    }
    
    // Update document text
    getDocument()->getOwner()->spliceJsonTextByDomChange(getAbsTextRange().start+lChangeAddr, 0, lCalculatedText);
    getDocument()->getOwner()->notifyUpdatedNode(this);
    
    // Add element and fixup addresses after splicing
    aElement->textRange.start = TextCoordinate(lElemAddr);
    aElement->textRange.end = TextCoordinate(lElemAddr + lElementTextLen);
    lMember.nameRange.start = TextCoordinate(lNameAddr);
    lMember.nameRange.end = TextCoordinate(lNameAddr + lNameLen);
    
    members.insert(lIter, std::move(lMember));
    
    return *lIter;
}

ObjectNode::Member &ObjectNode::domAddMemberNode(wstring &&aName, Node *aElement)
{
    aElement->parent = this;
    members.emplace_back(std::move(aName), aElement);
    
    return *(members.end()-1);
}

ObjectNode::Member &ObjectNode::domAddMemberNode(const wstring &aName, Node *aElement)
{
    aElement->parent = this;
    members.emplace_back(aName, aElement);
    
    return *(members.end()-1);
}

Node *ObjectNode::clone() const {
    ObjectNode *ret = new ObjectNode();
    for(Members::const_iterator lIter = members.begin(); lIter!=members.end(); lIter++)
    {
        ret->domAddMemberNode(std::wstring(lIter->name), lIter->node->clone());
    }
    
    return ret;
}

void ObjectNode::detachChildAt(int aIdx, Node **aNode)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(getDocument()->getOwner());
    
    Members::iterator lIter = members.begin() + aIdx;
    Members::iterator lNextIter = lIter+1;
    
    // Splice text in document
    TextRange lMyAbsRange = getAbsTextRange();
    TextRange lSpliceRange = lMyAbsRange;
    
    lSpliceRange.start += lIter->nameRange.start;
    
    if(lNextIter!=members.end())
    {
        lSpliceRange.end = lNextIter->nameRange.start + lMyAbsRange.start;
    } else
    {
        if(lIter != members.begin()) {
            Members::iterator lPrevIter = lIter-1;
            lSpliceRange.start = lPrevIter->node->getAbsTextRange().end;
        }
        lSpliceRange.end = lSpliceRange.end - 1;
    }
    
    *aNode = lIter->node.release();
    (*aNode)->parent = NULL;
    
    members.erase(lIter);
    
    getDocument()->getOwner()->spliceJsonTextByDomChange(lSpliceRange.start, lSpliceRange.length(), L"");
    getDocument()->getOwner()->notifyUpdatedNode(this);
}

NodeTypeId ObjectNode::getNodeTypeId() const
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

void ObjectNode::acceptInRange(NodeVisitor *aVisitor, TextRange &range) {
    TextRange adjustedRange = range.intersectWithAndRelativeTo(getTextRange());
    
    if(!adjustedRange.length()) {
        return;
    }
    
    aVisitor->visitNode(this);
    
    // Since the search range may actually start in the child's key name, we
    // err for the careful side and start the search in the previous child
    int firstChildToSearch = this->findChildContaining(adjustedRange.start, false) - 1;
    if(firstChildToSearch < 0) {
        firstChildToSearch = 0;
    }
    
    for(Members::iterator lIter = members.begin() + firstChildToSearch;
        lIter!=members.end() && lIter->node->getTextRange().start < adjustedRange.end;
        lIter++)
    {
        lIter->node->acceptInRange(aVisitor, adjustedRange);
    }
}


bool ObjectNode::valueEquals(Node *other) const {
    ObjectNode *objOtherNode = dynamic_cast<ObjectNode*>(other);
    if(!objOtherNode) {
        return false;
    }
    
    if(!ContainerNode::valueEquals(other)) {
        return false;
    }
    
    int childCount = getChildCount();
    for(int idx=0; idx<childCount; idx++) {
        if(getMemberNameAt(idx) != objOtherNode->getMemberNameAt(idx)) {
            return false;
        }
    }
    
    return true;
}

void ObjectNode::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = L"{ ";
    for(auto iter = members.begin(); iter != members.end(); ) {
        aDest += L"\"" + iter->name + L"\": ";
        
        std::wstring fragment;
        iter->node->calculateJsonTextRepresentation(fragment, maxLenHint);
        aDest += fragment;
        
        iter ++;
        
        if(iter != members.end()) {
            aDest += L", ";
        } else {
            aDest += L" ";
        }
        
        if(aDest.length() > maxLenHint) {
            return;
        }
    }
    
    aDest += L"}";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DocumentNode::DocumentNode(JsonFile *aOwner, Node *aChild)
: rootNode(aChild), owner(aOwner)
{
    if(rootNode) {
        rootNode->parent = this;
    }
}

NodeTypeId DocumentNode::getNodeTypeId() const 
{
    return ntObject;
}

int DocumentNode::getChildCount() const
{
    return rootNode ? 1 : 0;
}

Node *DocumentNode::getChildAt(int aIdx)
{
    return rootNode.get();
}

const Node *DocumentNode::getChildAt(int aIdx) const
{
    return rootNode.get();
}

void DocumentNode::storeChildAt(int aIdx, Node *aNode)
{
    // NOP
}

int DocumentNode::findChildEndingAfter(const TextCoordinate &aDocOffset) const
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

void DocumentNode::acceptInRange(NodeVisitor *aVisitor, TextRange &range)
{
    aVisitor->visitNode(this);
    if(rootNode) {
        rootNode->acceptInRange(aVisitor, range);
    }
}

void DocumentNode::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    // TODO
}

void DocumentNode::detachChildAt(int aIdx, Node **aNode)
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

void LeafNode::acceptInRange(NodeVisitor *aVisitor, TextRange &range) {
    aVisitor->visitNode(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


NodeTypeId NullNode::getNodeTypeId() const
{
    return ntNull;
}

void NullNode::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = L"null";
}

bool NullNode::valueEquals(Node *other) const {
    return !!dynamic_cast<NullNode*>(other);
}

bool NullNode::valueLt(Node *other) const {
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



template<> 
void ValueNode<std::wstring, ntString>::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    wstringstream lStream;
    
    lStream << L'"';
    
    std::wstring::const_iterator it(value.begin()),
    itEnd(value.end());
    for (; it != itEnd; ++it)
    {
        switch (*it)
        {
            case L'"':         lStream << L"\\\"";   break;
            case L'\\':        lStream << L"\\\\";   break;
            case L'\b':        lStream << L"\\b";    break;
            case L'\f':        lStream << L"\\f";    break;
            case L'\n':        lStream << L"\\n";    break;
            case L'\r':        lStream << L"\\r";    break;
            case L'\t':        lStream << L"\\t";    break;
            default:
                if (*it >= 0x20 && *it < 128) {
                    lStream << *it;
                }
                else {
                    lStream << "\\u" << std::setw(4) << (*it & 0xffff);
                }
        }
    }
    
    lStream << '"';
    
    aDest = lStream.str();
}


template<> 
void ValueNode<double, ntNumber>::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    wstringstream lStream;
    
    lStream << std::setprecision(20) << value;
    
    aDest = lStream.str();
}

template<> 
void ValueNode<bool, ntBoolean>::calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = value ? L"true" : L"false";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class JsonParseErrorCollectionListenerListener : public ParseListener
{
public:
    JsonParseErrorCollectionListenerListener(MarkerList<ParseErrorMarker> &errors)
    : _errors(errors) {   }
    
    void EndOfLine(TextCoordinate aCoord)
    {
    }
    
    void Error(TextCoordinate aWhere, int aCode, const string &aText)
    {
        _errors.addMarker(ParseErrorMarker(aWhere, aCode, aText));
    }
    
private:
    MarkerList<ParseErrorMarker> &_errors;
} ;


JsonFile::JsonFile()
: notificationsDeferred(0), jsonDom(new DocumentNode(this, new NullNode())), jsonText(new std::wstring())
{
    lineStarts.appendMarker(BaseMarker(TextCoordinate(0)));
}


void JsonFile::setText(const wstring &aText)
{
    JsonParseErrorCollectionListenerListener listener(errors);
    
    lineStarts.clear();
    lineStarts.appendMarker(BaseMarker(TextCoordinate(0)));
    
    jsonText = make_shared<std::wstring>(aText);
    
    errors.clear();
    
    stopwatch lStopWatch("Read Json");
    Node *lNode = NULL;
    Reader::Read(lNode, aText, &listener);
    lStopWatch.stop();
    
    jsonDom.reset(new DocumentNode(this, lNode));
    jsonDom->textRange.start = TextCoordinate(0);
    jsonDom->textRange.end = TextCoordinate(aText.length());
    
    TextCoordinate lineChangeStart;
    TextLength lineChangeOldLen, lineChangeNewLen;
    updateLineOffsetsAfterSplice((TextCoordinate)0, 0, aText.length(), aText.c_str(), &lineChangeStart, &lineChangeOldLen, &lineChangeNewLen);
    
    notify(ErrorsChangedNotification());
}

const wstring &JsonFile::getText() const
{
    return *jsonText;
}

DocumentNode *JsonFile::getDom()
{
    return jsonDom.get();
}

const DocumentNode *JsonFile::getDom() const
{
    return jsonDom.get();
}

bool JsonFile::findPathForJsonPathString(std::wstring aPathString, JsonPath &aRet) {
    Node *lCurNode = jsonDom->getChildAt(0);
    wstringstream lPathStream(aPathString.c_str());
    wstring lPathComponent;
    while(getline(lPathStream, lPathComponent, L'.')) {
        if(ObjectNode *lCurObjectNode = dynamic_cast<ObjectNode*>(lCurNode)) {
            int lMemberIdx = lCurObjectNode->getIndexOfMemberWithName(lPathComponent);
            if(lMemberIdx < 0) {
                return false;
            }
            aRet.push_back(lMemberIdx);
            lCurNode = lCurObjectNode->getChildAt(lMemberIdx);
        } else
            if(ArrayNode *lCurArrayNode = dynamic_cast<ArrayNode*>(lCurNode)) {
                wchar_t * p;
                int lArrayIndex = (int)wcstol(lPathComponent.c_str(), &p, 10);
                if(*p != 0) {
                    return false;
                }
                aRet.push_back(lArrayIndex);
                lCurNode = lCurArrayNode->getChildAt((int)lArrayIndex);
            } else {
                return false;
            }
    }
    
    return true;
}

bool JsonFile::findPathContaining(TextCoordinate aDocOffset, JsonPath &path) const
{
    return findNodeContaining(aDocOffset, &path) != nullptr;
}

const json::Node *JsonFile::findNodeContaining(TextCoordinate aDocOffset, JsonPath *path, bool strict) const
{
    const json::Node *ret = jsonDom->getChildAt(0);
    if(!ret) {
        return NULL;
    }
    
    TextRange rootRange = ret->getTextRange();
    if(rootRange.start > aDocOffset || rootRange.end <= aDocOffset ||
       (strict && rootRange.start == aDocOffset) ) {
        return NULL;
    }
    
    const ContainerNode *lCurElem = dynamic_cast<const ContainerNode*>(ret);
    
    while( lCurElem )
    {
        aDocOffset = aDocOffset.relativeTo(lCurElem->getTextRange().start);
        
        int lNextNav = lCurElem->findChildContaining(aDocOffset, strict);
        
        if(lNextNav<0)
            return ret;
        
        if(path) {
            path->push_back(lNextNav);
        }
        
        ret = lCurElem->getChildAt(lNextNav);
        lCurElem = dynamic_cast<const ContainerNode*>(ret);
    }
    
    return ret;
}

bool JsonFile::attemptReparseClosure(Node *spliceContainer,
                                     TextCoordinate aOffsetStart,
                                     TextLength aLen,
                                     TextLength maxParsedRegionLength,
                                     const std::wstring &aNewText,
                                     TextRange *outAbsReparseRange,
                                     Node **outParsedNode,
                                     MarkerList<ParseErrorMarker> *outReparseErrors) {
    
    // Ensure integral node indeed contains the entire changed region and
    // that it's not too long
    *outAbsReparseRange = spliceContainer->getAbsTextRange().intersectWith(this->getDom()->textRange);
    const TextRange &absReparseRange = *outAbsReparseRange;
    if(absReparseRange.start > aOffsetStart ||
       absReparseRange.end < aOffsetStart + aLen ||
       absReparseRange.length() > maxParsedRegionLength) {
        return false;
    }
    
    
    // Construct updated JSON for node
    std::wstring updatedJsonRegion = this->jsonText->substr(absReparseRange.start, absReparseRange.length());
    updatedJsonRegion.insert(aOffsetStart - absReparseRange.start, aNewText);
    updatedJsonRegion.erase(aOffsetStart - absReparseRange.start + aNewText.length(), aLen);
    
    // Re-parse updated JSON
    MarkerList<ParseErrorMarker> reparseErrors;
    JsonParseErrorCollectionListenerListener listener(reparseErrors);
    
    stopwatch repraseStopWatch("Reparse Json");
    Node *reparsedNode = NULL;
    Reader::Read(reparsedNode, updatedJsonRegion, &listener);
    repraseStopWatch.stop();
    
    if(!reparsedNode) {
        return false;
    }
    
    // Iterate through errors and fixup addresses to match real offset.
    // Fallback to slow parse if there are errors that indicate that the entire region
    // is incomplete or there's further content to process (which means that we need to parse
    // a bigger region than we expected, e.g because the change modifies node boundaries
    bool predictedChangeRegionWrong = false;
    for_each(reparseErrors.begin(), reparseErrors.end(), [&predictedChangeRegionWrong, &absReparseRange](ParseErrorMarker &error) {
        error.adjustCoordinate(absReparseRange.start.getAddress());
        if(error.getErrorCode() == PARSER_ERROR_EXPECTED_EOS || error.getErrorCode() == PARSER_ERROR_UNEXPECTED_EOS) {
            predictedChangeRegionWrong = true;
        }
    });
    if(predictedChangeRegionWrong) {
        if(reparsedNode) {
            delete reparsedNode;
        }
        return false;
    }
    
    *outParsedNode = reparsedNode;
    *outReparseErrors = std::move(reparseErrors);
    return true;
}

void JsonFile::minimizeChangedRegion(TextCoordinate aOffsetStart,
                                     TextLength aLen,
                                     const std::wstring &aNewText,
                                     TextCoordinate *outMinimizedStart,
                                     TextLength *outMinimizedLen,
                                     std::wstring *outMinimizedUpdatedText
                                     ) {
    // Adjust splice range to not include non-modified regions.
    // E.g on MacOS the text system my specific a longer range
    // than actually changed.
    unsigned long newTextLen = aNewText.length();
    unsigned long trimLeft = 0;
    while(trimLeft < aLen &&
          trimLeft < newTextLen &&
          aNewText[trimLeft] == (*jsonText)[aOffsetStart + trimLeft]) {
        trimLeft++;
    }
    
    unsigned long trimRight = 0;
    while(trimRight < (aLen-trimLeft) &&
          trimRight < (newTextLen-trimLeft) &&
          aNewText[newTextLen - 1 - trimRight] == (*jsonText)[aOffsetStart + aLen - 1 - trimRight]) {
        trimRight++;
    }
    
    *outMinimizedStart = aOffsetStart + trimLeft;
    *outMinimizedLen = aLen - (trimLeft + trimRight);
    *outMinimizedUpdatedText = aNewText.substr(trimLeft, newTextLen-trimRight-trimLeft);
}

bool JsonFile::fastSpliceTextWithWorkLimit(TextCoordinate aOffsetStart,
                                           TextLength aLen,
                                           const std::wstring &aNewText,
                                           int maxParsedRegionLength) {
    stopwatch spliceStopWatch("Fast spliceText");
    
    TextCoordinate trimmedStart;
    TextLength trimmedLen;
    std::wstring trimmedUpdatedText;
    minimizeChangedRegion(aOffsetStart, aLen, aNewText,
                          &trimmedStart, &trimmedLen, &trimmedUpdatedText);
    
    TextLength trimmedUpdatedTextLength = trimmedUpdatedText.length();
    
    
    // Locate integral node that contains the entire changed region
    JsonPath integralNodeJsonPath;
    Node *spliceContainer = jsonDom->getChildAt(0);
    ContainerNode *spliceContainerAsContainerNode;
    TextCoordinate soughtOffset = trimmedStart;
    
    while( (spliceContainerAsContainerNode = dynamic_cast<ContainerNode*>(spliceContainer)) != NULL )
    {
        soughtOffset = soughtOffset.relativeTo(spliceContainer->getTextRange().start);
        
        stopwatch fcc("FindChildContaining ");
        int lNextNav = spliceContainerAsContainerNode->findChildContaining(soughtOffset, false);
        fcc.stop();
        if(lNextNav<0)
            break;
        
        
        Node *nextNavNode = spliceContainerAsContainerNode->getChildAt(lNextNav);
        if(nextNavNode->getTextRange().end < soughtOffset+aLen) {
            break;
        }
        
        spliceContainer = nextNavNode;
        
        integralNodeJsonPath.push_back(lNextNav);
    }
    
    // Attempt reparsing growing containers until
    // succesful or over a threshold of attempts
    TextRange absReparseRange;
    Node *reparsedNode = NULL;
    MarkerList<ParseErrorMarker> reparseErrors;
    while(!reparsedNode && spliceContainer && spliceContainer != jsonDom->getChildAt(0) ) {
        if(!attemptReparseClosure(spliceContainer, trimmedStart, trimmedLen, maxParsedRegionLength, trimmedUpdatedText, &absReparseRange, &reparsedNode, &reparseErrors)) {
            reparsedNode = NULL;
            spliceContainer=spliceContainer->getParent();
            integralNodeJsonPath.pop_back();
        }
    }
    
    if(!reparsedNode) {
        return false;
    }
    
    // Fixup trees and lines
    TextCoordinate lLineChangeStart;
    TextLength lLineChangeLen, lLineChangeNewLen;
    
    jsonText->insert(trimmedStart, trimmedUpdatedText);
    jsonText->erase(trimmedStart.getAddress() + trimmedUpdatedTextLength, trimmedLen);
    updateTreeOffsetsAfterSplice(trimmedStart, trimmedLen, trimmedUpdatedTextLength);
    updateLineOffsetsAfterSplice(trimmedStart, trimmedLen, trimmedUpdatedTextLength,
                                 trimmedUpdatedText.c_str(), &lLineChangeStart,
                                 &lLineChangeLen, &lLineChangeNewLen);
    updateErrorsAfterSplice(absReparseRange.start, absReparseRange.length(), absReparseRange.length()+trimmedUpdatedTextLength-trimmedLen, &reparseErrors);
    
    ContainerNode *spliceContainerContainer = spliceContainer->getParent();
    
    // Update node addresses to be relative to parent
    long offsetAdjust = (absReparseRange.start - spliceContainerContainer->getAbsTextRange().start);
    reparsedNode->textRange.start += offsetAdjust;
    reparsedNode->textRange.end += offsetAdjust;
    
    int spliceContainerIndexInParent = spliceContainer->getParent()->getIndexOfChild(spliceContainer);
    spliceContainerContainer->setChildAt(spliceContainerIndexInParent, reparsedNode, true);
    
    // Notify
    notify(NodeRefreshNotification(std::move(integralNodeJsonPath)));
    notify(SpliceNotification(aOffsetStart, aLen, trimmedUpdatedTextLength, lLineChangeStart, lLineChangeLen, lLineChangeNewLen));
    
    return true;
}

JsonFileSemanticModelReconciliationTask::JsonFileSemanticModelReconciliationTask(std::shared_ptr<std::wstring> text)
:  newText(text),
parsedNode(NULL),
errorCollectionListener(new JsonParseErrorCollectionListenerListener(errors)),
inputStream(new InputStream(text->c_str(), text->size(), errorCollectionListener.get())),
tokenStream(new TokenStream(*inputStream, errorCollectionListener.get())),
cancelled(false)
{
}


void JsonFileSemanticModelReconciliationTask::cancelExecution() {
    cancelled = true;
    TextLength len = inputStream->length();
    if(len) {
        // We don't seek to EOS because this could mess with races between
        // time of EOS check and character access. This will abort soon enough.
        inputStream->seek(len-1);
    }
}

void JsonFileSemanticModelReconciliationTask::executeInBackground() {
    errors.clear();
    
    try {
        stopwatch lStopWatch("Read Json");
        Reader reader(errorCollectionListener.get());
        reader.Parse(parsedNode, *tokenStream, false);
        lStopWatch.stop();
    } catch(...) {
        if(!cancelled) {
            throw;
        }
    }
    
    if(cancelled) {
        throw ParseCancelledException();
    }
}


size_t JsonFile::spliceJsonTextContent(TextCoordinate aOffsetStart,
                                       TextLength aLen,
                                       const std::wstring &aNewText) {
    stopwatch lSpliceTime("spliceJsonTextContent");
    
    size_t lNewLen = aNewText.length();
    
    // Create new string if there's a pending reconciliation task since that
    // may actually look at the current string.
    if(pendingReconciliationTask) {
        jsonText = make_shared<std::wstring>(jsonText->substr(0, aOffsetStart) + aNewText + jsonText->substr(aOffsetStart+aLen));
    } else {
        // Update text and line lengths
        jsonText->insert(aOffsetStart, aNewText);
        jsonText->erase(aOffsetStart+lNewLen, aLen);
    }
    
    lSpliceTime.lap("Text update");
    
    TextCoordinate lLineDelStart;
    TextLength lLineDelLen, lNumNewLines;
    updateLineOffsetsAfterSplice(aOffsetStart, aLen, lNewLen, aNewText.c_str(), &
                                 lLineDelStart, &lLineDelLen, &lNumNewLines);
    
    updateErrorsAfterSplice(aOffsetStart, aLen, lNewLen);
    
    lSpliceTime.lap("Markers update");
    
    notify(SpliceNotification(aOffsetStart, aLen, lNewLen, lLineDelStart, lLineDelLen, lNumNewLines));
    
    return lNewLen;
}

shared_ptr<JsonFileSemanticModelReconciliationTask> JsonFile::spliceTextWithDirtySemanticModel(TextCoordinate aOffsetStart,
                                                                                               TextLength aLen,
                                                                                               const std::wstring &aNewText) {
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(this);
    
    spliceJsonTextContent(aOffsetStart, aLen, aNewText);
    
    pendingReconciliationTask = make_shared<JsonFileSemanticModelReconciliationTask>(jsonText);
    return pendingReconciliationTask;
}

void JsonFile::spliceJsonTextByDomChange(TextCoordinate aOffsetStart, TextLength aLen, const std::wstring &aNewText)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(this);
    
    size_t lNewLen = spliceJsonTextContent(aOffsetStart, aLen, aNewText);
    
    // Update tree offsets
    updateTreeOffsetsAfterSplice(aOffsetStart, aLen, lNewLen);
}


void JsonFile::applyReconciliationTask(shared_ptr<JsonFileSemanticModelReconciliationTask> task) {
    if(pendingReconciliationTask == task) {
        errors = task->errors;
        
        jsonDom.reset(new DocumentNode(this, task->parsedNode));
        jsonDom->textRange.start = TextCoordinate(0);
        jsonDom->textRange.end = TextCoordinate(jsonText->length());
        
        notify(ErrorsChangedNotification());
        
        pendingReconciliationTask.reset();
    }
}


void JsonFile::updateLineOffsetsAfterSplice(TextCoordinate aOffsetStart,
                                            TextLength aLen,
                                            TextLength aNewLen,
                                            const wchar_t *aUpdatedText,
                                            TextCoordinate *aOutLineDelStart,
                                            TextLength *aOutLineDelLen,
                                            TextLength *aOutNumNewLines)
{
    SimpleMarkerList newLines;
    
    const wchar_t *updateEnd = aUpdatedText + aNewLen;
    
    for(const wchar_t *cur = aUpdatedText; cur != updateEnd; cur++) {
        if(*cur == L'\n') {
            newLines.appendMarker(aOffsetStart + (unsigned long)(cur - aUpdatedText) );
        }
    }
    
    int lLineDelStart, lLineDelLen;
    if(lineStarts.spliceCoordinatesList(aOffsetStart, aLen, aNewLen,
                                        &newLines, // TODO
                                        &lLineDelStart,
                                        &lLineDelLen))
    {
        *aOutLineDelStart = (TextCoordinate)lLineDelStart;
        *aOutLineDelLen = (TextLength)lLineDelLen;
        *aOutNumNewLines = (TextLength)newLines.size();
    } else {
        *aOutLineDelStart = (TextCoordinate)0;
        *aOutLineDelLen = 0;
        *aOutNumNewLines = 0;
    }
}

void JsonFile::notifyUpdatedNode(Node *updatedNode) {
    // Notify that the semantic model was updated
    JsonPath nodePath;
    Node *lastSought = updatedNode;
    ContainerNode *currentContainer = updatedNode->getParent();
    while(currentContainer && currentContainer != jsonDom.get()) {
        nodePath.push_front(currentContainer->getIndexOfChild(lastSought));
        lastSought = currentContainer;
        currentContainer = lastSought -> getParent();
    }
    
    notify(NodeRefreshNotification(std::move(nodePath)));
}

void JsonFile::updateTreeOffsetsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen)
{
    // Update offsets in json tree; start in top most level
    long lLenDiff = aNewLen - aLen;
    ContainerNode *lCurContainer = jsonDom.get();
    ContainerNode *lNextContainer = NULL;
    jsonDom->textRange.end += lLenDiff;
    
    TextCoordinate lChangedOffset = aOffsetStart;
    do
    {
        lChangedOffset = lChangedOffset.relativeTo(lCurContainer->getTextRange().start);
        int lChildCount = lCurContainer->getChildCount();
        
        // First first child that ends after change start
        int lCurProcessChild = lCurContainer->findChildEndingAfter(lChangedOffset);
        if(lCurProcessChild==-1)
        {
            lNextContainer=NULL;
            break;
        }
        
        // Check if first child intersects with change offset
        Node *lNode = lCurContainer->getChildAt(lCurProcessChild);
        if(lNode->getTextRange().start > lChangedOffset)
        {
            // No intersection - we don't need to look in additional inner containers
            lNextContainer = NULL;
        } else
        {
            lNode->textRange.end += lLenDiff;
            
            if(aLen==0 && lNode->getTextRange().start == lChangedOffset)
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
            lCurContainer->adjustChildRangeAt(lCurProcessChild, lLenDiff);
            lCurProcessChild++;
        }
        
        lCurContainer = lNextContainer;
    } while(lCurContainer);
}



void JsonFile::updateErrorsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen, MarkerList<ParseErrorMarker> *aNewMarkers)
{
    if(errors.spliceCoordinatesList(aOffsetStart, aLen, aNewLen, aNewMarkers))
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

wstring jsonizeString(const wstring &aSrc)
{
    StringNode lNode(aSrc);
    wstring lRet;
    lNode.calculateJsonTextRepresentation(lRet);
    return lRet;
}


TextCoordinate getContainerStartColumnAddr(const json::ContainerNode *containerNode) {
    const json::ObjectNode *containerContainerObj = dynamic_cast<const json::ObjectNode *>(containerNode->getParent());
    if(containerContainerObj) {
        int idx = containerContainerObj->getIndexOfChild(containerNode);
        
        return (containerContainerObj->getAbsTextRange().start +
                containerContainerObj->getChildMemberAt(idx)->nameRange.start.getAddress());
    } else {
        return containerNode->getAbsTextRange().start;
    }
}

void JsonFile::getCoordinateRowCol(TextCoordinate aCoord, int &aRow, int &aCol) const
{
    unsigned long coordAddress = aCoord.getAddress();
    if(coordAddress)
    {
        SimpleMarkerList::const_iterator iter = lower_bound(lineStarts.begin(), lineStarts.end(), aCoord);
        
        if(iter == lineStarts.begin())
        {
            aRow = 1;
            aCol = (int)(coordAddress + 1);
            return;
        }
        
        aRow = (int)(iter - lineStarts.begin() + 1);
        iter--;
        aCol = (int)(aCoord - *iter);
    } else
    {
        aRow = 1;
        aCol = 1;
    }
}

TextCoordinate JsonFile::getLineStart(unsigned long aRow) const {
    if(aRow <= 1) {
        return TextCoordinate(0);
    } else
        if(aRow-2 > lineStarts.size()) {
            return TextCoordinate::infinity;
        } else {
            return (TextCoordinate)lineStarts[(int)(aRow-2)];
        }
}

TextCoordinate JsonFile::getLineFirstCharacter(unsigned long aRow) const {
    return getLineStart(aRow) + (aRow>1 ? 1 : 0);
}

TextCoordinate JsonFile::getLineEnd(unsigned long aRow) const {
    if(aRow-1 < lineStarts.size()) {
        return TextCoordinate(lineStarts[(int)(aRow-1)].getCoordinate()-1);
    } else {
        return (TextCoordinate)jsonText->length();
    }
}


}

