// delaunay.h : header file
//

#pragma once

#include "vec2d.h"
#include <cstdlib>


typedef Vec2D Point;

struct Triangle;

struct TriRef
{
    Triangle *ptr;
    int index;

    TriRef()
    {
    }

    TriRef(Triangle *ptr_, int index_) : ptr(ptr_), index(index_)
    {
    }

    operator Triangle * () const
    {
        return ptr;
    }

    Triangle *operator -> () const
    {
        return ptr;
    }

    operator bool() const
    {
        return ptr != 0;
    }

    bool operator ! () const
    {
        return ptr == 0;
    }
};

struct Triangle
{
    const Point *pt[3];
    TriRef next[3];
};

size_t triangulate(Triangle *tr, const Point *point, size_t point_count);
