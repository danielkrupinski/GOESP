#pragma once

#include "Vector.h"
#include "VirtualMethod.h"

#include <cstddef>
#include <functional>

struct Ray {
    Ray(const Vector& src, const Vector& dest) : start(src), delta(dest - src) { isSwept = delta.x || delta.y || delta.z; }
    alignas(16) Vector start;
    alignas(16) Vector delta;
    alignas(16) Vector startOffset{};
    alignas(16) Vector extents{};
    alignas(16) const Matrix3x4* worldAxisTransform = nullptr;
    bool isRay = true;
    bool isSwept = false;
};

class Entity;

struct TraceFilter {
    TraceFilter(const Entity* entity) : skip{ entity } { }
    virtual bool shouldHitEntity(Entity* entity, int contentsMask) { return entity != skip; }
    virtual int getTraceType() const { return 0; }
    const void* skip;
};

struct Trace {
    Vector startpos;
    Vector endpos;
    std::byte pad[20];
    float fraction;
    int contents;
    unsigned short dispFlags;
    bool allSolid;
    bool startSolid;
    float fractionLeftSolid;

    struct Surface {
        const char* name;
        short surfaceProps;
        unsigned short flags;
    } surface;

    int hitgroup;
    short physicsBone;
    unsigned short worldSurfaceIndex;
    Entity* entity;
    int hitbox;
};

class EngineTrace {
public:
    VIRTUAL_METHOD(void, traceRay, 5, (const Ray& ray, unsigned int mask, const TraceFilter& filter, Trace& trace), (this, std::cref(ray), mask, std::cref(filter), std::ref(trace)))
};
