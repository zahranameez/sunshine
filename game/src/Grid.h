#pragma once
#include "raylib.h"
#include "Collision.h"

constexpr int GRID_LENGTH = 80;
constexpr int GRID_LENGTH_SQR = GRID_LENGTH * GRID_LENGTH;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_WIDTH = SCREEN_WIDTH / GRID_LENGTH;
constexpr int TILE_HEIGHT = SCREEN_HEIGHT / GRID_LENGTH;

inline size_t ScreenToGrid(Vector2 point)
{
    size_t col = point.x / TILE_WIDTH;
    size_t row = point.y / TILE_HEIGHT;
    return row * GRID_LENGTH + col;
}

inline Vector2 GridToScreen(size_t index)
{
    size_t col = index % GRID_LENGTH;
    size_t row = index / GRID_LENGTH;
    return { float(col * TILE_WIDTH), float(row * TILE_HEIGHT) };
}

inline std::vector<size_t> OverlapTiles(Rectangle rectangle)
{
    if (rectangle.x < 0.0f) rectangle.x = 0.0f;
    if (rectangle.y < 0.0f) rectangle.y = 0.0f;
    if (rectangle.x + rectangle.width > SCREEN_WIDTH) rectangle.x = SCREEN_WIDTH - rectangle.width;
    if (rectangle.y + rectangle.height > SCREEN_HEIGHT) rectangle.y = SCREEN_HEIGHT - rectangle.height;

    const size_t colMin = rectangle.x / TILE_WIDTH;
    const size_t rowMin = rectangle.y / TILE_HEIGHT;
    const size_t colMax = (rectangle.x + rectangle.width) / TILE_WIDTH;
    const size_t rowMax = (rectangle.y + rectangle.height) / TILE_HEIGHT;

    std::vector<size_t> indices;
    indices.reserve((colMax - colMin) * (rowMax - rowMin));
    for (size_t row = rowMin; row < rowMax; row++)
    {
        for (size_t col = colMin; col < colMax; col++)
        {
            indices.push_back(row * GRID_LENGTH + col);
        }
    }
    return indices;
}

inline std::vector<size_t> VisibleTiles(Circle target, float detectionRadius,
    const Obstacles& obstacles, const std::vector<size_t>& tiles)
{
    std::vector<size_t> visibilityTiles;
    visibilityTiles.reserve(tiles.size());
    for (size_t i : tiles)
    {
        Vector2 tileCenter = GridToScreen(i) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
        Vector2 tileEnd = tileCenter + Normalize(target.position - tileCenter) * detectionRadius;
        if (IsCircleVisible(tileCenter, tileEnd, target, obstacles)) visibilityTiles.push_back(i);
    }
    return visibilityTiles;
}