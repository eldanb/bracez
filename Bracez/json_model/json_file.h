/*
 *  json_file.h
 *  JsonMockup
 *
 *  Created by Eldan on 6/5/10.
 *  Copyright 2010 Eldan Ben-Haim. All rights reserved.
 *
 */

#ifndef __json_file__
#define __json_file__


#include <list>
#include <string>
#include <deque>
#include <vector>
#include <stdexcept>
#include "assert.h"
#include "Exception.hpp"
#include "marker_list.h"

namespace json
{

using namespace std;

class NodeVisitor;
class ContainerNode;
class DocumentNode;
class JsonFile;
class InputStream;
class TokenStream;
class ParseListener;
class ObjectNode;

struct TextRange
{
    TextRange() {}
    TextRange(const TextCoordinate &aStart, const TextCoordinate &aEnd) : start(aStart), end(aEnd) {}
    
    bool operator==(const TextRange &other) const {
        return start == other.start && end == other.end;
    }

    bool operator!=(const TextRange &other) const {
        return !(*this == other);
    }

    TextLength length() const { return end-start; }
    
    TextRange intersectWith(TextRange other) const {
        TextRange ret;
        ret.start = start > other.start ? start : other.start;
        ret.end = end < other.end ? end : other.end;
        ret.end = ret.end < ret.start ? ret.start : ret.end;
        
        return ret;
    }
    
    TextRange intersectWithAndRelativeTo(TextRange other) const {
        TextRange ret = intersectWith(other);
        ret.start -= other.start;
        ret.end -= other.start;
        
        return ret;
    }
    
    bool contains(TextRange other) const {
        return start<=other.start && end>=other.end;
    }
    
    TextCoordinate start;   // First character in range
    TextCoordinate end; // One past last charcter in range
};

typedef std::list<int> JsonPath;

enum NodeTypeId {
    ntNull,
    ntNumber,
    ntBoolean,
    ntString,
    ntObject,
    ntArray,
};

class Node
{
public:
    Node() : parent(NULL) {}
    virtual ~Node() {}
    
    const ContainerNode *getParent() const { return parent; }
    ContainerNode *getParent() { return parent; }
    
    DocumentNode *getDocument() const;
    
    const TextRange &getTextRange() const;
    TextRange getAbsTextRange() const;
    
    wstring getDocumentText() const;
    
    virtual void accept(NodeVisitor *aVisitor) const = 0;
    virtual void accept(NodeVisitor *aVisitor) = 0;
    virtual void acceptInRange(NodeVisitor *aVisitor, TextRange &range) = 0;
    
    virtual NodeTypeId getNodeTypeId() const = 0;
    
    virtual void calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint = -1) const = 0;
    
    virtual bool valueEquals(Node *other) const = 0;
    virtual bool valueLt(Node *other) const = 0;
    
    virtual Node *clone() const = 0;
    
    virtual std::wstring toString() const = 0;
    virtual ObjectNode *createDebugRepresentation() const;
    
private:
    friend class Reader;
    friend class ArrayNode;
    friend class ObjectNode;
    friend class ContainerNode;
    friend class DocumentNode;
    friend class JsonFile;
    
protected:
    TextRange textRange;
    
private:
    ContainerNode *parent;
} ;


class ContainerNode : public Node
{
public:
    ContainerNode() : cachedLastRangeFoundChild(-1) {};
    
    void setChildAt(int aIdx, Node *aNode, bool fromReparse = false);
    
    virtual int getChildCount()  const = 0;
    
    virtual Node *getChildAt(int aIdx) = 0;
    virtual const Node *getChildAt(int aIdx) const = 0;
    virtual void removeChildAt(int aIdx);
    
    virtual int getIndexOfChild(const Node *aChild) const;
    
    virtual int findChildEndingAfter(const TextCoordinate &aDocOffset) const = 0;
    virtual int findChildContaining(const TextCoordinate &aDocOffset, bool strict) const;
    
    virtual void detachChildAt(int aIdx, Node **aNode) = 0;
    
    virtual bool valueEquals(Node *other) const;
    bool valueLt(Node *other) const;
    
protected:
    virtual void adjustChildRangeAt(int aIdx, long aDiff);
    virtual void storeChildAt(int aIdx, Node *aNode) = 0;
    
    friend class JsonFile;
    
private:
    mutable int cachedLastRangeFoundChild;
    
} ;

class ArrayNode : public ContainerNode
{
public:
    
    typedef std::deque<std::unique_ptr<Node>> Elements;
    typedef Elements::iterator iterator;
    typedef Elements::const_iterator const_iterator;
    
    iterator begin();
    iterator end();
    
