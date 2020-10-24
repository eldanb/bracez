/**********************************************

License: BSD
Project Webpage: http://cajun-jsonapi.sourceforge.net/
Author: Terry Caton

***********************************************/

#include <cassert>
#include <set>
#include <sstream>

/*  

TODO:
* better documentation
* unicode character decoding

*/

namespace json
{


//////////////////////
// Reader::InputStream

class Reader::InputStream // would be cool if we could inherit from std::istream & override "get"
{
public:
   InputStream(std::istream& iStr, ParseListener *aListener = NULL) :
      m_iStr(iStr), m_Location(0), m_parseListener(aListener) {}

   // protect access to the input stream, so we can keeep track of document/line offsets
   char Get(); // big, define outside
   char Peek() {
      assert(m_iStr.eof() == false); // enforce reading of only valid stream data 
      return m_iStr.peek();
   }

   bool EOS() {
      m_iStr.peek(); // apparently eof flag isn't set until a character read is attempted. whatever.
      return m_iStr.eof();
   }

   bool VerifyString(const std::string &sExpected)
   {
      bool lRet=true;
      int lPerformedGetCount  = 0;
      std::string::const_iterator it(sExpected.begin()),
                                 itEnd(sExpected.end());
      for ( ; lRet && it != itEnd; ++it) {
          if (EOS() ||      // did we reach the end before finding what we're looking for...
              (lPerformedGetCount++, m_iStr.get()) != *it) // ...or did we find something different?
          {
             lRet=false;
          }
       }
       
       while(lPerformedGetCount--)
       {
          m_iStr.unget();
       }
       
       return lRet;
   }
   
   const TextCoordinate& GetLocation() const { return m_Location; }

private:
   std::istream& m_iStr;
   TextCoordinate m_Location;
   ParseListener *m_parseListener;
};


inline char Reader::InputStream::Get()
{
   assert(m_iStr.eof() == false); // enforce reading of only valid stream data 
   char c = m_iStr.get();
   
   ++m_Location;
   if (c == '\n') {
      if(m_parseListener)
      {
         m_parseListener->EndOfLine(m_Location-1);
      }
   }

   return c;
}



//////////////////////
// Reader::TokenStream

class Reader::TokenStream
{
public:
   TokenStream(InputStream &aInputStream, ParseListener *aListener);

   const Token& Peek();
   const Token& Get();

   bool EOS() const;

private:
   void pumpTokenIfNeeded();
   void pumpToken();
   void EatWhiteSpace();
   void Match4Hex(unsigned int& integer);
   void MatchString();
   void MatchBareWordToken();
   void MatchNumber();

   bool tokenEaten;
   Token currentToken;

   InputStream &inputStream;
   ParseListener *listener;
   
