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


wstring Node::GetDocumentText() const
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


void ContainerNode::SetChildAt(int aIdx, Node *aNode, bool fromReparse)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());
    
    // Get old node
    Node *lOldNode = GetChildAt(aIdx);
    
    // Get new node text and range
    if(!fromReparse) {
        TextCoordinate lNewStart = lOldNode->textRange.start;
        TextCoordinate lNewEnd;

        std::wstring lNewNodeText;
        aNode->CalculateJsonTextRepresentation(lNewNodeText);
    
        lNewEnd = lNewStart + (int)lNewNodeText.length();

        aNode->textRange.start = lNewStart;
        aNode->textRange.end = lNewEnd;

        // Splice text in document
        TextRange lAbsTextRange = lOldNode->GetAbsTextRange();
        GetDocument()->GetOwner()->spliceJsonTextByDomChange(lAbsTextRange.start, lAbsTextRange.length(), lNewNodeText);
        GetDocument()->GetOwner()->notifyUpdatedNode(this);
    }
    
    // Update in child list and import to document
    StoreChildAt(aIdx, aNode);
    aNode->parent = this;
}

int ContainerNode::FindChildContaining(const TextCoordinate &aDocOffset, bool strict) const
{
    int lRet = FindChildEndingAfter(aDocOffset);
    if(lRet>=0)
    {
        TextRange tr = GetChildAt(lRet)->GetTextRange();
        if(tr.start < aDocOffset || (!strict && tr.start == aDocOffset)) {
            return lRet;
        }
    }
    
    return -1;
}

int ContainerNode::GetIndexOfChild(const Node *aChild) const
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


void ContainerNode::AdjustChildRangeAt(int aIdx, long aDiff)
{
    Node *lNode = GetChildAt(aIdx);
    
    lNode->textRange.end += aDiff;
    lNode->textRange.start += aDiff;
}

bool ContainerNode::ValueEquals(Node *other) const {
    ContainerNode *otherCont = dynamic_cast<ContainerNode*>(other);
    if(!otherCont) {
        return false;
    }
    
    if(otherCont->GetChildCount() != GetChildCount()) {
        return false;
    }
    
    for(int idx=0; idx<GetChildCount(); idx++) {
        if(!GetChildAt(idx)->ValueEquals(otherCont->GetChildAt(idx))) {
            return false;
        }
    }
    
    return true;
}

bool ContainerNode::ValueLt(Node *other) const {
    return false;
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
    return (int)elements.size();
}

Node *ArrayNode::GetChildAt(int aIdx)
{
    return elements[aIdx].get();
}

const Node *ArrayNode::GetChildAt(int aIdx) const
{
    return elements[aIdx].get();
}

void ArrayNode::StoreChildAt(int aIdx, Node *aNode) {
    elements[aIdx].reset(aNode);
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
    
    *aNode = lIter->release();
    (*aNode)->parent = NULL;
    
    elements.erase(lIter);
    
    GetDocument()->GetOwner()->spliceJsonTextByDomChange(lSpliceRange.start, lSpliceRange.length(), wstring(L""));
    GetDocument()->GetOwner()->notifyUpdatedNode(this);
}

void ArrayNode::InsertMemberAt(int aIdx, Node *aElement, const wstring *aElementText)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());
    
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
        aElement->CalculateJsonTextRepresentation(lTxt);
        lElementTextLen = lTxt.length();
        lCalculatedText += lTxt;
    }
    
    // Add comma if not last element
    if(!lIsLast)
    {
        lCalculatedText += L", ";
    }
    
    // Update document text
    GetDocument()->GetOwner()->spliceJsonTextByDomChange(GetAbsTextRange().start+lChangeAddr, 0, lCalculatedText);
    GetDocument()->GetOwner()->notifyUpdatedNode(this);
    
    // Add element to sequence and setup address
    elements.insert(lIter, std::unique_ptr<Node>(aElement));
    
    aElement->textRange.start = TextCoordinate(lElemAddr);
    aElement->textRange.end = TextCoordinate(lElemAddr + (unsigned int)lElementTextLen);
}

