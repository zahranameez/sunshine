#include "rlImGui.h"
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    InitAudioDevice();
    SetTargetFPS(60);

    Texture enterprise = LoadTexture("../game/assets/textures/enterprise.png");
    Sound yay = LoadSound("../game/assets/audio/yay.ogg");
    Music music = LoadMusicStream("../game/assets/audio/ncs_time_leap_aero_chord.mp3");
    PlaySound(yay);
    PlayMusicStream(music);
    while (!WindowShouldClose())
    {
        UpdateMusicStream(music);
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello World!", 16, 9, 20, RED);
        DrawTexture(enterprise, 0, 0, WHITE);
        EndDrawing();
    }

    UnloadTexture(enterprise);
    UnloadMusicStream(music);
    UnloadSound(yay);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}