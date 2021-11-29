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

typedef unsigned long TextLength;

class TextCoordinate {

public:
    explicit TextCoordinate(unsigned long coord = 0) : coord(coord) {}
    
    inline TextCoordinate &operator+=(long other) {
        if(!infinite()) {
            coord += other;
        }
        
        return *this;
    }

    inline TextCoordinate &operator-=(long other) {
        if(!infinite()) {
            coord -= other;
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
    
    inline TextCoordinate operator-(TextLength ofs) const {
        if(infinite()) {
            return *this;
        }
        return TextCoordinate(coord - ofs);
    }

    inline unsigned long getAddress() const {
        if(infinite()) {
            throw Exception("Can't get address for infinite");
        }
        
        return coord;
    }
    
    inline operator unsigned long() const {
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

    void assign(unsigned long aCoord) {
        coord = aCoord;
    }

    inline bool infinite() const {
        return coord == TEXT_COORDINATE_INFINITY;
    }

    inline TextCoordinate relativeTo(TextCoordinate other) const {
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
    unsigned long coord;
};



#endif /* TextCoordinate_hpp */