struct ArrayMemberTextRangeOrdering 
{
    bool operator()(const std::unique_ptr<Node> &aLeft, const std::unique_ptr<Node> &aRight) const
    {
        return aLeft->GetTextRange().end < aRight->GetTextRange().end;
    }
    
    bool operator() (std::unique_ptr<Node> const &aLeft, TextCoordinate aRight) const
    {
        return aLeft->GetTextRange().end < aRight;
    }
};


int ArrayNode::FindChildEndingAfter(const TextCoordinate &aDocOffset) const
{
    const_iterator lContainingElement = lower_bound(elements.begin(), elements.end(), aDocOffset+1, ArrayMemberTextRangeOrdering());
    
    // Found member ending past offset?
    if(lContainingElement == elements.end())
        return -1;
    
    return (int)(lContainingElement - elements.begin());
}


void ArrayNode::DomAddElementNode(Node *aElement)
{
    elements.push_back(std::unique_ptr<Node>(aElement));
    aElement->parent = this;
}

NodeTypeId ArrayNode::GetNodeTypeId() const
{
    return ntArray;
}

void ArrayNode::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = L"[ ";
    for(auto iter = elements.begin(); iter != elements.end(); ) {
        std::wstring fragment;
        iter->get()->CalculateJsonTextRepresentation(fragment, maxLenHint);
        
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
    return (int)members.size();
}

Node *ObjectNode::GetChildAt(int aIdx)
{
    return members[aIdx].node.get();
}

const Node *ObjectNode::GetChildAt(int aIdx) const
{
    return members[aIdx].node.get();
}

int ObjectNode::GetIndexOfMemberWithName(const wstring &name) const {
    for(Members::const_iterator iter = members.begin();
        iter != members.end();
        iter++) {
        if(iter->name == name) {
            return (int)(iter - members.begin());
        }
    }
    
    return -1;
}

const ObjectNode::Member *ObjectNode::GetChildMemberAt(int aIdx) const
{
    return &members[aIdx];
}

void ObjectNode::StoreChildAt(int aIdx, Node *aNode) {
    members[aIdx].node.reset(aNode);
}

void ObjectNode::AdjustChildRangeAt(int aIdx, long aDiff)
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
    
    bool operator() (const ObjectNode::Member &aLeft, TextCoordinate aRight) const
    {
        return aLeft.node->GetTextRange().end < aRight;
    }
};

int ObjectNode::FindChildEndingAfter(const TextCoordinate &aDocOffset) const
{
    const_iterator lContainingElement = lower_bound(members.begin(), members.end(), aDocOffset+1, ObjectMemberTextRangeOrdering());
    
    // Found member ending past offset?
    if(lContainingElement == members.end())
        return -1;
    
    //   printf("Child ending after %d is '%s\n", aDocOffset, lContainingElement->name.c_str());
    return (int)(lContainingElement - members.begin());
}

const wstring &ObjectNode::GetMemberNameAt(int aIdx) const
{
    return members[aIdx].name;
}

void ObjectNode::RenameMemberAt(int aIdx, const wstring &aName) {
    TextRange orgRange = members[aIdx].nameRange;
    std::wstring orgName = members[aIdx].name;
    
    std::wstring jsonizedName = jsonizeString(aName);
    TextRange myAbs = GetAbsTextRange();
    members[aIdx].name = aName;
    GetDocument()->GetOwner()->spliceJsonTextByDomChange(orgRange.start + myAbs.start, orgRange.length(),
                                                         jsonizedName);
    GetDocument()->GetOwner()->notifyUpdatedNode(this);

    
    // A bit of a hack:
    // Fix name range; it it was changed wrongly by spliceJsonTextByDomChange above.
    members[aIdx].nameRange.start = orgRange.start;
    members[aIdx].nameRange.end = members[aIdx].nameRange.start + (int)jsonizedName.length();
}

