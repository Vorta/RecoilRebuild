#pragma once

#include "recoil/recoil_types.h"

#include <math.h>
#include <stddef.h>

struct zVec3 {
    float x;
    float y;
    float z;
};

RECOIL_STATIC_ASSERT(sizeof(zVec3) == 0x0c);

struct zVec2 {
    float x;
    float y;
};

struct zMat4x3 {
    float xx;
    float xy;
    float xz;
    float yx;
    float yy;
    float yz;
    float zx;
    float zy;
    float zz;
    float posX;
    float posY;
    float posZ;
};

RECOIL_STATIC_ASSERT(sizeof(zMat4x3) == 0x30);
RECOIL_STATIC_ASSERT(
    offsetof(
        zMat4x3,
        posX
    ) == 0x24
);

struct zQuat {
    float w;
    float x;
    float y;
    float z;
};

RECOIL_STATIC_ASSERT(sizeof(zQuat) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zQuat,
        x
    ) == 0x04
);

struct zBBox3f {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

struct zBBoxCorners {
    float values[24];
};

struct zProjectedPoint {
    float x;
    float y;
    float reciprocalZ;
};

struct zProjectedSphere {
    float x;
    float y;
    float screenRadius;
};

RECOIL_STATIC_ASSERT(sizeof(zBBox3f) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zBBoxCorners) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(_exception) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        _exception,
        name
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        _exception,
        arg1
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        _exception,
        arg2
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        _exception,
        retval
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zVec2) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zProjectedPoint) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zProjectedSphere) == 0x0c);
