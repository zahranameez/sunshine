#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <iostream>

using namespace std;

int main(void)
{
    InitWindow(1280, 720, "Sunshine");
    InitAudioDevice();
    rlImGuiSetup(true);

    bool useGUI = false;
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            if (ImGui::Button("Click Me"))
                cout << "Thank you ever so much for clicking me. I am forever in your debt!" << endl;
            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
