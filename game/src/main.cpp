#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <iostream>

using namespace std;

void DrawTextureCircle(const Texture& texture, const Circle& circle,
float rotation/*degrees*/ = 0.0f, Color tint = WHITE)
{
    Rectangle src{ 0.0f, 0.0f, texture.width, texture.height };
    Rectangle dst{ circle.position.x, circle.position.y, circle.radius * 2.0f, circle.radius * 2.0f };
    DrawTexturePro(texture, src, dst, { dst.width * 0.5f, dst.height * 0.5f }, rotation, tint);
}

int main(void)
{
    InitWindow(1280, 720, "Sunshine");
    rlImGuiSetup(true);
    InitAudioDevice();

    Sound sound = LoadSound("../game/assets/audio/yay.ogg");
    Music music = LoadMusicStream("../game/assets/audio/ncs_time_leap_aero_chord.mp3");
    bool musicPaused = true;

    Texture texObstacle = LoadTexture("../game/assets/textures/nebula.png");
    Obstacles obstacles = LoadObstacles();

    bool drawColliders = true;
    bool useGUI = false;
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateMusicStream(music);

        if (musicPaused)
            PauseMusicStream(music);
        else
            PlayMusicStream(music);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (const Circle& obstacle : obstacles)
        {
            if (drawColliders)
                DrawCircleV(obstacle.position, obstacle.radius, GRAY);
            DrawTextureCircle(texObstacle, obstacle);
        }

        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("Draw Colliders", &drawColliders);

            if (ImGui::Button("Play Sound"))
                PlaySound(sound);

            if (musicPaused)
            {
                if (ImGui::Button("Play Music"))
                    musicPaused = false;
            }
            else
            {
                if (ImGui::Button("Pause Music"))
                    musicPaused = true;
            }

            if (ImGui::Button("Restart Music"))
                SeekMusicStream(music, 0.0f);

            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadSound(sound);
    UnloadMusicStream(music);

    CloseAudioDevice();
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
