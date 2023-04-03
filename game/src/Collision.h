#pragma once
#include "raylib.h"
#include "Math.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cassert>

Vector2 Nearest(const Vector2& point, const std::vector<Vector2>& points)
{
    assert(!points.empty());
    return *std::min_element(points.begin(), points.end(),
        [&point](const Vector2& a, const Vector2& b)
        {
            return DistanceSqr(point, a) < DistanceSqr(point, b);
        }
    );
}

struct Circle
{
    Vector2 position;
    float radius;
};

using Polygon = std::vector<Vector2>;

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

    Vector2 pois[2]
    {
        Vector2 { lineStart + dx * t1 },
        Vector2 { lineStart + dx * t2 }
    };

    poi = DistanceSqr(lineStart, pois[0]) < DistanceSqr(lineStart, pois[1]) ? pois[0] : pois[1];
    return true;
}

bool CheckCollisionLinePolygon(Vector2 lineStart, Vector2 lineEnd, const Polygon& polygon)
{
    for (size_t i = 0; i < polygon.size(); i++)
    {
        if (CheckCollisionLines(lineStart, lineEnd, polygon[i], polygon[(i + 1) % polygon.size()], nullptr))
            return true;
    }
    return false;
}

bool CheckCollisionPolygons(const Polygon& polygon1, const Polygon& polygon2)
{
    for (size_t i = 0; i < polygon1.size(); i++)
    {
        Vector2 p1A = polygon1[i];
        Vector2 p1B = polygon1[(i + 1) % polygon1.size()];
        for (size_t j = 0; j < polygon2.size(); j++)
        {
            Vector2 p2A = polygon2[j];
            Vector2 p2B = polygon2[(j + 1) % polygon1.size()];
            if (CheckCollisionLines(p1A, p1B, p2A, p2B, nullptr))
                return true;
        }
    }
    return false;
}

std::vector<Vector2> CheckIntersectionLinePolygon(Vector2 lineStart, Vector2 lineEnd, const Polygon& polygon)
{
    std::vector<Vector2> intersections;
    intersections.reserve(polygon.size());
    for (size_t i = 0; i < polygon.size(); i++)
    {
        Vector2 point;
        if (CheckCollisionLines(lineStart, lineEnd, polygon[i], polygon[(i + 1) % polygon.size()], &point))
            intersections.push_back(point);
    }
    return intersections;
}

std::vector<Vector2> CheckIntersectionPolygons(Polygon& polygon1, const Polygon& polygon2)
{
    std::vector<Vector2> intersections;
    intersections.reserve(std::min(polygon1.size(), polygon2.size()));
    for (size_t i = 0; i < polygon1.size(); i++)
    {
        Vector2 p1A = polygon1[i];
        Vector2 p1B = polygon1[(i + 1) % polygon1.size()];
        for (size_t j = 0; j < polygon2.size(); j++)
        {
            Vector2 p2A = polygon2[j];
            Vector2 p2B = polygon2[(j + 1) % polygon1.size()];
            Vector2 poi;
            if (CheckCollisionLines(p1A, p1B, p2A, p2B, &poi))
                intersections.push_back(poi);
        }
    }
    return intersections;
}

void DrawPolygon(const Polygon& polygon, Color color, float thickness = 1.0f)
{
    for (size_t i = 0; i < polygon.size(); i++)
    {
        Vector2 lineStart = polygon[i];
        Vector2 lineEnd = polygon[(i + 1) % polygon.size()];
        DrawLineEx(lineStart, lineEnd, thickness, color);
    }
}

bool NearestIntersection(Vector2 lineStart, Vector2 lineEnd,
    const std::vector<Polygon>& polygons, Vector2& poi)
{
    std::vector<Vector2> intersectionsOuter;
    intersectionsOuter.reserve(polygons.size());

    for (const Polygon& polygon : polygons)
    {
        std::vector<Vector2> intersectionsInner = CheckIntersectionLinePolygon(lineStart, lineEnd, polygon);
        if (!intersectionsInner.empty())
            intersectionsOuter.push_back(Nearest(lineStart, intersectionsInner));
    }

    if (!intersectionsOuter.empty())
        poi = Nearest(lineStart, intersectionsOuter);

    return !intersectionsOuter.empty();
}

bool IsPointVisible(Vector2 lineStart, Vector2 lineEnd, const std::vector<Polygon>& obstacles)
{
    for (const Polygon& obstacle : obstacles)
    {
        if (CheckCollisionLinePolygon(lineStart, lineEnd, obstacle))
            return false;
    }
    return true;
}

bool IsCircleVisible(Vector2 lineStart, Vector2 lineEnd, Circle circle, const std::vector<Polygon>& obstacles)
{
    Vector2 circlePoi;
    bool circleCollision = CheckCollisionLineCircle(lineStart, lineEnd, circle, circlePoi);
    if (!circleCollision) return false;

    Vector2 polygonPoi;
    bool polygonCollision = NearestIntersection(lineStart, lineEnd, obstacles, polygonPoi);
    if (!polygonCollision) return true;

    return DistanceSqr(circlePoi, lineStart) < DistanceSqr(polygonPoi, lineStart);
}

bool IsPolygonVisible(Vector2 lineStart, Vector2 lineEnd,
    const Polygon& target, const std::vector<Polygon>& obstacles)
{
    std::vector<Vector2> targetIntersections = CheckIntersectionLinePolygon(lineStart, lineEnd, target);
    if (targetIntersections.empty()) return false;

    Vector2 obstaclePoi;
    bool obstacleCollision = NearestIntersection(lineStart, lineEnd, obstacles, obstaclePoi);
    if (!obstacleCollision) return true;

    Vector2 targetPoi = Nearest(lineStart, targetIntersections);
    return DistanceSqr(targetPoi, lineStart) < DistanceSqr(obstaclePoi, lineStart);
}
