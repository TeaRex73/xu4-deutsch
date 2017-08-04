/*
 * $Id$
 */

#ifndef COORDS_H
#define COORDS_H

/**
 * A simple representation of a point in 3D space.
 */
class Coords {
public:
    int x, y, z;

    Coords(int initx = 0, int inity = 0, int initz = 0)
        :x(initx), y(inity), z(initz)
    {
    }
        
    bool operator==(const Coords &a) const
    {
        return __builtin_expect((x == a.x), false)
            && __builtin_expect((y == a.y), false)
            && __builtin_expect((z == a.z), false);
    }

    bool operator<(const Coords &a) const
    {
        if (__builtin_expect(!(x == a.x), true))
        {
            return x < a.x;
        }
    
        if (__builtin_expect(!(y == a.y), true))
        {
            return y < a.y;
        }
        return z < a.z;
    }    

    bool operator!=(const Coords &a) const
    {
        return __builtin_expect(!operator==(a), true);
    }

};

#endif /* COORDS_H */
