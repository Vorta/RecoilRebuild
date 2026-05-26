#include "zClipRect.h"

#include "GameZRecoil/zModel/zModel.h"

#include <string.h>

zClipVert g_Clip_PolyVerts[0x40] = {0};
zClipVert g_Clip_PolyVertsScratch[0x40] = {0};
zClipUV g_Clip_PolyUvsStorage[0x40] = {0};
zClipUV *g_Clip_PolyUvs = g_Clip_PolyUvsStorage;
zClipRectPartial gClipRect_Primary = {0};

namespace {
const int kClipBufferCapacity = 0x40;

bool IsInsideNear(const zClipVert &vertex, float zMin) {
    return vertex.z >= zMin;
}

bool IsInsideMin(float value, float minValue) {
    return value >= minValue;
}

bool IsInsideMax(float value, float maxValue) {
    return value < maxValue;
}

zClipVert InterpolateVert(const zClipVert &a, const zClipVert &b, float t, float z) {
    zClipVert out = {0};
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = z;
    return out;
}

zClipVert InterpolateVertOnAxis(const zClipVert &a, const zClipVert &b, float t, int axis,
                                float bound) {
    zClipVert out = {0};
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = a.z + (b.z - a.z) * t;
    if (axis == 0) {
        out.x = bound;
    } else {
        out.y = bound;
    }
    return out;
}

zClipUV InterpolateUv(const zClipUV &a, const zClipUV &b, float t) {
    zClipUV out = {0};
    out.u = a.u + (b.u - a.u) * t;
    out.v = a.v + (b.v - a.v) * t;
    return out;
}

float InterpolateFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

void AppendClipped(zClipVert *verts, zClipUV *uvs, int &count, const zClipVert &vert,
                   const zClipUV &uv) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    uvs[count] = uv;
    ++count;
}

void AppendClippedVert(zClipVert *verts, int &count, const zClipVert &vert) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    ++count;
}

void AppendClippedWithAttr(zClipVert *verts, zClipUV *uvs, float *attrs, int &count,
                           const zClipVert &vert, const zClipUV &uv, float attr) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    uvs[count] = uv;
    attrs[count] = attr;
    ++count;
}

void AppendClippedWithAttr012(zClipVert *verts, zClipUV *uvs, float *attr0, float *attr1,
                              float *attr2, int &count, const zClipVert &vert,
                              const zClipUV &uv, float value0, float value1, float value2) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    uvs[count] = uv;
    attr0[count] = value0;
    attr1[count] = value1;
    attr2[count] = value2;
    ++count;
}

void AppendClippedVertWithAttr012(zClipVert *verts, float *attr0, float *attr1, float *attr2,
                                  int &count, const zClipVert &vert, float value0,
                                  float value1, float value2) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    attr0[count] = value0;
    attr1[count] = value1;
    attr2[count] = value2;
    ++count;
}