ObjectNode::Member &ObjectNode::InsertMemberAt(int aIdx, const wstring &aName, Node *aElement, const wstring *aElementText)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(GetDocument()->GetOwner());
    
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
        aElement->CalculateJsonTextRepresentation(lTxt);
        lCalculatedText += lTxt;
        lElementTextLen = lTxt.length();
    }
    
    if(!lIsLast)
    {
        lCalculatedText += L", ";
    }
    
    // Update document text
    GetDocument()->GetOwner()->spliceJsonTextByDomChange(GetAbsTextRange().start+lChangeAddr, 0, lCalculatedText);
    GetDocument()->GetOwner()->notifyUpdatedNode(this);
    
    // Add element and fixup addresses after splicing
    aElement->textRange.start = TextCoordinate(lElemAddr);
    aElement->textRange.end = TextCoordinate(lElemAddr + lElementTextLen);
    lMember.nameRange.start = TextCoordinate(lNameAddr);
    lMember.nameRange.end = TextCoordinate(lNameAddr + lNameLen);
    
    members.insert(lIter, std::move(lMember));
    
    return *lIter;
}

ObjectNode::Member &ObjectNode::DomAddMemberNode(const wstring &aName, Node *aElement)
{
    Member lMemberInfo;
    lMemberInfo.name = aName;
    lMemberInfo.node.reset(aElement);
    lMemberInfo.node->parent = this;
    
    members.push_back(std::move(lMemberInfo));
    
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
        lSpliceRange.end = lSpliceRange.end - 1;
    }
    
    *aNode = lIter->node.release();
    (*aNode)->parent = NULL;
    
    members.erase(lIter);
    
    GetDocument()->GetOwner()->spliceJsonTextByDomChange(lSpliceRange.start, lSpliceRange.length(), L"");
    GetDocument()->GetOwner()->notifyUpdatedNode(this);
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

bool ObjectNode::ValueEquals(Node *other) const {
    ObjectNode *objOtherNode = dynamic_cast<ObjectNode*>(other);
    if(!objOtherNode) {
        return false;
    }
    
    if(!ContainerNode::ValueEquals(other)) {
        return false;
    }
    
    int childCount = GetChildCount();
    for(int idx=0; idx<childCount; idx++) {
        if(GetMemberNameAt(idx) != objOtherNode->GetMemberNameAt(idx)) {
            return false;
        }
    }
    
    return true;
}

void ObjectNode::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = L"{ ";
    for(auto iter = members.begin(); iter != members.end(); ) {
        aDest += L"\"" + iter->name + L"\": ";
        
        std::wstring fragment;
        iter->node->CalculateJsonTextRepresentation(fragment, maxLenHint);
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
    return rootNode.get();
}

const Node *DocumentNode::GetChildAt(int aIdx) const
{
    return rootNode.get();
}

void DocumentNode::StoreChildAt(int aIdx, Node *aNode)
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

void DocumentNode::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
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

void NullNode::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    aDest = L"null";
}

bool NullNode::ValueEquals(Node *other) const {
    return !!dynamic_cast<NullNode*>(other);
}

bool NullNode::ValueLt(Node *other) const {
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



template<> 
void ValueNode<std::wstring, ntString>::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
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
void ValueNode<double, ntNumber>::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
{
    wstringstream lStream;
    
    lStream << std::setprecision(20) << value;
    
    aDest = lStream.str();
}

template<> 
void ValueNode<bool, ntBoolean>::CalculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint) const
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
: notificationsDeferred(0), jsonDom(new DocumentNode(this, new NullNode()))
{
}


void JsonFile::setText(const wstring &aText)
{
    JsonParseErrorCollectionListenerListener listener(errors);
    
    jsonText = aText;
    wstringstream lStream(aText);
    
    errors.clear();
    
    stopwatch lStopWatch("Read Json");
    Node *lNode = NULL;
    Reader::Read(lNode, lStream, &listener);
    lStopWatch.stop();
    
    jsonDom.reset(new DocumentNode(this, lNode));
    jsonDom->textRange.start = TextCoordinate(0);
    jsonDom->textRange.end = TextCoordinate(aText.length());
    
    notify(ErrorsChangedNotification());
}

const wstring &JsonFile::getText() const
{
    return jsonText;
}

DocumentNode *JsonFile::getDom()
{
    return jsonDom.get();
}

const DocumentNode *JsonFile::getDom() const
{
    return jsonDom.get();
}

