//
//  SyntaxHilightJsonVisitor.h
//  Bracez
//
//  Created by Eldan Ben Haim on 16/10/2020.
//

#import <Foundation/Foundation.h>
#include "json_file.h"
#import "NodeTypeToColorTransformer.h"

NS_ASSUME_NONNULL_BEGIN

using namespace json;

class SyntaxHighlightJsonVisitor : public NodeVisitor
{
public:
   SyntaxHighlightJsonVisitor(NSMutableAttributedString *aTextStorage, const NSRange &aHighlightRange);
   
    virtual bool visitNode(Node *aNode);
    virtual bool visitObjectMemberNode(ObjectNode::Member *aMember);
    virtual bool visitNode(const Node *aNode);
   
   
    virtual bool visitObjectMemberNode(const ObjectNode::Member *aMember);
   
   
private:
    void loadColors();
   
    NodeTypeToColorTransformer *colors;
    NSMutableAttributedString *textStorage;
    NSRange hilightRange;
} ;

NS_ASSUME_NONNULL_END
