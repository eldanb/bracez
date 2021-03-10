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
   
   struct TextRange
   {
      TextRange() {}
      TextRange(const TextCoordinate &aStart, const TextCoordinate &aEnd) : start(aStart), end(aEnd) {}

      TextLength length() const { return end-start; }
      
       TextRange intersectWith(TextRange other) const {
           TextRange ret;
           ret.start = start > other.start ? start : other.start;
           ret.end = end < other.end ? end : other.end;
           
           return ret;
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
      
      const ContainerNode *GetParent() const { return parent; }
      ContainerNode *GetParent() { return parent; }
      
      DocumentNode *GetDocument() const;
      
      const TextRange &GetTextRange() const;
      TextRange GetAbsTextRange() const;
      
      wstring GetDocumentText() const;
      
      virtual void accept(NodeVisitor *aVisitor) const = 0;
      virtual void accept(NodeVisitor *aVisitor) = 0;
      
      virtual NodeTypeId GetNodeTypeId() const = 0;
   
      virtual void CalculateJsonTextRepresentation(std::wstring &aDest) const = 0;
       
      virtual bool ValueEquals(Node *other) const = 0;
      virtual bool ValueLt(Node *other) const = 0;
       
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
      void SetChildAt(int aIdx, Node *aNode, bool fromReparse = false);
       
      virtual int GetChildCount()  const = 0;
      
      virtual Node *GetChildAt(int aIdx) = 0;
      virtual const Node *GetChildAt(int aIdx) const = 0;
      virtual void RemoveChildAt(int aIdx);

      virtual int GetIndexOfChild(const Node *aChild) const;

      virtual int FindChildEndingAfter(const TextCoordinate &aDocOffset) const = 0;
      virtual int FindChildContaining(const TextCoordinate &aDocOffset, bool strict) const;
      
      
      virtual void DetachChildAt(int aIdx, Node **aNode) = 0;

      virtual bool ValueEquals(Node *other) const;
      bool ValueLt(Node *other) const;

   protected:
      virtual void AdjustChildRangeAt(int aIdx, int aDiff);
      virtual void StoreChildAt(int aIdx, Node *aNode) = 0;

      friend class JsonFile;

   } ;

   class ArrayNode : public ContainerNode
   {
   public:
   
      typedef std::deque<Node *> Elements;
      typedef Elements::iterator iterator;
      typedef Elements::const_iterator const_iterator;

      iterator Begin();
      iterator End();
      
      const_iterator Begin() const;
      const_iterator End() const;

      int GetChildCount() const;
      
      Node *GetChildAt(int aIdx);
      const Node *GetChildAt(int aIdx) const;
      void DetachChildAt(int aIdx, Node **aNode);

      int FindChildEndingAfter(const TextCoordinate &aDocOffset) const;

      void InsertMemberAt(int aIdx, Node *aElement, 
                          const wstring *aElementText=NULL);
      
      void accept(NodeVisitor *aVisitor) const;
      void accept(NodeVisitor *aVisitor);
         
      NodeTypeId GetNodeTypeId() const;

      void CalculateJsonTextRepresentation(std::wstring &aDest) const;
      
      // DOM-only modifiers
      void DomAddElementNode(Node *aElement);
      
   protected:
       virtual void StoreChildAt(int aIdx, Node *aNode);

   private:
      Elements elements;
   };

   class ObjectNode : public ContainerNode
   {
   public:
   
      struct Member
      {
         Member(const wstring &aName, Node *aNode) : node(aNode), name(aName) {}
         Member() : node(NULL) {}

         wstring name;
         TextRange nameRange;
         
         Node *node;
      } ;
      
      typedef std::deque<Member> Members;
      typedef Members::iterator iterator;
      typedef Members::const_iterator const_iterator;

      iterator Begin();
      iterator End();
      
      const_iterator Begin() const;
      const_iterator End() const;

      int GetChildCount() const;
      
      Node *GetChildAt(int aIdx);
      const Node *GetChildAt(int aIdx) const;
      const Member *GetChildMemberAt(int aIdx) const;

      void DetachChildAt(int aIdx, Node **aNode);
      ObjectNode::Member &InsertMemberAt(int aIdx, const wstring &aName,
                           Node *aElement, const wstring *aElementText=NULL);
      void RenameMemberAt(int aIdx, const wstring &aName);

      int FindChildEndingAfter(const TextCoordinate &aDocOffset) const;

      const wstring &GetMemberNameAt(int aIdx) const;
      int GetIndexOfMemberWithName(const wstring &name) const;

      void accept(NodeVisitor *aVisitor) const;
      void accept(NodeVisitor *aVisitor);

      NodeTypeId GetNodeTypeId() const;

      void CalculateJsonTextRepresentation(std::wstring &aDest) const;

      // DOM-only modifiers
      Member &DomAddMemberNode(const wstring &aName, Node *aElement);

      bool ValueEquals(Node *other) const;

   protected:
      void AdjustChildRangeAt(int aIdx, int aDiff);
      virtual void StoreChildAt(int aIdx, Node *aNode);

   private:
      Members members;

   } ;

   class DocumentNode : public ContainerNode
   {      
   public:
      DocumentNode(JsonFile *aOwner, Node *aChild);
      
      virtual void accept(NodeVisitor *aVisitor) const;
      virtual void accept(NodeVisitor *aVisitor);
      
      NodeTypeId GetNodeTypeId() const ;

      virtual int GetChildCount() const;
      
      virtual Node *GetChildAt(int aIdx);
      virtual const Node *GetChildAt(int aIdx) const;
      void DetachChildAt(int aIdx, Node **aNode);


      int FindChildEndingAfter(const TextCoordinate &aDocOffset) const;
      
      JsonFile *GetOwner() const { return owner; }
      
      void CalculateJsonTextRepresentation(std::wstring &aDest) const;

   protected:
       virtual void StoreChildAt(int aIdx, Node *aNode);

   private:
      Node *rootNode;
      JsonFile *owner;
   } ;

   class LeafNode : public Node
   {
   public:
      void accept(NodeVisitor *aVisitor);
      void accept(NodeVisitor *aVisitor) const; 

   } ;
   
   class NullNode : public LeafNode
   {            
   public:
      NodeTypeId GetNodeTypeId() const;

      void CalculateJsonTextRepresentation(std::wstring &aDest) const;

      bool ValueEquals(Node *other) const;
      bool ValueLt(Node *other) const;

   protected:  

   } ;

   template<class ValueType, int NodeTypeConst> 
   class ValueNode : public LeafNode
   {
   public:
      ValueNode(const ValueType &aData) : value(aData) {}
      
      NodeTypeId GetNodeTypeId() const { return (NodeTypeId)NodeTypeConst; }
      
      const ValueType &GetValue() const { return value; }
            
      operator ValueType() const { return value; }

      void CalculateJsonTextRepresentation(std::wstring &aDest) const;
      
      bool ValueEquals(Node *other) const {
          ValueNode<ValueType, NodeTypeConst> *otherTyped = dynamic_cast< ValueNode<ValueType, NodeTypeConst> *>(other);
          return otherTyped && otherTyped->GetValue() == GetValue();
      }

       bool ValueLt(Node *other) const {
           ValueNode<ValueType, NodeTypeConst> *otherTyped = dynamic_cast< ValueNode<ValueType, NodeTypeConst> *>(other);
           return otherTyped && otherTyped->GetValue() > GetValue();
       }

   protected:

   private:
      ValueType value;
   } ;

   typedef ValueNode<std::wstring, ntString> StringNode;
   typedef ValueNode<double, ntNumber> NumberNode;
   typedef ValueNode<bool, ntBoolean> BooleanNode;

   struct JsonFileChangeListener
   {
      virtual void notifyTextSpliced(JsonFile *aSender, TextCoordinate aOldOffset, 
                                     TextLength aOldLength, TextLength aNewLength) {}
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
   private:
      std::string errorText;
      int errorCode;
   } ;
   
   namespace priv
   {
      class Notification
      {
      public:
         virtual void deliverToListener(JsonFile *aSender, JsonFileChangeListener *aListener) const = 0;
         virtual Notification *clone() const = 0;
      } ;
      
      class DeferNotificationsInBlock;
   }
   
   class JsonFile
   {
   public:
      JsonFile();
      
      void setText(const std::wstring &aText);
      const std::wstring &getText() const;
      
      DocumentNode *getDom();
      const DocumentNode *getDom() const;
        
      bool FindPathContaining(TextCoordinate aDocOffset, JsonPath &aRet) const;
      const json::Node *FindNodeContaining(TextCoordinate aDocOffset, JsonPath *path, bool strict = false) const;

      bool FindPathForJsonPathString(std::wstring aPathString, JsonPath &aRet);
             
      void addListener(JsonFileChangeListener *aListener);
      void removeListener(JsonFileChangeListener *aListener);
      
      const MarkerList<ParseErrorMarker> &getErrors() { return errors; }
      
       
      bool spliceTextWithWorkLimit(TextCoordinate aOffsetStart,
                                   TextLength aLen,
                                   const std::wstring &aNewText,
                                   int maxParsedRegionLength);

   private:
      void spliceJsonTextByDomChange(TextCoordinate aOffsetStart, TextLength aLen, const std::wstring &aNewText);
      
      void updateTreeOffsetsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen);
      void updateErrorsAfterSplice(TextCoordinate aOffsetStart, TextLength aLen, TextLength aNewLen);

   private:
      void notify(const priv::Notification &aNotification);
      void beginDeferNotifications();
      void endDeferNotifications();
      
   private:
      void deliverNotification(const priv::Notification &aNotification);
      
   private:
      friend class JsonFileParseListener;
      friend class ContainerNode;
      friend class ArrayNode;
      friend class ObjectNode;
      friend class Notification;
      friend class priv::DeferNotificationsInBlock;
      
      DocumentNode *jsonDom;      

      std::wstring jsonText;      

      MarkerList<ParseErrorMarker> errors;
      
      typedef list<JsonFileChangeListener*> Listeners;
      Listeners listeners;
      int notificationsDeferred;
      typedef list<priv::Notification*> DeferredNotifications;
      DeferredNotifications deferredNotifications;
   };
   
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   
    wstring jsonizeString(const wstring &aSrc);

}
#endif
