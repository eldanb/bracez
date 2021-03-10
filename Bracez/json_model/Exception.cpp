//
//  Exception.cpp
//  Bracez
//
//  Created by Eldan Ben Haim on 10/03/2021.
//

#include "Exception.hpp"

Exception::Exception(const std::string& sMessage) : runtime_error(sMessage)
{
}

