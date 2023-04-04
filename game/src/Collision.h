#pragma once
#include "raylib.h"
#include "Math.h"
#include <array>
#include <vector>
#include <algorithm>

struct Circle
{
    Vector2 position{};
    float radius = 0.0f;
};

using Obstacles = std::vector<Circle>;

Rectangle From(Circle circle)
{
    return {
        circle.position.x - circle.radius, circle.position.y - circle.radius,
        circle.radius * 2.0f, circle.radius * 2.0f
    };
}

Circle From(Rectangle rectangle)
{
    Vector2 center = { rectangle.x + rectangle.width * 0.5f, rectangle.y + rectangle.height * 0.5f };
    return { center, std::max(rectangle.width, rectangle.height) * 0.5f };
}

void DrawCircle(Circle circle, Color color)
{
    DrawCircleV(circle.position, circle.radius, color);
}

bool CheckCollisionPointCircle(Vector2 point, Circle circle)
{
    return DistanceSqr(circle.position, point) <= circle.radius * circle.radius;
}

bool CheckCollisionCircles(Circle circle1, Circle circle2)
{
    return DistanceSqr(circle1.position, circle2.position) <=
        (circle1.radius * circle1.radius) + (circle2.radius * circle2.radius);
}

bool CheckCollisionLineCircle(Vector2 lineStart, Vector2 lineEnd, Circle circle)
{
    Vector2 nearest = NearestPoint(lineStart, lineEnd, circle.position);
    return DistanceSqr(nearest, circle.position) <= circle.radius * circle.radius;
}

bool CheckCollisionLineCircle(Vector2 lineStart, Vector2 lineEnd, Circle circle, Vector2& poi)
{
    Vector2 dc = lineStart - circle.position;
    Vector2 dx = lineEnd - lineStart;
    float a = LengthSqr(dx);
    float b = Dot(dx, dc) * 2.0f;
    float c = LengthSqr(dc) - circle.radius * circle.radius;
    float det = b * b - 4.0f * a * c;

    if (a <= FLT_EPSILON || det < 0.0f) return false;

    det = sqrtf(det);
    float t1 = (-b - det) / (2.0f * a);
    float t2 = (-b + det) / (2.0f * a);

    Vector2 poiA{ lineStart + dx * t1 };
    Vector2 poiB{ lineStart + dx * t2 };

    // Line is infinite so we must restrit it between start and end via bounding rectangle
    float xMin = std::min(lineStart.x, lineEnd.x);
    float xMax = std::max(lineStart.x, lineEnd.x);
    float yMin = std::min(lineStart.y, lineEnd.y);
    float yMax = std::max(lineStart.y, lineEnd.y);
    Rectangle rec{ xMin, yMin, xMax - xMin, yMax - yMin };

    bool collisionA = CheckCollisionPointRec(poiA, rec);
    bool collisionB = CheckCollisionPointRec(poiB, rec);
    if (collisionA && collisionB)
    {
        poi = DistanceSqr(lineStart, poiA) < DistanceSqr(lineStart, poiB) ? poiA : poiB;
        return true;
    }
    if (collisionA)
    {
        poi = poiA;
        return true;
    }
    if (collisionB)
    {
        poi = poiB;
        return true;
    }
    return false;
}

bool NearestIntersection(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles, Vector2& poi)
{
    std::vector<Vector2> intersections;
    intersections.reserve(obstacles.size());

    for (const Circle& obstacle : obstacles)
    {
        Vector2 poi;
        if (CheckCollisionLineCircle(lineStart, lineEnd, obstacle, poi))
            intersections.push_back(std::move(poi));
    }

    if (!intersections.empty())
    {
        poi = *std::min_element(intersections.begin(), intersections.end(),
            [&lineStart](const Vector2& a, const Vector2& b)
            {
                return DistanceSqr(lineStart, a) < DistanceSqr(lineStart, b);
            }
        );
    }

    return !intersections.empty();
}

bool IsPointVisible(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        if (CheckCollisionLineCircle(lineStart, lineEnd, obstacle))
            return false;
    }
    return true;
}

bool IsCircleVisible(Vector2 lineStart, Vector2 lineEnd, Circle circle, const Obstacles& obstacles)
{
    Vector2 circlePoi;
    bool circleCollision = CheckCollisionLineCircle(lineStart, lineEnd, circle, circlePoi);
    if (!circleCollision) return false;

    Vector2 obstaclePoi;
    bool obstacleCollision = NearestIntersection(lineStart, lineEnd, obstacles, obstaclePoi);
    if (!obstacleCollision) return true;

    return DistanceSqr(circlePoi, lineStart) < DistanceSqr(obstaclePoi, lineStart);
}