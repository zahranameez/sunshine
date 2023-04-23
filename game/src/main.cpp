#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <iostream>

using namespace std;

int main(void)
{
    InitWindow(1280, 720, "Sunshine");
    rlImGuiSetup(true);
    InitAudioDevice();

    Sound sound = LoadSound("../game/assets/audio/yay.ogg");
    Music music = LoadMusicStream("../game/assets/audio/ncs_time_leap_aero_chord.mp3");
    bool musicPaused = true;

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

        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();

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
