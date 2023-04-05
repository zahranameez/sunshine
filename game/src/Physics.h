#pragma once
#include "Math.h"

struct Rigidbody
{
    Vector2 vel{};      // linear velocity
    Vector2 acc{};      // linear acceleration
    //Vector2 angVel{};   // angular velocity
    //Vector2 angAcc{};   // angular acceleration
};

// v2 = v1 + a(t)
// p2 = p1 + v2(t) + 0.5a(t^2)
Vector2 Integrate(const Vector2& pos, Rigidbody& rb, float dt)
{
    rb.vel = rb.vel + rb.acc * dt;
    return pos + rb.vel * dt + rb.acc * dt * dt * 0.5f;
}

// vf^2 = vi^2 + 2a(d)
// 0^2 = vi^2 + 2a(d)
// -vi^2 / 2d = a
Vector2 Decelerate(
    const Vector2& targetPosition,
    const Vector2& seekerPosition,
    const Vector2& seekerVelocity)
{
    float d = Length(targetPosition - seekerPosition);
    float a = Dot(seekerVelocity, seekerVelocity) / (d * 2.0f);

    return Negate(Normalize(seekerVelocity)) * a;
}

// Accelerate towards target
Vector2 Seek(
    const Vector2& targetPosition,
    const Vector2& seekerPosition,
    const Vector2& seekerVelocity, float maxSpeed)
{
    Vector2 desiredVelocity = Normalize(targetPosition - seekerPosition) * maxSpeed;
    return desiredVelocity - seekerVelocity;
}

// Arrive at target
void Arrive(
    const Vector2& targetPosition,
    const Vector2& seekerPosition,
    Rigidbody& seekerBody, float maxSpeed, float dt)
{
    seekerBody.vel = seekerBody.vel + Seek(targetPosition, seekerPosition, seekerBody.vel, maxSpeed) * dt;
    seekerBody.acc = Decelerate(targetPosition, seekerPosition, seekerBody.vel);
}

// Apply seek to seeker position & body
void ApplySeek(Vector2 target, Vector2& seekerPosition, Rigidbody& seekerBody, float maxSpeed, float dt)
{
    seekerBody.acc = Seek(target, seekerPosition, seekerBody.vel, maxSpeed);
    seekerPosition = Integrate(seekerPosition, seekerBody, dt);
}

// Apply arrive to seeker position & body
void ApplyArrive(Vector2 target, Vector2& seekerPosition, Rigidbody& seekerBody, float maxSpeed, float dt)
{
    seekerBody.vel = seekerBody.vel + Seek(target, seekerPosition, seekerBody.vel, maxSpeed) * dt;
    seekerBody.acc = Decelerate(target, seekerPosition, seekerBody.vel);
    seekerPosition = Integrate(seekerPosition, seekerBody, dt);
}