int ClipVertsAgainstPlane(const zClipVert *source, int sourceCount,
                                   zClipVert *dest, int axis, float bound, bool clipMin) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = source[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(prevValue, bound) : IsInsideMax(prevValue, bound);

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = source[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(currValue, bound) : IsInsideMax(currValue, bound);

        if (prevInside != currInside) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            if (destCount < kClipBufferCapacity) {
                dest[destCount] = InterpolateVertOnAxis(prevVert, currVert, t, axis, bound);
                ++destCount;
            }
        }

        if (currInside && destCount < kClipBufferCapacity) {
            dest[destCount] = currVert;
            ++destCount;
        }

        prevVert = currVert;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

int ClipVertsUvsAgainstPlane(const zClipVert *sourceVerts, const zClipUV *sourceUvs,
                                      int sourceCount, zClipVert *destVerts,
                                      zClipUV *destUvs, int axis, float bound, bool clipMin) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    zClipUV prevUv = sourceUvs[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(prevValue, bound) : IsInsideMax(prevValue, bound);

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const zClipUV currUv = sourceUvs[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(currValue, bound) : IsInsideMax(currValue, bound);

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(prevVert, currVert, t, axis, bound);
            destUvs[destCount] = InterpolateUv(prevUv, currUv, t);
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destUvs[destCount] = currUv;
            ++destCount;
        }

        prevVert = currVert;
        prevUv = currUv;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

int ClipVertsAttr0AgainstPlane(const zClipVert *sourceVerts, const float *sourceAttrs,
                                        int sourceCount, zClipVert *destVerts,
                                        float *destAttrs, int axis, float bound, bool clipMin) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    float prevAttr = sourceAttrs[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(prevValue, bound) : IsInsideMax(prevValue, bound);

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const float currAttr = sourceAttrs[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(currValue, bound) : IsInsideMax(currValue, bound);

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(prevVert, currVert, t, axis, bound);
            destAttrs[destCount] = InterpolateFloat(prevAttr, currAttr, t);
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destAttrs[destCount] = currAttr;
            ++destCount;
        }

        prevVert = currVert;
        prevAttr = currAttr;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

int ClipVertsAttr012AgainstPlane(const zClipVert *sourceVerts, const float *sourceAttr0,
                                          const float *sourceAttr1, const float *sourceAttr2,
                                          int sourceCount, zClipVert *destVerts,
                                          float *destAttr0, float *destAttr1, float *destAttr2,
                                          int axis, float bound, bool clipMin) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    float prevAttr0 = sourceAttr0[sourceCount - 1];
    float prevAttr1 = sourceAttr1[sourceCount - 1];
    float prevAttr2 = sourceAttr2[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(prevValue, bound) : IsInsideMax(prevValue, bound);

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const float currAttr0 = sourceAttr0[i];
        const float currAttr1 = sourceAttr1[i];
        const float currAttr2 = sourceAttr2[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(currValue, bound) : IsInsideMax(currValue, bound);

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(prevVert, currVert, t, axis, bound);
            destAttr0[destCount] = InterpolateFloat(prevAttr0, currAttr0, t);
            destAttr1[destCount] = InterpolateFloat(prevAttr1, currAttr1, t);
            destAttr2[destCount] = InterpolateFloat(prevAttr2, currAttr2, t);
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destAttr0[destCount] = currAttr0;
            destAttr1[destCount] = currAttr1;
            destAttr2[destCount] = currAttr2;
            ++destCount;
        }

        prevVert = currVert;
        prevAttr0 = currAttr0;
        prevAttr1 = currAttr1;
        prevAttr2 = currAttr2;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

int ClipVertsUvsAttr012AgainstPlane(const zClipVert *sourceVerts, const zClipUV *sourceUvs,
                                             const float *sourceAttr0, const float *sourceAttr1,
                                             const float *sourceAttr2, int sourceCount,
                                             zClipVert *destVerts, zClipUV *destUvs,
                                             float *destAttr0, float *destAttr1, float *destAttr2,
                                             int axis, float bound, bool clipMin) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    zClipUV prevUv = sourceUvs[sourceCount - 1];
    float prevAttr0 = sourceAttr0[sourceCount - 1];
    float prevAttr1 = sourceAttr1[sourceCount - 1];
    float prevAttr2 = sourceAttr2[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(prevValue, bound) : IsInsideMax(prevValue, bound);

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const zClipUV currUv = sourceUvs[i];
        const float currAttr0 = sourceAttr0[i];
        const float currAttr1 = sourceAttr1[i];
        const float currAttr2 = sourceAttr2[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(currValue, bound) : IsInsideMax(currValue, bound);

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(prevVert, currVert, t, axis, bound);
            destUvs[destCount] = InterpolateUv(prevUv, currUv, t);
            destAttr0[destCount] = InterpolateFloat(prevAttr0, currAttr0, t);
            destAttr1[destCount] = InterpolateFloat(prevAttr1, currAttr1, t);
            destAttr2[destCount] = InterpolateFloat(prevAttr2, currAttr2, t);
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destUvs[destCount] = currUv;
            destAttr0[destCount] = currAttr0;
            destAttr1[destCount] = currAttr1;
            destAttr2[destCount] = currAttr2;
            ++destCount;
        }

        prevVert = currVert;
        prevUv = currUv;
        prevAttr0 = currAttr0;
        prevAttr1 = currAttr1;
        prevAttr2 = currAttr2;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

int ClipPolyNoUvCore(zClipRectPartial *clipRect, int *vertexCount) {
    zClipVert scratchA[kClipBufferCapacity] = {0};
    zClipVert scratchB[kClipBufferCapacity] = {0};
    const zClipVert *source = g_Clip_PolyVerts;
    zClipVert *dest = scratchA;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsAgainstPlane(source, count, dest, 0, clipRect->xMin, true);
        source = dest;
        dest = scratchB;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsAgainstPlane(source, count, dest, 0, clipRect->xMaxAlt, false);
        source = dest;
        dest = dest == scratchA ? scratchB : scratchA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsAgainstPlane(source, count, dest, 1, clipRect->yMin, true);
        source = dest;
        dest = dest == scratchA ? scratchB : scratchA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsAgainstPlane(source, count, dest, 1, clipRect->yMaxAlt, false);
        source = dest;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (source != g_Clip_PolyVerts) {
        memcpy(g_Clip_PolyVerts, source,
                    static_cast<size_t>(outputCount) * sizeof(zClipVert));
    }
    return 1;
}

int ClipPolyUvCore(zClipRectPartial *clipRect, int *vertexCount) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    zClipUV scratchUvsA[kClipBufferCapacity] = {0};
    zClipUV scratchUvsB[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const zClipUV *sourceUvs = g_Clip_PolyUvs;
    zClipVert *destVerts = scratchVertsA;
    zClipUV *destUvs = scratchUvsA;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(sourceVerts, sourceUvs, count, destVerts, destUvs, 0,
                                               clipRect->xMin, true);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        destVerts = scratchVertsB;
        destUvs = scratchUvsB;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(sourceVerts, sourceUvs, count, destVerts, destUvs, 0,
                                               clipRect->xMaxAlt, false);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(sourceVerts, sourceUvs, count, destVerts, destUvs, 1,
                                               clipRect->yMin, true);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(sourceVerts, sourceUvs, count, destVerts, destUvs, 1,
                                               clipRect->yMaxAlt, false);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(g_Clip_PolyVerts, sourceVerts,
                    static_cast<size_t>(outputCount) * sizeof(zClipVert));
    }
    if (sourceUvs != g_Clip_PolyUvs) {
        memcpy(g_Clip_PolyUvs, sourceUvs,
                    static_cast<size_t>(outputCount) * sizeof(zClipUV));
    }
    return 1;
}

int ClipPolyAttr0NoUvCore(zClipRectPartial *clipRect, int *vertexCount) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    float scratchAttrsA[kClipBufferCapacity] = {0};
    float scratchAttrsB[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const float *sourceAttrs = g_Clip_PolyAttr0;
    zClipVert *destVerts = scratchVertsA;
    float *destAttrs = scratchAttrsA;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(sourceVerts, sourceAttrs, count, destVerts,
                                                 destAttrs, 0, clipRect->xMin, true);
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        destVerts = scratchVertsB;
        destAttrs = scratchAttrsB;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(sourceVerts, sourceAttrs, count, destVerts,
                                                 destAttrs, 0, clipRect->xMaxAlt, false);
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttrs = wroteA ? scratchAttrsB : scratchAttrsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(sourceVerts, sourceAttrs, count, destVerts,
                                                 destAttrs, 1, clipRect->yMin, true);
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttrs = wroteA ? scratchAttrsB : scratchAttrsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(sourceVerts, sourceAttrs, count, destVerts,
                                                 destAttrs, 1, clipRect->yMaxAlt, false);
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(g_Clip_PolyVerts, sourceVerts,
                    static_cast<size_t>(outputCount) * sizeof(zClipVert));
    }
    if (sourceAttrs != g_Clip_PolyAttr0) {
        memcpy(g_Clip_PolyAttr0, sourceAttrs,
                    static_cast<size_t>(outputCount) * sizeof(float));
    }
    return 1;
}

int ClipPolyAttr012NoUvCore(zClipRectPartial *clipRect, int *vertexCount) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    float scratchAttr0A[kClipBufferCapacity] = {0};
    float scratchAttr0B[kClipBufferCapacity] = {0};
    float scratchAttr1A[kClipBufferCapacity] = {0};
    float scratchAttr1B[kClipBufferCapacity] = {0};
    float scratchAttr2A[kClipBufferCapacity] = {0};
    float scratchAttr2B[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const float *sourceAttr0 = g_Clip_PolyAttr0;
    const float *sourceAttr1 = g_Clip_PolyAttr1;
    const float *sourceAttr2 = g_Clip_PolyAttr2;
    zClipVert *destVerts = scratchVertsA;
    float *destAttr0 = scratchAttr0A;
    float *destAttr1 = scratchAttr1A;
    float *destAttr2 = scratchAttr2A;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(sourceVerts, sourceAttr0, sourceAttr1,
                                                   sourceAttr2, count, destVerts, destAttr0,
                                                   destAttr1, destAttr2, 0, clipRect->xMin, true);
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        destVerts = scratchVertsB;
        destAttr0 = scratchAttr0B;
        destAttr1 = scratchAttr1B;
        destAttr2 = scratchAttr2B;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(
            sourceVerts, sourceAttr0, sourceAttr1, sourceAttr2, count, destVerts, destAttr0,
            destAttr1, destAttr2, 0, clipRect->xMaxAlt, false);
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(sourceVerts, sourceAttr0, sourceAttr1,
                                                   sourceAttr2, count, destVerts, destAttr0,
                                                   destAttr1, destAttr2, 1, clipRect->yMin, true);
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(
            sourceVerts, sourceAttr0, sourceAttr1, sourceAttr2, count, destVerts, destAttr0,
            destAttr1, destAttr2, 1, clipRect->yMaxAlt, false);
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(g_Clip_PolyVerts, sourceVerts,
                    static_cast<size_t>(outputCount) * sizeof(zClipVert));
        memcpy(g_Clip_PolyAttr0, sourceAttr0,
                    static_cast<size_t>(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr1, sourceAttr1,
                    static_cast<size_t>(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr2, sourceAttr2,
                    static_cast<size_t>(outputCount) * sizeof(float));
    }
    return 1;
}

int ClipPolyAttr012UvCore(zClipRectPartial *clipRect, int *vertexCount) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    zClipUV scratchUvsA[kClipBufferCapacity] = {0};
    zClipUV scratchUvsB[kClipBufferCapacity] = {0};
    float scratchAttr0A[kClipBufferCapacity] = {0};
    float scratchAttr0B[kClipBufferCapacity] = {0};
    float scratchAttr1A[kClipBufferCapacity] = {0};
    float scratchAttr1B[kClipBufferCapacity] = {0};
    float scratchAttr2A[kClipBufferCapacity] = {0};
    float scratchAttr2B[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const zClipUV *sourceUvs = g_Clip_PolyUvs;
    const float *sourceAttr0 = g_Clip_PolyAttr0;
    const float *sourceAttr1 = g_Clip_PolyAttr1;
    const float *sourceAttr2 = g_Clip_PolyAttr2;
    zClipVert *destVerts = scratchVertsA;
    zClipUV *destUvs = scratchUvsA;
    float *destAttr0 = scratchAttr0A;
    float *destAttr1 = scratchAttr1A;
    float *destAttr2 = scratchAttr2A;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts, sourceUvs, sourceAttr0, sourceAttr1, sourceAttr2, count, destVerts,
            destUvs, destAttr0, destAttr1, destAttr2, 0, clipRect->xMin, true);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        destVerts = scratchVertsB;
        destUvs = scratchUvsB;
        destAttr0 = scratchAttr0B;
        destAttr1 = scratchAttr1B;
        destAttr2 = scratchAttr2B;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts, sourceUvs, sourceAttr0, sourceAttr1, sourceAttr2, count, destVerts,
            destUvs, destAttr0, destAttr1, destAttr2, 0, clipRect->xMaxAlt, false);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts, sourceUvs, sourceAttr0, sourceAttr1, sourceAttr2, count, destVerts,
            destUvs, destAttr0, destAttr1, destAttr2, 1, clipRect->yMin, true);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts, sourceUvs, sourceAttr0, sourceAttr1, sourceAttr2, count, destVerts,
            destUvs, destAttr0, destAttr1, destAttr2, 1, clipRect->yMaxAlt, false);
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(g_Clip_PolyVerts, sourceVerts,
                    static_cast<size_t>(outputCount) * sizeof(zClipVert));
        memcpy(g_Clip_PolyUvs, sourceUvs,
                    static_cast<size_t>(outputCount) * sizeof(zClipUV));
        memcpy(g_Clip_PolyAttr0, sourceAttr0,
                    static_cast<size_t>(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr2, sourceAttr2,
                    static_cast<size_t>(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr1, sourceAttr1,
                    static_cast<size_t>(outputCount) * sizeof(float));
    }
    return 1;
}
} // namespace

