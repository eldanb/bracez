//
//  main.cpp
//  BracezTests
//
//  Created by Eldan Ben-Haim on 08/12/2021.
//

#define CATCH_CONFIG_RUNNER

#include <iostream>
#include "catch2/catch.hpp"

int main(int argc, const char * argv[]) {
    // your setup ...

      int result = Catch::Session().run( argc, argv );

      // your clean-up...

      return result;
}
