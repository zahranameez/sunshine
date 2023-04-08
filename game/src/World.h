#pragma once
#include "raylib.h"
#include "Physics.h"
#include "Geometry.h"
#include <string>

// Used to ensure we don't do stuff like fire projectiles every frame of the attack action
enum ActionType
{
    PATROL,
    FIND_VISIBILITY,
    FIND_COVER,
    SEEK,
    FLEE,
    ARRIVE,
    CLOSE_ATTACK,
    RANGED_ATTACK,
    WAIT,
};

struct Entity : public Rigidbody
{
    std::string name;

    float radius = 0.0f;
    Circle Collider() const { return { pos, radius }; }

    const float maxHealth = 100.0f;
    float health = maxHealth;
    float HealthPercent() const { return health / maxHealth; }

    // Copy-assignment needed because compiler doesn't know whether to copy-initialized health to maxHealth or entity.health
    Entity& operator=(const Entity& entity)
    {
        name = entity.name;
        radius = entity.radius;
        health = entity.health;
        return *this;
    }
};

struct Enemy : public Entity
{
    float speed = 0.0f;
    size_t point = 0;

    float detectionRadius = 0.0f;
    float combatRadius = 0.0f;
    float probeLength = 0.0f;

    ActionType prev = WAIT, curr = WAIT;
};

struct Player : public Entity
{
};

struct Projectile : public Entity
{
    float damage = 0.0f;
};

using Projectiles = std::vector<Projectile>;

struct World
{
    Points points;
    Obstacles obstacles;
    Projectiles projectiles;
};