namespace zClipRect {
// Reimplements 0x47aa80: zClipRect::ClipPolyNearZ
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyNearZ(zClipRectPartial *clipRect,
                                                           int *vertexCount) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    zClipUV clippedUvs[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        bool prevInside = IsInsideNear(prevVert, clipRect->zMin);

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const bool currInside = IsInsideNear(currVert, clipRect->zMin);

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClipped(clippedVerts, clippedUvs, outputCount,
                              InterpolateVert(prevVert, currVert, t, clipRect->zMin),
                              InterpolateUv(prevUv, currUv, t));
            }

            if (currInside) {
                AppendClipped(clippedVerts, clippedUvs, outputCount, currVert, currUv);
            }

            prevVert = currVert;
            prevUv = currUv;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(g_Clip_PolyVertsScratch, clippedVerts,
                static_cast<size_t>(outputCount) * sizeof(zClipVert));
    memcpy(g_Clip_PolyUvs, clippedUvs,
                static_cast<size_t>(outputCount) * sizeof(zClipUV));
    return 1;
}

// Reimplements 0x47af60: zClipRect::ClipPolyNearZ_WithAttr0
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyNearZ_WithAttr0(zClipRectPartial *clipRect,
                                                                     int *vertexCount) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    zClipUV clippedUvs[kClipBufferCapacity] = {0};
    float clippedAttrs[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        float prevAttr = g_Clip_PolyAttr0[count - 1];
        bool prevInside = IsInsideNear(prevVert, clipRect->zMin);

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const float currAttr = g_Clip_PolyAttr0[i];
            const bool currInside = IsInsideNear(currVert, clipRect->zMin);

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedWithAttr(clippedVerts, clippedUvs, clippedAttrs, outputCount,
                                      InterpolateVert(prevVert, currVert, t, clipRect->zMin),
                                      InterpolateUv(prevUv, currUv, t),
                                      InterpolateFloat(prevAttr, currAttr, t));
            }

            if (currInside) {
                AppendClippedWithAttr(clippedVerts, clippedUvs, clippedAttrs, outputCount, currVert,
                                      currUv, currAttr);
            }

            prevVert = currVert;
            prevUv = currUv;
            prevAttr = currAttr;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(g_Clip_PolyVertsScratch, clippedVerts,
                static_cast<size_t>(outputCount) * sizeof(zClipVert));
    memcpy(g_Clip_PolyUvs, clippedUvs,
                static_cast<size_t>(outputCount) * sizeof(zClipUV));
    memcpy(g_Clip_PolyAttr0, clippedAttrs,
                static_cast<size_t>(outputCount) * sizeof(float));
    return 1;
}

