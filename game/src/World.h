#pragma once
#include "raylib.h"
#include "Physics.h"
#include "Geometry.h"
#include <string>

struct Entity : public Rigidbody
{
    std::string name;
    float radius = 0.0f;
    Circle Collider() const { return { pos, radius }; }
};

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

struct Enemy : public Entity
{
    float speed = 0.0f;
    size_t point = 0;

    float detectionRadius = 0.0f;
    float combatRadius = 0.0f;
    float probeLength = 0.0f;

    ActionType prev = WAIT, curr = WAIT;
};

struct Player : public Entity {};

struct World
{
    Points points;
    Obstacles obstacles;
    std::vector<Entity> projectiles;
};
