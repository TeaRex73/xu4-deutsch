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

    explicit Coords(
        const int initX = 0, const int initY = 0, const int initZ = 0
    )
        :x(initX), y(initY), z(initZ)
    {
    }

    bool operator==(const Coords &a) const
    {
        return __builtin_expect(x == a.x, false)
            && __builtin_expect(y == a.y, false)
            && __builtin_expect(z == a.z, false);
    }

    bool operator<(const Coords &a) const
    {
        if (__builtin_expect(x != a.x, true))
        {
            return x < a.x;
        }

        if (__builtin_expect(y != a.y, true))
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