   char tokenTypeLookup[256];
   char charClassLookup[256];
};


inline Reader::TokenStream::TokenStream(InputStream& aInputStream, ParseListener *aListener) :
    inputStream(aInputStream),
    tokenEaten(true),
    listener(aListener)
{
    // Prepare lookup tables
    memset(tokenTypeLookup, Token::TOKEN_UNKNOWN, 256);
    
    tokenTypeLookup['{'] = Token::TOKEN_OBJECT_BEGIN;
    tokenTypeLookup['}'] = Token::TOKEN_OBJECT_END;
    tokenTypeLookup['['] = Token::TOKEN_ARRAY_BEGIN;
    tokenTypeLookup[']'] = Token::TOKEN_ARRAY_END;
    tokenTypeLookup[','] = Token::TOKEN_NEXT_ELEMENT;
    tokenTypeLookup[':'] = Token::TOKEN_MEMBER_ASSIGN;
    
    tokenTypeLookup['-'] = Token::TOKEN_NUMBER;
    for(int i='0'; i<='9'; i++) tokenTypeLookup[i] = Token::TOKEN_NUMBER;
    
    tokenTypeLookup['"'] =  Token::TOKEN_STRING;
    
    
    memset(charClassLookup, Token::CHAR_CLASS_UNKNOWN, 256);
    const char lNumericChars[] = "0123456789.eE-+";
    for(const char *lChar = lNumericChars; *lChar; lChar++) charClassLookup[*lChar] = Token::CHAR_CLASS_NUMERIC;
    
    
    EatWhiteSpace();              // ignore any leading white space...
}

inline const Reader::Token& Reader::TokenStream::Peek() {
   pumpTokenIfNeeded();
    
   return currentToken;
}

inline const Reader::Token& Reader::TokenStream::Get() {
   assert(!EOS());
   pumpTokenIfNeeded();
   tokenEaten = true;
   
   return currentToken; 
}

inline bool Reader::TokenStream::EOS() const {
   return tokenEaten && inputStream.EOS(); 
}

inline void Reader::TokenStream::pumpTokenIfNeeded()
{
    if(tokenEaten)
    {
       assert(!EOS());
       pumpToken();
       tokenEaten = false;
    }
}

inline void Reader::TokenStream::pumpToken()
{
  // Mark token start
  currentToken.locBegin = inputStream.GetLocation();
  
  // Get current token type (good guess...)
  char sChar = inputStream.Peek();
  currentToken.nType = (Token::Type)tokenTypeLookup[sChar];

  switch (currentToken.nType)
  {
     case Token::TOKEN_NUMBER:
        MatchNumber();
        break;
        
     case Token::TOKEN_STRING:
        MatchString();
        break;
        
     case Token::TOKEN_UNKNOWN:
        MatchBareWordToken();
        break;
        
     default:   // Default case is for simple tokens
        inputStream.Get();            
  }
  
  currentToken.locEnd = inputStream.GetLocation();
  
  EatWhiteSpace();              // Skip to next real token
}

inline void Reader::TokenStream::EatWhiteSpace()
{
   while (inputStream.EOS() == false && 
          ::isspace(inputStream.Peek()))
      inputStream.Get();
}

inline void Reader::TokenStream::Match4Hex(unsigned int& integer) {
   std::stringstream ss;
   int i;
   for (i = 0; (i < 4) && (inputStream.EOS() == false) && isxdigit(inputStream.Peek()); ++i)
      ss << inputStream.Get();

   listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected a hex digit");

   ss << std::hex;
   ss >> integer;
}

inline void Reader::TokenStream::MatchString()
{
   // Eat starting "\""
   inputStream.Get();
   
   // Initialize value
   currentToken.sValue.clear();
   
   while (inputStream.EOS() == false &&
          inputStream.Peek() != '"')
   {
      char c = inputStream.Get();

      // escape?
      if (c == '\\' &&
          inputStream.EOS() == false) // shouldn't have reached the end yet
      {
         c = inputStream.Get();
         switch (c) {
            case '/':      currentToken.sValue.push_back('/');     break;
            case '"':      currentToken.sValue.push_back('"');     break;
            case '\\':     currentToken.sValue.push_back('\\');    break;
            case 'b':      currentToken.sValue.push_back('\b');    break;
            case 'f':      currentToken.sValue.push_back('\f');    break;
            case 'n':      currentToken.sValue.push_back('\n');    break;
            case 'r':      currentToken.sValue.push_back('\r');    break;
            case 't':      currentToken.sValue.push_back('\t');    break;
            case 'u':
                {
                    unsigned int codepoint = 0, surrogate = 0;
                    Match4Hex(codepoint);
                    if ((codepoint & 0xFC00) == 0xD800) { // Surrogate?
                        if(inputStream.Peek() == '\\')
                        {
                            inputStream.Get(); 
                            if(inputStream.Peek() == 'u')
                            {
                                Match4Hex(surrogate);
                                codepoint = (((codepoint & 0x3F) << 10) | 
                                    ((((codepoint >> 6) & 0xF) + 1) << 16) | 
                                    (surrogate & 0x3FF));
                            } else
                            {
                                listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected \\u after surrogate character.");
                            }
                        } else
                        {
                            listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, "Expected \\u after surrogate character.");
                        }                                
                    }

                    // To UTF8
                    if (codepoint < 0x80) {
                        currentToken.sValue.push_back((char)codepoint);
                    } else if (codepoint < 0x0800) {
                        currentToken.sValue.push_back((char)((codepoint >> 6) | 0xC0));
                        currentToken.sValue.push_back((char)((codepoint & 0x3F) | 0x80));
                    } else if (codepoint < 0x10000) {
                        currentToken.sValue.push_back((char)((codepoint >> 12) | 0xE0));
                        currentToken.sValue.push_back((char)(((codepoint >> 6) & 0x3F) | 0x80));
                        currentToken.sValue.push_back((char)((codepoint & 0x3F) | 0x80));
                    } else if (codepoint < 0x200000) {
                        currentToken.sValue.push_back((char)((codepoint >> 18) | 0xF0));
                        currentToken.sValue.push_back((char)(((codepoint >> 12) & 0x3F) | 0x80));
                        currentToken.sValue.push_back((char)(((codepoint >> 6) & 0x3F) | 0x80));
                        currentToken.sValue.push_back((char)((codepoint & 0x3F) | 0x80));
                    } else {
                        currentToken.sValue.push_back('?');
                    }
                    break;
                }
            default: {
               std::string sMessage = "Unrecognized escape sequence found in string: \\" + c;
               listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, sMessage);
            }
         }
      }
      else {
         currentToken.sValue.push_back(c);
      }
   }

    if(inputStream.EOS() == false) {
        // eat the last '"' that we just peeked
        inputStream.Get();
    } else {
        std::string sMessage = "Unterminated string constant.";
        listener->Error(inputStream.GetLocation(), PARSER_ERROR_UNEXPECTED_CHARACTER, sMessage);
    }
}