// Reimplements 0x47a200: zClipRect::ClipPolyZRange_NoUV
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyZRange_NoUV(zClipRectPartial *clipRect,
                                                                 int *vertexCount) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        bool prevInside = IsInsideNear(prevVert, clipRect->zMin);

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const bool currInside = IsInsideNear(currVert, clipRect->zMin);

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedVert(clippedVerts, outputCount,
                                  InterpolateVert(prevVert, currVert, t, clipRect->zMin));
            }

            if (currInside) {
                AppendClippedVert(clippedVerts, outputCount, currVert);
            }

            prevVert = currVert;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(g_Clip_PolyVertsScratch, clippedVerts,
                static_cast<size_t>(outputCount) * sizeof(zClipVert));
    return 1;
}

// Reimplements 0x47a4e0: zClipRect::ClipPolyZRange_NoUV_WithAttribs
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPolyZRange_NoUV_WithAttribs(zClipRectPartial *clipRect, int *vertexCount) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    float clippedAttr0[kClipBufferCapacity] = {0};
    float clippedAttr1[kClipBufferCapacity] = {0};
    float clippedAttr2[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        float prevAttr0 = g_Clip_PolyAttr0[count - 1];
        float prevAttr1 = g_Clip_PolyAttr1[count - 1];
        float prevAttr2 = g_Clip_PolyAttr2[count - 1];
        bool prevInside = IsInsideNear(prevVert, clipRect->zMin);

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const float currAttr0 = g_Clip_PolyAttr0[i];
            const float currAttr1 = g_Clip_PolyAttr1[i];
            const float currAttr2 = g_Clip_PolyAttr2[i];
            const bool currInside = IsInsideNear(currVert, clipRect->zMin);

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedVertWithAttr012(clippedVerts, clippedAttr0, clippedAttr1, clippedAttr2,
                                             outputCount,
                                             InterpolateVert(prevVert, currVert, t, clipRect->zMin),
                                             InterpolateFloat(prevAttr0, currAttr0, t),
                                             InterpolateFloat(prevAttr1, currAttr1, t),
                                             InterpolateFloat(prevAttr2, currAttr2, t));
            }

            if (currInside) {
                AppendClippedVertWithAttr012(clippedVerts, clippedAttr0, clippedAttr1, clippedAttr2,
                                             outputCount, currVert, currAttr0, currAttr1,
                                             currAttr2);
            }

            prevVert = currVert;
            prevAttr0 = currAttr0;
            prevAttr1 = currAttr1;
            prevAttr2 = currAttr2;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(g_Clip_PolyVertsScratch, clippedVerts,
                static_cast<size_t>(outputCount) * sizeof(zClipVert));
    memcpy(g_Clip_PolyAttr0, clippedAttr0,
                static_cast<size_t>(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr1, clippedAttr1,
                static_cast<size_t>(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr2, clippedAttr2,
                static_cast<size_t>(outputCount) * sizeof(float));
    return 1;
}

// Reimplements 0x47e900: zClipRect::ClipPolyZRange_WithAttr012
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyZRange_WithAttr012(zClipRectPartial *clipRect,
                                                                        int *vertexCount) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    zClipUV clippedUvs[kClipBufferCapacity] = {0};
    float clippedAttr0[kClipBufferCapacity] = {0};
    float clippedAttr1[kClipBufferCapacity] = {0};
    float clippedAttr2[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        float prevAttr0 = g_Clip_PolyAttr0[count - 1];
        float prevAttr1 = g_Clip_PolyAttr1[count - 1];
        float prevAttr2 = g_Clip_PolyAttr2[count - 1];
        bool prevInside = IsInsideNear(prevVert, clipRect->zMin);

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const float currAttr0 = g_Clip_PolyAttr0[i];
            const float currAttr1 = g_Clip_PolyAttr1[i];
            const float currAttr2 = g_Clip_PolyAttr2[i];
            const bool currInside = IsInsideNear(currVert, clipRect->zMin);

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedWithAttr012(
                    clippedVerts, clippedUvs, clippedAttr0, clippedAttr1, clippedAttr2, outputCount,
                    InterpolateVert(prevVert, currVert, t, clipRect->zMin),
                    InterpolateUv(prevUv, currUv, t), InterpolateFloat(prevAttr0, currAttr0, t),
                    InterpolateFloat(prevAttr1, currAttr1, t),
                    InterpolateFloat(prevAttr2, currAttr2, t));
            }

            if (currInside) {
                AppendClippedWithAttr012(clippedVerts, clippedUvs, clippedAttr0, clippedAttr1,
                                         clippedAttr2, outputCount, currVert, currUv, currAttr0,
                                         currAttr1, currAttr2);
            }

            prevVert = currVert;
            prevUv = currUv;
            prevAttr0 = currAttr0;
            prevAttr1 = currAttr1;
            prevAttr2 = currAttr2;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(g_Clip_PolyVertsScratch, clippedVerts,
                static_cast<size_t>(outputCount) * sizeof(zClipVert));
    memcpy(g_Clip_PolyUvs, clippedUvs,
                static_cast<size_t>(outputCount) * sizeof(zClipUV));
    memcpy(g_Clip_PolyAttr0, clippedAttr0,
                static_cast<size_t>(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr2, clippedAttr2,
                static_cast<size_t>(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr1, clippedAttr1,
                static_cast<size_t>(outputCount) * sizeof(float));
    return 1;
}

// Reimplements 0x47b540: zClipRect::ClipPoly_NoUV_Alt
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_NoUV_Alt(zClipRectPartial *clipRect,
                                                               int *vertexCount) {
    return ClipPolyNoUvCore(clipRect, vertexCount);
}

// Reimplements 0x47cdc0: zClipRect::ClipPoly_NoUV
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_NoUV(zClipRectPartial *clipRect,
                                                           int *vertexCount) {
    return ClipPolyNoUvCore(clipRect, vertexCount);
}

// Reimplements 0x47d3f0: zClipRect::ClipPoly
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly(zClipRectPartial *clipRect,
                                                      int *vertexCount) {
    return ClipPolyUvCore(clipRect, vertexCount);
}

// Reimplements 0x47efd0: zClipRect::ClipPoly_WithAttr012
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_WithAttr012(zClipRectPartial *clipRect,
                                                                  int *vertexCount) {
    return ClipPolyAttr012UvCore(clipRect, vertexCount);
}

// Reimplements 0x47dfb0: zClipRect::ClipPoly_NoUV_WithAttr0_Alt
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPoly_NoUV_WithAttr0_Alt(zClipRectPartial *clipRect, int *vertexCount) {
    return ClipPolyAttr0NoUvCore(clipRect, vertexCount);
}

// Reimplements 0x47bd30: zClipRect::ClipPoly_NoUV_WithAttr012_Alt
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPoly_NoUV_WithAttr012_Alt(zClipRectPartial *clipRect, int *vertexCount) {
    return ClipPolyAttr012NoUvCore(clipRect, vertexCount);
}

// Reimplements 0x4803b0: zClipRect::TrivialRejectPolyXY
RECOIL_NOINLINE int RECOIL_FASTCALL TrivialRejectPolyXY(zClipRectPartial *clipRect,
                                                                 int vertexCount) {
    const int flags = clipRect->flags;
    if (flags == 0) {
        return 1;
    }

    if ((flags & 0x01) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].x >= clipRect->xMin) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    if ((flags & 0x02) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].x < clipRect->xMax) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    if ((flags & 0x04) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].y >= clipRect->yMin) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    if ((flags & 0x08) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].y < clipRect->yMax) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    return 1;
}
} // namespace zClipRect
