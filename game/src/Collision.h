#pragma once
#include "raylib.h"
#include "Math.h"
#include <array>
#include <vector>
#include <algorithm>

struct Circle
{
    Vector2 position;
    float radius;
};

bool CheckCollisionLineCircle(Vector2 lineStart, Vector2 lineEnd, Circle circle)
{
    Vector2 nearest = NearestPoint(lineStart, lineEnd, circle.position);
    return DistanceSqr(nearest, circle.position) <= circle.radius * circle.radius;
}

bool CheckCollisionLineRec(Vector2 lineStart, Vector2 lineEnd, Rectangle rectangle)
{
    float xMin = rectangle.x;
    float xMax = rectangle.x + rectangle.width;
    float yMin = rectangle.y;
    float yMax = rectangle.y + rectangle.height;

    std::array<Vector2, 4> points
    {
        Vector2 {xMin, yMin},   // top left
        Vector2 {xMax, yMin},   // top right
        Vector2 {xMax, yMax},   // bot right
        Vector2 {xMin, yMax},   // bot left
    };

    for (size_t i = 0; i < points.size(); i++)
    {
        if (CheckCollisionLines(lineStart, lineEnd, points[i], points[(i + 1) % points.size()], nullptr))
            return true;
    }
    return false;
}

bool CheckCollisionLineRec(Vector2 lineStart, Vector2 lineEnd, Rectangle rectangle, Vector2& poi)
{
    float xMin = rectangle.x;
    float xMax = rectangle.x + rectangle.width;
    float yMin = rectangle.y;
    float yMax = rectangle.y + rectangle.height;

    std::array<Vector2, 4> points
    {
        Vector2 {xMin, yMin},   // top left
        Vector2 {xMax, yMin},   // top right
        Vector2 {xMax, yMax},   // bot right
        Vector2 {xMin, yMax},   // bot left
    };

    std::array<Vector2, 4> intersections;
    intersections.assign(Vector2One() * 100000.0f);

    bool collision = false;
    for (size_t i = 0; i < points.size(); i++)
    {
        if (CheckCollisionLines(lineStart, lineEnd, points[i], points[(i + 1) % points.size()], &intersections[i]))
        {
            collision = true;
        }
    }

    if (collision)
    {
        poi = *std::min_element(intersections.begin(), intersections.end(),
            [&lineStart](const Vector2& a, const Vector2& b)
            {
                return DistanceSqr(lineStart, a) < DistanceSqr(lineStart, b);
            }
        );
    }

    return collision;
}

// Determines if circle is visible from line start
bool IsCircleVisible(Vector2 lineStart, Vector2 lineEnd, Circle circle, const std::vector<Rectangle>& obstacles)
{
    if (!CheckCollisionLineCircle(lineStart, lineEnd, circle)) return false;
    float targetDistance = DistanceSqr(lineStart, circle.position);

    std::vector<Vector2> intersections(obstacles.size());
    for (const Rectangle& obstacle : obstacles)
    {
        Vector2 poi;
        bool intersects = CheckCollisionLineRec(lineStart, lineEnd, obstacle, poi);
        if (intersects)
        {
            if (DistanceSqr(lineStart, poi) < targetDistance) return false;
        }
    }

    return true;
}

// Determines if rectangle is visible from line start
bool IsRectangleVisible(Vector2 lineStart, Vector2 lineEnd, Rectangle rectangle, const std::vector<Rectangle>& obstacles)
{
    if (!CheckCollisionLineRec(lineStart, lineEnd, rectangle)) return false;
    float targetDistance = DistanceSqr(lineStart, 
        { rectangle.x + rectangle.width * 0.5f, rectangle.y + rectangle.height * 0.5f});

    std::vector<Vector2> intersections(obstacles.size());
    for (const Rectangle& obstacle : obstacles)
    {
        Vector2 poi;
        bool intersects = CheckCollisionLineRec(lineStart, lineEnd, obstacle, poi);
        if (intersects)
        {
            if (DistanceSqr(lineStart, poi) < targetDistance) return false;
        }
    }

    return true;
}

bool NearestIntersection(Vector2 lineStart, Vector2 lineEnd, const std::vector<Rectangle>& obstacles, Vector2& poi)
{
    bool collision = false;
    std::vector<Vector2> intersections(obstacles.size());
    for (const Rectangle& obstacle : obstacles)
    {
        if (CheckCollisionLineRec(lineStart, lineEnd, obstacle, poi))
        {
            intersections.push_back(poi);
            collision = true;
        }
    }

    if (collision)
    {
        poi = *std::min_element(intersections.begin(), intersections.end(),
            [&lineStart](const Vector2& a, const Vector2& b)
            {
                return DistanceSqr(lineStart, a) < DistanceSqr(lineStart, b);
            }
        );
    }

    return collision;
}