    const_iterator begin() const;
    const_iterator end() const;
    
    int getChildCount() const;
    
    Node *getChildAt(int aIdx);
    const Node *getChildAt(int aIdx) const;
    void detachChildAt(int aIdx, Node **aNode);
    
    int findChildEndingAfter(const TextCoordinate &aDocOffset) const;
    
    void InsertMemberAt(int aIdx, Node *aElement,
                        const wstring *aElementText);
    
    void accept(NodeVisitor *aVisitor) const;
    void accept(NodeVisitor *aVisitor);
    void acceptInRange(NodeVisitor *aVisitor, TextRange &range);
    
    NodeTypeId getNodeTypeId() const;
    
    void calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint = -1) const;
    
    // DOM-only modifiers
    void domAddElementNode(Node *aElement);
    
    virtual std::wstring toString() const {
        return L"[Array]";
    }
    
    virtual ArrayNode *clone() const;
    
    virtual ObjectNode *createDebugRepresentation() const;
    
protected:
    virtual void storeChildAt(int aIdx, Node *aNode);
    
private:
    Elements elements;
};

class ObjectNode : public ContainerNode
{
public:
    
    struct Member
    {
        Member(const wstring &aName, Node *aNode) : node(aNode), name(aName) {}
        Member(wstring &&aName, Node *aNode) : node(aNode), name(std::move(aName)) {}
        Member() : node(nullptr) {}
        
        wstring name;
        TextRange nameRange;
        
        std::unique_ptr<Node> node;
    } ;
    
    typedef std::deque<Member> Members;
    typedef Members::iterator iterator;
    typedef Members::const_iterator const_iterator;
    
    iterator begin();
    iterator end();
    
    const_iterator begin() const;
    const_iterator end() const;
    
    int getChildCount() const;
    
    Node *getChildAt(int aIdx);
    const Node *getChildAt(int aIdx) const;
    const Member *getChildMemberAt(int aIdx) const;
    
    void detachChildAt(int aIdx, Node **aNode);
    ObjectNode::Member &insertMemberAt(int aIdx, const wstring &aName,
                                       Node *aElement, const wstring *aElementText);
    void renameMemberAt(int aIdx, const wstring &aName);
    
    int findChildEndingAfter(const TextCoordinate &aDocOffset) const;
    
    const wstring &getMemberNameAt(int aIdx) const;
    int getIndexOfMemberWithName(const wstring &name) const;
    
    void accept(NodeVisitor *aVisitor) const;
    void accept(NodeVisitor *aVisitor);
    void acceptInRange(NodeVisitor *aVisitor, TextRange &range);
    
    NodeTypeId getNodeTypeId() const;
    
    void calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint = -1) const;
    
    // DOM-only modifiers
    Member &domAddMemberNode(const wstring &aName, Node *aElement);
    Member &domAddMemberNode(wstring &&aName, Node *aElement);
    
    bool valueEquals(Node *other) const;
    
    virtual std::wstring toString() const {
        return L"[Object]";
    }
    
    virtual Node *clone() const;
    
    virtual ObjectNode *createDebugRepresentation() const;
protected:
    void adjustChildRangeAt(int aIdx, long aDiff);
    virtual void storeChildAt(int aIdx, Node *aNode);
    
private:
    Members members;
    
} ;

class DocumentNode : public ContainerNode
{
public:
    DocumentNode(JsonFile *aOwner, Node *aChild);
    
    virtual void accept(NodeVisitor *aVisitor) const;
    virtual void accept(NodeVisitor *aVisitor);
    void acceptInRange(NodeVisitor *aVisitor, TextRange &range);
    
    NodeTypeId getNodeTypeId() const ;
    
    virtual int getChildCount() const;
    
    virtual Node *getChildAt(int aIdx);
    virtual const Node *getChildAt(int aIdx) const;
    void detachChildAt(int aIdx, Node **aNode);
    
    int findChildEndingAfter(const TextCoordinate &aDocOffset) const;
    
    JsonFile *getOwner() const { return owner; }
    
    virtual Node *clone() const { return new DocumentNode(owner, rootNode->clone()); }
    
    void calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint = -1) const;
    
    virtual std::wstring toString() const {
        return L"[Document]";
    }
    
    virtual ObjectNode *createDebugRepresentation() const;
    
protected:
    virtual void storeChildAt(int aIdx, Node *aNode);
    
private:
    std::unique_ptr<Node> rootNode;
    JsonFile *owner;
} ;

class LeafNode : public Node
{
public:
    void accept(NodeVisitor *aVisitor);
    void accept(NodeVisitor *aVisitor) const;
    void acceptInRange(NodeVisitor *aVisitor, TextRange &range);
    
} ;