bool JsonFile::FindPathForJsonPathString(std::wstring aPathString, JsonPath &aRet) {
    Node *lCurNode = jsonDom->GetChildAt(0);
    wstringstream lPathStream(aPathString.c_str());
    wstring lPathComponent;
    while(getline(lPathStream, lPathComponent, L'.')) {
        if(ObjectNode *lCurObjectNode = dynamic_cast<ObjectNode*>(lCurNode)) {
            int lMemberIdx = lCurObjectNode->GetIndexOfMemberWithName(lPathComponent);
            if(lMemberIdx < 0) {
                return false;
            }
            aRet.push_back(lMemberIdx);
            lCurNode = lCurObjectNode->GetChildAt(lMemberIdx);
        } else
            if(ArrayNode *lCurArrayNode = dynamic_cast<ArrayNode*>(lCurNode)) {
                wchar_t * p;
                int lArrayIndex = (int)wcstol(lPathComponent.c_str(), &p, 10);
                if(*p != 0) {
                    return false;
                }
                aRet.push_back(lArrayIndex);
                lCurNode = lCurArrayNode->GetChildAt((int)lArrayIndex);
            } else {
                return false;
            }
    }
    
    return true;
}

bool JsonFile::FindPathContaining(TextCoordinate aDocOffset, JsonPath &path) const
{
    return FindNodeContaining(aDocOffset, &path) != nullptr;
}

const json::Node *JsonFile::FindNodeContaining(TextCoordinate aDocOffset, JsonPath *path, bool strict) const
{
    const json::Node *ret = jsonDom->GetChildAt(0);
    if(!ret) {
        return NULL;
    }
    
    TextRange rootRange = ret->GetTextRange();
    if(rootRange.start > aDocOffset || rootRange.end <= aDocOffset ||
       (strict && rootRange.start == aDocOffset) ) {
        return NULL;
    }
    
    const ContainerNode *lCurElem = dynamic_cast<const ContainerNode*>(ret);
    
    while( lCurElem )
    {
        aDocOffset = aDocOffset.relativeTo(lCurElem->GetTextRange().start);
        
        int lNextNav = lCurElem->FindChildContaining(aDocOffset, strict);
        
        if(lNextNav<0)
            return ret;
        
        if(path) {
            path->push_back(lNextNav);
        }
        
        ret = lCurElem->GetChildAt(lNextNav);
        lCurElem = dynamic_cast<const ContainerNode*>(ret);
    }
    
    return ret;
}

