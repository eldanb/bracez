//
//  Exception.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 10/03/2021.
//

#ifndef Exception_hpp
#define Exception_hpp

#include <stdio.h>
#include <string>

/////////////////////////////////////////////////////////////////////////
// Exception - base class for all JSON-related runtime errors

class Exception : public std::runtime_error
{
public:
   Exception(const std::string& sMessage);
};

class ParseCancelledException : public Exception {
public:
    ParseCancelledException() : Exception("Parse cancelled") {}
} ;
#endif /* Exception_hpp */