class NullNode : public LeafNode
{
public:
    NodeTypeId getNodeTypeId() const;
    
    void calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint = -1) const;
    
    bool valueEquals(Node *other) const;
    bool valueLt(Node *other) const;
    
    virtual Node *clone() const { return new NullNode(); }
    
    virtual std::wstring toString() const {
        return L"null";
    }
    
    virtual ObjectNode *createDebugRepresentation() const;
protected:
    
} ;


template<class T> class ValueNodeTraits {};

template<>
class ValueNodeTraits<std::wstring> {
public:
    static const int nodeType = ntString;
};

template<>
class ValueNodeTraits<double> {
public:
    static const int nodeType = ntNumber;
};

template<>
class ValueNodeTraits<bool> {
public:
    static const int nodeType = ntBoolean;
};

template<class ValueType>
class ValueNode : public LeafNode
{
public:
    ValueNode(ValueType &&aData) : value(std::move(aData)) {}
    ValueNode(const ValueType &aData) : value(aData) {}
    
    NodeTypeId getNodeTypeId() const { return (NodeTypeId)ValueNodeTraits<ValueType>::nodeType; }
    
    const ValueType &getValue() const { return value; }
    
    operator ValueType() const { return value; }
    
    void calculateJsonTextRepresentation(std::wstring &aDest, int maxLenHint = -1) const;
    
    bool valueEquals(Node *other) const {
        ValueNode<ValueType> *otherTyped = dynamic_cast< ValueNode<ValueType> *>(other);
        return otherTyped && otherTyped->getValue() == getValue();
    }
    
    bool valueLt(Node *other) const {
        ValueNode<ValueType> *otherTyped = dynamic_cast< ValueNode<ValueType> *>(other);
        return otherTyped && otherTyped->getValue() > getValue();
    }
    
    std::wstring toString() const {
        return computeString(value);
    }
    
    virtual Node *clone() const {
        return new ValueNode(value);
    }
    
    virtual ObjectNode *createDebugRepresentation() const {
        ObjectNode *ret = Node::createDebugRepresentation();
        
        ret->domAddMemberNode(L"type", new ValueNode<std::wstring>(L"value"));
        
        std::wstring valRep;
        calculateJsonTextRepresentation(valRep);
        ret->domAddMemberNode(L"jsonValue", new ValueNode<std::wstring>(valRep));
        
        return ret;
    }
protected:
    
private:
    ValueType value;
    
    template<class T>
    static std::wstring computeString(const T &t) { return std::to_wstring(t); }
    
    template<>
    static std::wstring computeString(const std::wstring &t) { return t; }
    
    template<>
    static std::wstring computeString(const bool &t) { return t ? L"true" : L"false"; }
    
} ;


typedef ValueNode<std::wstring> StringNode;
typedef ValueNode<double> NumberNode;
typedef ValueNode<bool> BooleanNode;

struct JsonFileChangeListener
{
    virtual void notifyTextSpliced(JsonFile *aSender, TextCoordinate aOldOffset,
                                   TextLength aOldLength, TextLength aNewLength,
                                   TextCoordinate aOldLineStart, TextLength aOldLineLength,
                                   TextLength aNewLineLength) = 0;
    virtual void notifyErrorsChanged(JsonFile *aSender) {}
    virtual void notifyNodeChanged(JsonFile *aSender, const JsonPath &nodePath) {}
};


class NodeVisitor
{
public:
    virtual bool visitNode(Node *aNode) = 0;
    virtual bool visitNode(const Node *aNode) = 0;
} ;

class ParseErrorMarker : public BaseMarker
{
public:
    ParseErrorMarker(TextCoordinate aErrorCoord, int aErrorCode, const std::string &aErrorText) :
    BaseMarker(aErrorCoord), errorCode(aErrorCode), errorText(aErrorText)
    {}
    
    const std::string &getErrorText() const { return errorText; }
    int getErrorCode() const  { return errorCode; }
    
private:
    std::string errorText;
    int errorCode;
} ;

namespace priv
{
class Notification
{
public:
    virtual ~Notification() {};
    
    virtual void deliverToListener(JsonFile *aSender, JsonFileChangeListener *aListener) const = 0;
    virtual Notification *clone() const = 0;
} ;

class DeferNotificationsInBlock;
}

class JsonFileSemanticModelReconciliationTask {
public:
    JsonFileSemanticModelReconciliationTask(std::shared_ptr<std::wstring> text);
    
    void executeInBackground();
    void cancelExecution();
    
private:
    shared_ptr<std::wstring> newText;
    MarkerList<ParseErrorMarker> errors;
    Node *parsedNode;
    
