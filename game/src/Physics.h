#pragma once
#include "raylib.h"
#include "Math.h"

struct Rigidbody
{
    Vector2 pos{};
    Vector2 vel{};
    Vector2 acc{};

    Vector2 dir{};
    float angularSpeed;
};

// v2 = v1 + a(t)
// p2 = p1 + v2(t) + 0.5a(t^2)
inline void Integrate(Rigidbody& rb, float dt)
{
    rb.vel = rb.vel + rb.acc * dt;
    rb.pos = rb.pos + rb.vel * dt + rb.acc * dt * dt * 0.5f;
    rb.dir = RotateTowards(rb.dir, Normalize(rb.vel), rb.angularSpeed * dt);
}

// vf^2 = vi^2 + 2a(d)
// 0^2 = vi^2 + 2a(d)
// -vi^2 / 2d = a
inline Vector2 Decelerate(const Vector2& pos, const Rigidbody& rb)
{
    float d = Length(pos - rb.pos);
    float a = LengthSqr(rb.vel) / (d * 2.0f);
    return Negate(Normalize(rb.vel)) * a;
}

inline Vector2 Acceleration(const Vector2& vi, const Vector2& vf, float dt)
{
    return (vf - vi) / dt;
}

// Accelerate towards target
inline Vector2 Seek(const Vector2& pos, const Rigidbody& rb, float maxSpeed)
{
    return Normalize(pos - rb.pos) * maxSpeed - rb.vel;
}

inline Vector2 Arrive(const Vector2& pos, const Rigidbody& rb, float maxSpeed, float slowRadius, float slowFactor = 1.0f)
{
    Vector2 acc = Seek(pos, rb, maxSpeed);
    float distance = Distance(pos, rb.pos);
    if (distance <= slowRadius)
    {
        float t = 1.0f - (distance / slowRadius);
        return Lerp(acc, Decelerate(pos, rb) * slowFactor, t);
    }
    return acc;
}
