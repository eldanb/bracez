//
//  TextCoordinate.hpp
//  Bracez
//
//  Created by Eldan Ben Haim on 10/03/2021.
//

#ifndef TextCoordinate_hpp
#define TextCoordinate_hpp

#include <stdio.h>

#include "Exception.hpp"

#define TEXT_COORDINATE_INFINITY ((unsigned int)-1)

typedef unsigned int TextLength;

class TextCoordinate {

public:
    explicit TextCoordinate(unsigned int coord = 0) : coord(coord) {}
    
    inline TextCoordinate &operator+=(int other) {
        if(!infinite()) {
            coord += other;
        }
        
        return *this;
    }

    inline TextCoordinate operator+(int ofs) const {
        if(!infinite()) {
            return TextCoordinate(coord + ofs);
        } else {
            return *this;
        }
    }

    inline TextCoordinate operator+(TextLength ofs) const {
        if(!infinite()) {
            return TextCoordinate(coord + ofs);
        } else {
            return *this;
        }
    }

    inline TextCoordinate operator-(int ofs) const {
        if(infinite()) {
            return *this;
        }
        return TextCoordinate(coord - ofs);
    }

    inline unsigned int getAddress() const {
        if(infinite()) {
            throw Exception("Can't get address for infinite");
        }
        
        return coord;
    }
    
    inline operator unsigned int() const {
        return getAddress();
    }
    
    inline TextLength operator-(TextCoordinate other) const {
        if(infinite() || other.infinite()) {
            throw Exception("Can't compute length between infinites");
        }
        
        return coord - other.coord;
    }

    inline bool operator<(TextCoordinate other) const {
        return coord < other.coord;
    }

    inline bool operator<=(TextCoordinate other) const {
        return coord <= other.coord;
    }

    inline bool operator==(TextCoordinate other) const {
        return coord == other.coord;
    }

    inline bool operator!=(TextCoordinate other) const {
        return coord != other.coord;
    }

    inline bool operator>=(TextCoordinate other) const {
        return coord >= other.coord;
    }

    inline bool operator>(TextCoordinate other) const {
        return coord > other.coord;
    }

    TextCoordinate &operator=(TextCoordinate other) {
        coord = other.coord;
        return *this;
    }
    
    bool infinite() const {
        return coord == TEXT_COORDINATE_INFINITY;
    }

    TextCoordinate relativeTo(TextCoordinate other) const {
        if(other.infinite()) {
            throw Exception("Can't devise coordinate relative to infinite");
        }
        
        if(infinite()) {
            return *this;
        }
        
        return TextCoordinate(coord - other.coord);
    }
    

    
public:
    static TextCoordinate infinity;
    
private:
    unsigned int coord;
};



#endif /* TextCoordinate_hpp */