    unique_ptr<ParseListener> errorCollectionListener;
    unique_ptr<InputStream> inputStream;
    unique_ptr<TokenStream> tokenStream;
    
    std::atomic<bool> cancelled;
    
    friend class JsonFile;
};

class JsonFile
{
public:
    JsonFile();
    
    void setText(const std::wstring &aText);
    const std::wstring &getText() const;
    
    DocumentNode *getDom();
    const DocumentNode *getDom() const;
    
    bool findPathContaining(TextCoordinate aDocOffset, JsonPath &aRet) const;
    const json::Node *findNodeContaining(TextCoordinate aDocOffset, JsonPath *path, bool strict = false) const;
    
    
    bool findPathForJsonPathString(std::wstring aPathString, JsonPath &aRet);
    
    void addListener(JsonFileChangeListener *aListener);
    void removeListener(JsonFileChangeListener *aListener);
    
    const MarkerList<ParseErrorMarker> &getErrors() { return errors; }
    
    
    bool fastSpliceTextWithWorkLimit(TextCoordinate aOffsetStart,
                                     TextLength aLen,
                                     const std::wstring &aNewText,
                                     int maxParsedRegionLength);
    
    
    shared_ptr<JsonFileSemanticModelReconciliationTask>  spliceTextWithDirtySemanticModel(
                                                                                          TextCoordinate aOffsetStart,
                                                                                          TextLength aLen,
                                                                                          const std::wstring &newText);
    void applyReconciliationTask(shared_ptr<JsonFileSemanticModelReconciliationTask> task);
    
    
    void getCoordinateRowCol(TextCoordinate aCoord, int &aRow, int &aCol) const;
    TextCoordinate getLineStart(unsigned long aRow) const;
    TextCoordinate getLineFirstCharacter(unsigned long aRow) const;
    TextCoordinate getLineEnd(unsigned long aRow) const;
    
    inline unsigned long numLines() const { return lineStarts.size()+1; }
    
    void beginDeferNotifications();
    void endDeferNotifications();

    
private:
    size_t spliceJsonTextContent(TextCoordinate aOffsetStart,
                                 TextLength aLen,
                                 const std::wstring &aNewText);
    
    void spliceJsonTextByDomChange(TextCoordinate aOffsetStart, TextLength aLen, const std::wstring &aNewText);
    void updateLineOffsetsAfterSplice(TextCoordinate aOffsetStart,
                                      TextLength aLen,
                                      TextLength aNewLen,
                                      const wchar_t *updatedText,
                                      TextCoordinate *aOutLineDelStart,
                                      TextLength *aOutLineDelLen,
                                      TextLength *aOutNumNewLines);
    
    void updateTreeOffsetsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen);
    void updateErrorsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen, MarkerList<ParseErrorMarker> *aNewMarkers = NULL);
    
    void notifyUpdatedNode(Node *updatedNode);
    
    void minimizeChangedRegion(TextCoordinate aOffsetStart,
                               TextLength aLen,
                               const std::wstring &aNewText,
                               TextCoordinate *outMinimizedStart,
                               TextLength *outMinimizedLen,
                               std::wstring *outMinimizedUpdatedText);
    
    bool attemptReparseClosure(Node *spliceContainer,
                               TextCoordinate aOffsetStart,
                               TextLength aLen,
                               TextLength maxParsedRegionLength,
                               const std::wstring &aNewText,
                               TextRange *outAbsReparseRange,
                               Node **outParsedNode,
                               MarkerList<ParseErrorMarker> *outReparseErrors);
    
private:
    void notify(const priv::Notification &aNotification);
    
private:
    void deliverNotification(const priv::Notification &aNotification);
    
private:
    friend class JsonFileParseListener;
    friend class ContainerNode;
    friend class ArrayNode;
    friend class ObjectNode;
    friend class Notification;
    friend class priv::DeferNotificationsInBlock;
    
    std::unique_ptr<DocumentNode> jsonDom;
    std::shared_ptr<std::wstring> jsonText;
    
    SimpleMarkerList lineStarts;
    MarkerList<ParseErrorMarker> errors;
    
    typedef list<JsonFileChangeListener*> Listeners;
    Listeners listeners;
    
    int notificationsDeferred;
    typedef list<priv::Notification*> DeferredNotifications;
    DeferredNotifications deferredNotifications;
    
    shared_ptr<JsonFileSemanticModelReconciliationTask> pendingReconciliationTask;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

wstring jsonizeString(const wstring &aSrc);

TextCoordinate getContainerStartColumnAddr(const json::ContainerNode *containerNode);

}
#endif
