/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton

***********************************************/

#pragma once

#include "json_file.h"
#include <iostream>
#include <vector>

namespace json
{

#define PARSER_ERROR_UNEXPECTED_TOKEN     1
#define PARSER_ERROR_UNEXPECTED_CHARACTER 2
#define PARSER_ERROR_UNEXPECTED_EOS       3
#define PARSER_ERROR_EXPECTED_EOS         4
#define PARSER_ERROR_DUPLICATE_MEMBER     5
#define PARSER_ERROR_MALFORMED_NUMBER     6
#define PARSER_ERROR_INVALID_MEMBER       7

class Reader
{
public:
   struct ParseListener 
   {
       virtual void EndOfLine(TextCoordinate aWhere) = 0;
       virtual void Error(TextCoordinate aWhere, int aCode, const string &aText)=0;
   };
   
   // thrown during the first phase of reading. generally catches low-level problems such
   //  as errant characters or corrupt/incomplete documents
   class ScanException : public Exception
   {
   public:
      ScanException(const std::string& sMessage, const TextCoordinate& locError) :
         Exception(sMessage),
         m_locError(locError) {}

      TextCoordinate m_locError;
   };

   // thrown during the second phase of reading. generally catches higher-level problems such
   //  as missing commas or brackets
   class ParseException : public Exception
   {
   public:
      ParseException(const std::string& sMessage, const TextCoordinate& locTokenBegin, const TextCoordinate& locTokenEnd) :
         Exception(sMessage),
         m_locTokenBegin(locTokenBegin),
         m_locTokenEnd(locTokenEnd) {}

      TextCoordinate m_locTokenBegin;
      TextCoordinate m_locTokenEnd;
   };


   // if you know what the document looks like, call one of these...
   static void Read(ObjectNode *& object, std::wistream& istr);
   static void Read(ArrayNode *& array, std::wistream& istr);
   static void Read(StringNode *& string, std::wistream& istr);
   static void Read(NumberNode *& number, std::wistream& istr);
   static void Read(BooleanNode *& boolean, std::wistream& istr);
   static void Read(NullNode *& null, std::wistream& istr);

   // ...otherwise, if you don't know, call this & visit it
   static void Read(Node *& elementRoot, std::wistream& istr, ParseListener *aParseListener=NULL, bool allowSuffix = false);

private:
   Reader(ParseListener *aParseListener);
    
   struct Token
   {
      enum Type
      {
         TOKEN_UNKNOWN,       //    invalid bareword
         TOKEN_OBJECT_BEGIN,  //    {
         TOKEN_OBJECT_END,    //    }
         TOKEN_ARRAY_BEGIN,   //    [
         TOKEN_ARRAY_END,     //    ]
         TOKEN_NEXT_ELEMENT,  //    ,
         TOKEN_MEMBER_ASSIGN, //    :
         TOKEN_STRING,        //    "xxx"
         TOKEN_NUMBER,        //    [+/-]000.000[e[+/-]000]
         TOKEN_BOOLEAN,       //    true -or- false
         TOKEN_NULL           //    null

      };
       
      enum CharClass {
           CHAR_CLASS_UNKNOWN = 0,
           CHAR_CLASS_NUMERIC = 1,
      };
       
      Type nType;
      std::wstring sValue;

      // for malformed file debugging
      TextCoordinate locBegin;
      TextCoordinate locEnd;
   };


   class InputStream;
   class TokenStream;

   template <typename ElementTypeT>   
   static void Read_i(ElementTypeT& element, std::wistream& istr, ParseListener *aListener=NULL, bool allowSuffix = false);

public:
   // parsing token sequence into element structure
   void Parse(Node *& element, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);
   void Parse(ObjectNode *& object, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);
   void Parse(ArrayNode *& array, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);
   void Parse(StringNode *& string, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);
   void Parse(NumberNode *& number, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);
   void Parse(BooleanNode *& boolean, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);
   void Parse(NullNode *& null, TokenStream& tokenStream, TextCoordinate aBaseOfs=0);

   const std::wstring& MatchExpectedToken(Token::Type nExpected, TokenStream& tokenStream);
    
private:
    ParseListener *listener;
};


} // End namespace


#include "reader.inl"