inline void Reader::TokenStream::MatchBareWordToken()
{
    currentToken.sValue.clear();
    
    char c;
    while(!inputStream.EOS() &&     
          !::isspace(c=inputStream.Peek()) &&
          c != ']' && c != '}' && c != ',' && c != ':'  // Pretty ugly hack         
            )
    {
        currentToken.sValue.push_back(inputStream.Get());
    }
    
    if(currentToken.sValue == "true" || currentToken.sValue == "false")
    {
        currentToken.nType = Token::TOKEN_BOOLEAN;
    } else
    if(currentToken.sValue == "null")
    {
        currentToken.nType = Token::TOKEN_NULL;
    } else
    {
        std::string sErrorMessage = "Unknown token in stream: " + currentToken.sValue;
        listener->Error(currentToken.locBegin, PARSER_ERROR_UNEXPECTED_CHARACTER, sErrorMessage);
    }
}

inline void Reader::TokenStream::MatchNumber()
{
   currentToken.sValue.clear();
      
   while (inputStream.EOS() == false &&
          charClassLookup[inputStream.Peek()] == Reader::Token::CHAR_CLASS_NUMERIC)
   {
      currentToken.sValue.push_back(inputStream.Get());   
   }
}

///////////////////
// Reader (finally)

inline Reader::Reader(ParseListener *aParseListener) : listener(aParseListener)
{
}

inline void Reader::Read(ObjectNode*& object, std::istream& istr)                { Read_i(object, istr); }
inline void Reader::Read(ArrayNode*& array, std::istream& istr)                  { Read_i(array, istr); }
inline void Reader::Read(StringNode*& string, std::istream& istr)                { Read_i(string, istr); }
inline void Reader::Read(NumberNode*& number, std::istream& istr)                { Read_i(number, istr); }
inline void Reader::Read(BooleanNode*& boolean, std::istream& istr)              { Read_i(boolean, istr); }
inline void Reader::Read(NullNode*& null, std::istream& istr)                    { Read_i(null, istr); }
inline void Reader::Read(Node*& unknown, std::istream& istr, ParseListener *aParseListener)       { Read_i(unknown, istr, aParseListener); }


template <typename ElementTypeT>   
void Reader::Read_i(ElementTypeT& element, std::istream& istr, ParseListener *aParseListener)
{
   Reader reader(aParseListener);

   InputStream inputStream(istr, aParseListener);
   TokenStream tokenStream(inputStream, aParseListener);
   reader.Parse(element, tokenStream);

   if (tokenStream.EOS() == false)
   {
      const Token& token = tokenStream.Peek();
      std::string sMessage = "Expected End of token stream; found " + token.sValue;
      aParseListener->Error(token.locBegin, PARSER_ERROR_EXPECTED_EOS, sMessage);
   }
}




