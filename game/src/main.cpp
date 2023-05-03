#include "rlImGui.h"
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);

    const float radius = 20.0f;
    Vector2 position{ SCREEN_WIDTH * 0.5f - radius * 0.5f, SCREEN_HEIGHT * 0.5f - radius - 0.5f };

    bool useGUI = false;
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawCircleV(position, radius, RED);

        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::SliderFloat2("Position", &position.x, 0.0f, SCREEN_WIDTH);

            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        DrawText("Press ~ to open/close GUI", 10, 30, 20, GRAY);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}