bool JsonFile::spliceTextWithWorkLimit(TextCoordinate aOffsetStart,
                                       TextLength aLen,
                                       const std::wstring &aNewText,
                                       int maxParsedRegionLength) {
    stopwatch spliceStopWatch("Fast spliceText");

    // Adjust splice range to not include non-modified regions.
    // E.g on MacOS the text system my specific a longer range
    // than actually changed.
    unsigned long newTextLen = aNewText.length();
    unsigned long trimLeft = 0;
    while(trimLeft < aLen &&
          trimLeft < newTextLen &&
          aNewText[trimLeft] == jsonText[aOffsetStart + trimLeft]) {
        trimLeft++;
    }
    
    unsigned long trimRight = 0;
    while(trimRight < (aLen-trimLeft) &&
          trimRight < newTextLen &&
          aNewText[newTextLen - 1 - trimRight] == jsonText[aOffsetStart + aLen - 1 - trimRight]) {
        trimRight++;
    }
    aOffsetStart += trimLeft;
    aLen -= (trimLeft + trimRight);

    // Locate integral node that contains the entire changed region
    JsonPath integralNodeJsonPath;
    Node *spliceContainer = jsonDom->GetChildAt(0);
    ContainerNode *spliceContainerAsContainerNode;
    int spliceContainerIndexInParent = 0;
    TextCoordinate soughtOffset = aOffsetStart;
    while( (spliceContainerAsContainerNode = dynamic_cast<ContainerNode*>(spliceContainer)) != NULL )
    {
        soughtOffset = soughtOffset.relativeTo(spliceContainer->GetTextRange().start);
        
        int lNextNav = spliceContainerAsContainerNode->FindChildContaining(soughtOffset, false);
        if(lNextNav<0)
            break;

        Node *nextNavNode = spliceContainerAsContainerNode->GetChildAt(lNextNav);
        if(nextNavNode->GetTextRange().end < soughtOffset+aLen) {
            break;
        }
        
        spliceContainer = nextNavNode;
                
        integralNodeJsonPath.push_back(lNextNav);
        spliceContainerIndexInParent = lNextNav;
    }
    
    
    if(!spliceContainer || spliceContainer == jsonDom->GetChildAt(0)) {
        return false;
    }
    
    
    // Ensure integral node indeed contains the entire changed region and
    // that it's not too long
    TextRange absRange = spliceContainer->GetAbsTextRange().intersectWith(this->getDom()->textRange);
    if(absRange.start > aOffsetStart ||
       absRange.end < aOffsetStart + aLen ||
       absRange.length() > maxParsedRegionLength) {
        return false;
    }
    
    // Construct updated JSON for node
    size_t updatedTextLen = newTextLen-trimRight-trimLeft;
    std::wstring updatedJsonRegion = this->jsonText.substr(absRange.start, absRange.length());
    updatedJsonRegion.insert(aOffsetStart - absRange.start, aNewText, trimLeft, updatedTextLen);
    updatedJsonRegion.erase(aOffsetStart - absRange.start + updatedTextLen, aLen);
    
    // Re-parse updated JSON
    MarkerList<ParseErrorMarker> errors;
    JsonParseErrorCollectionListenerListener listener(errors);
    
    wstringstream reparseStream(updatedJsonRegion);
    
    stopwatch repraseStopWatch("Reparse Json");
    Node *reparsedNode = NULL;
    Reader::Read(reparsedNode, reparseStream, &listener); // TODO error listener
    repraseStopWatch.stop();
    
    if(errors.size()) {
        return false;
    }
    
    jsonText.insert(aOffsetStart, aNewText, trimLeft, updatedTextLen);
    jsonText.erase(aOffsetStart.getAddress() + updatedTextLen, aLen);
    updateTreeOffsetsAfterSplice(aOffsetStart, aLen, updatedTextLen);
    updateErrorsAfterSplice(aOffsetStart, aLen, updatedTextLen);

    ContainerNode *spliceContainerContainer = spliceContainer->GetParent();
    
    // Update node addresses to be relative to parent
    long offsetAdjust = (absRange.start - spliceContainerContainer->GetAbsTextRange().start);
    reparsedNode->textRange.start += offsetAdjust;
    reparsedNode->textRange.end += offsetAdjust;
    
    spliceContainerContainer->SetChildAt(spliceContainerIndexInParent, reparsedNode, true);
    notify(NodeRefreshNotification(std::move(integralNodeJsonPath)));
    
    return true;
}

void JsonFile::spliceJsonTextByDomChange(TextCoordinate aOffsetStart, TextLength aLen, const std::wstring &aNewText)
{
    // Don't send notifications till we're thru
    DeferNotificationsInBlock lDnib(this);
    
    stopwatch lSpliceTime("spliceJsonTextByDomChange");
    
    unsigned long lNewLen = aNewText.length();
    
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

void JsonFile::notifyUpdatedNode(Node *updatedNode) {
    // Notify that the semantic model was updated
    JsonPath nodePath;
    Node *lastSought = updatedNode;
    ContainerNode *currentContainer = updatedNode->GetParent();
    while(currentContainer && currentContainer != jsonDom.get()) {
        nodePath.push_front(currentContainer->GetIndexOfChild(lastSought));
        lastSought = currentContainer;
        currentContainer = lastSought -> GetParent();
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
        lChangedOffset = lChangedOffset.relativeTo(lCurContainer->GetTextRange().start);
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

wstring jsonizeString(const wstring &aSrc)
{
    StringNode lNode(aSrc);
    wstring lRet;
    lNode.CalculateJsonTextRepresentation(lRet);
    return lRet;
}


TextCoordinate getContainerStartColumnAddr(const json::ContainerNode *containerNode) {
    const json::ObjectNode *containerContainerObj = dynamic_cast<const json::ObjectNode *>(containerNode->GetParent());
    if(containerContainerObj) {
        int idx = containerContainerObj->GetIndexOfChild(containerNode);
        
        return (containerContainerObj->GetAbsTextRange().start +
                containerContainerObj->GetChildMemberAt(idx)->nameRange.start.getAddress());
    } else {
        return containerNode->GetAbsTextRange().start;
    }
}


}