inline void Reader::Parse(Node*& element, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs) 
{
   if (tokenStream.EOS()) {
      std::string sMessage = "Unexpected end of token stream";
      listener->Error(TextCoordinate(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of token stream");
      return;
   }

   const Token& token = tokenStream.Peek();
   switch (token.nType) {
      case Token::TOKEN_OBJECT_BEGIN:
      {
         // implicit non-const cast will perform conversion for us (if necessary)
         ObjectNode* object;
         Parse(object, tokenStream, aBaseOfs);
         element = object;
         break;
      }

      case Token::TOKEN_ARRAY_BEGIN:
      {
         ArrayNode* array;
         Parse(array, tokenStream, aBaseOfs);
         element = array;
         break;
      }

      case Token::TOKEN_STRING:
      {
         StringNode* string;
         Parse(string, tokenStream, aBaseOfs);
         element = string;
         break;
      }

      case Token::TOKEN_NUMBER:
      {
         NumberNode* number;
         Parse(number, tokenStream, aBaseOfs);
         element = number;
         break;
      }

      case Token::TOKEN_BOOLEAN:
      {
         BooleanNode* boolean;
         Parse(boolean, tokenStream, aBaseOfs);
         element = boolean;
         break;
      }

      case Token::TOKEN_NULL:
      {
         NullNode* null;
         Parse(null, tokenStream, aBaseOfs);
         element = null;
         break;
      }

      default:
      {
         std::string sMessage = "Unexpected token: " + token.sValue;
         listener->Error(token.locBegin, PARSER_ERROR_UNEXPECTED_TOKEN, sMessage);
         tokenStream.Get(); 
      }
   }
}


inline void Reader::Parse(ObjectNode*& object, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextCoordinate lBegin(tokenStream.Peek().locBegin);
   TextCoordinate lEndCoord(tokenStream.Peek().locEnd);
    
   MatchExpectedToken(Token::TOKEN_OBJECT_BEGIN, tokenStream);

   object = new ObjectNode();
   
   bool bContinue = (tokenStream.EOS() == false &&
                     tokenStream.Peek().nType != Token::TOKEN_OBJECT_END);
   while (bContinue)
   {

      // first the member name. save the token in case we have to throw an exception
      Token tokenName = tokenStream.Peek();
      MatchExpectedToken(Token::TOKEN_STRING, tokenStream);

      // ...then the key/value separator...
      MatchExpectedToken(Token::TOKEN_MEMBER_ASSIGN, tokenStream);

      // ...then the value itself (can be anything).
      Node *nodeVal = NULL;
      Parse(nodeVal, tokenStream, lBegin);

       // try adding it to the object (this could throw)
       if(nodeVal) {
           try
           {
              ObjectNode::Member &member = object->DomAddMemberNode(tokenName.sValue, nodeVal);
              member.nameRange = TextRange(tokenName.locBegin-lBegin, tokenName.locEnd-lBegin);
           }
           catch (Exception&)
           {
              // must be a duplicate name
              std::string sMessage = "Duplicate object member: " + tokenName.sValue;
              listener->Error(tokenName.locBegin, PARSER_ERROR_DUPLICATE_MEMBER, sMessage);
           }
       } else {
           std::string sMessage = "Could not parse member value '" + tokenName.sValue + "'";
           listener->Error(tokenName.locBegin, PARSER_ERROR_INVALID_MEMBER, sMessage);
       }
      Token lTok;
      while(!tokenStream.EOS() &&
            (lTok = tokenStream.Peek(), lTok.nType != Token::TOKEN_NEXT_ELEMENT && lTok.nType != Token::TOKEN_OBJECT_END))
      {
         lEndCoord = lTok.locBegin;
         listener->Error(lTok.locBegin, PARSER_ERROR_UNEXPECTED_TOKEN, "Expecting \",\" or \"}\"");
         tokenStream.Get();
      }

      if(lTok.nType == Token::TOKEN_NEXT_ELEMENT)
      {
         bContinue = true;
         lEndCoord = lTok.locEnd;
         MatchExpectedToken(lTok.nType, tokenStream);
      } else
      {
         bContinue = false;
      }
   }

   if(tokenStream.EOS())
   {
      listener->Error(lEndCoord, PARSER_ERROR_UNEXPECTED_EOS, "Expecting \",\" or \"}\"");
   } else
   {    
      lEndCoord = tokenStream.Peek().locEnd;
      MatchExpectedToken(Token::TOKEN_OBJECT_END, tokenStream);
   }

   object->textRange = TextRange(lBegin-aBaseOfs, lEndCoord-aBaseOfs);
}


inline void Reader::Parse(ArrayNode*& array, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextCoordinate lBegin(tokenStream.Peek().locBegin);
   MatchExpectedToken(Token::TOKEN_ARRAY_BEGIN, tokenStream);

   array = new ArrayNode();
   
   bool bContinue = (tokenStream.EOS() == false &&
                     tokenStream.Peek().nType != Token::TOKEN_ARRAY_END);
   while (bContinue)
   {
      Node *elemVal;
      Parse(elemVal, tokenStream, lBegin);

      array->DomAddElementNode(elemVal);

      bContinue = (tokenStream.EOS() == false &&
                   tokenStream.Peek().nType == Token::TOKEN_NEXT_ELEMENT);
      if (bContinue)
         MatchExpectedToken(Token::TOKEN_NEXT_ELEMENT, tokenStream);
   }

   if(tokenStream.EOS()) {
       listener->Error(0, PARSER_ERROR_UNEXPECTED_EOS, "Expecting \",\" or \"]\"");
       return;
   }
    
   TextCoordinate lEnd(tokenStream.Peek().locEnd);   
   array->textRange = TextRange(lBegin-aBaseOfs, lEnd-aBaseOfs);
   
   MatchExpectedToken(Token::TOKEN_ARRAY_END, tokenStream);
}


inline void Reader::Parse(StringNode*& string, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextRange lTokRange(tokenStream.Peek().locBegin-aBaseOfs, tokenStream.Peek().locEnd-aBaseOfs);
   string = new StringNode(MatchExpectedToken(Token::TOKEN_STRING, tokenStream));
   string->textRange = lTokRange;
}


inline void Reader::Parse(NumberNode*& number, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   const Token& currentToken = tokenStream.Peek(); // might need this later for throwing exception
   TextRange lTokRange(currentToken.locBegin-aBaseOfs, currentToken.locEnd-aBaseOfs);
    
   const std::string& sValue = MatchExpectedToken(Token::TOKEN_NUMBER, tokenStream);

   std::istringstream iStr(sValue);
   double dValue;
   iStr >> dValue;

   // did we consume all characters in the token?
   if (iStr.eof() == false)
   {
      std::string sMessage = std::string("Unexpected character in NUMBER token: ") + std::string(1, iStr.peek());
      listener->Error(currentToken.locBegin, PARSER_ERROR_MALFORMED_NUMBER, sMessage);
   }

   number = new NumberNode(dValue);
   number->textRange = lTokRange;
}


inline void Reader::Parse(BooleanNode*& boolean, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextRange lTokRange(tokenStream.Peek().locBegin-aBaseOfs, tokenStream.Peek().locEnd-aBaseOfs);
   const std::string& sValue = MatchExpectedToken(Token::TOKEN_BOOLEAN, tokenStream);
   
   boolean = new BooleanNode(sValue == "true");
   boolean->textRange = lTokRange;
}


inline void Reader::Parse(NullNode*& aNull, Reader::TokenStream& tokenStream, TextCoordinate aBaseOfs)
{
   TextRange lTokRange(tokenStream.Peek().locBegin-aBaseOfs, tokenStream.Peek().locEnd-aBaseOfs);
   MatchExpectedToken(Token::TOKEN_NULL, tokenStream);
   aNull = new NullNode();
   aNull->textRange = lTokRange;
}


inline const std::string& Reader::MatchExpectedToken(Token::Type nExpected, Reader::TokenStream& tokenStream)
{
   static string lEmptyStr;
   
   if (tokenStream.EOS())
   {
      std::string sMessage = "Unexpected end of token stream";
      listener->Error(TextCoordinate(), PARSER_ERROR_UNEXPECTED_EOS, "Unexpected end of token stream");
      return lEmptyStr;
   }

   const Token& token = tokenStream.Peek();
   if (token.nType != nExpected)
   {
     static const char *lTypeNames[] = { 
          "'{'",
          "'}'",
          "'['",
          "']'",
          "','",
          "':'",
          "string",
          "number",
          "'true' or 'false'",
          "'null'"          
      };

      std::string sMessage = "Unexpected token: " + token.sValue + "; expecting " + lTypeNames[nExpected];
      listener->Error(token.locBegin-1, PARSER_ERROR_UNEXPECTED_TOKEN, sMessage);
   } else
   {   
      tokenStream.Get();
   }
   
   return token.sValue;
}

} // End namespace
