#include "raylib.h"
#include "math.h"

#include "imgui.h"
#include "rlImGui.h"

#include <string>
#include <cstdio>

using namespace std;

struct Rigidbody
{
    Vector3 vel{ 0.0f, 0.0f, 0.0f };
    Vector3 acc{ 0.0f, 0.0f, 0.0f };
};

// v2 = v1 + a(t)
// p2 = p1 + v2(t) + 0.5a(t^2)
Vector3 Integrate(const Vector3& pos, Rigidbody& rb, float dt)
{
    rb.vel = rb.vel + rb.acc * dt;
    return pos + rb.vel * dt + rb.acc * dt * dt * 0.5f;
}

// vf^2 = vi^2 + 2a(d)
// 0^2 = vi^2 + 2a(d)
// -vi^2 / 2d = a
Vector3 Decelerate(
    const Vector3& targetPosition,
    const Vector3& seekerPosition,
    const Vector3& seekerVelocity)
{
    float d = Length(targetPosition - seekerPosition);
    float a = Dot(seekerVelocity, seekerVelocity) / (d * 2.0f);

    return Negate(Normalize(seekerVelocity)) * a;
}

// Accelerate towards target
Vector3 Seek(
    const Vector3& targetPosition,
    const Vector3& seekerPosition,
    const Vector3& seekerVelocity, float maxSpeed)
{
    Vector3 desiredVelocity = Normalize(targetPosition - seekerPosition) * maxSpeed;
    return desiredVelocity - seekerVelocity;
}

Matrix RotateAt(const Vector3& seeker, const Vector3& target, Vector3 up = { 0.0f, 1.0f, 0.0f })
{
    Vector3 forward = Normalize(Subtract(target, seeker));
    Vector3 right = Cross(forward, up);
    Vector3 above = Cross(forward, right);
    Matrix orientation = MatrixIdentity();

    orientation.m0 = right.x;
    orientation.m1 = up.x;
    orientation.m2 = forward.x;

    orientation.m4 = right.y;
    orientation.m5 = up.y;
    orientation.m6 = forward.y;

    orientation.m8 = right.z;
    orientation.m9 = up.z;
    orientation.m10 = forward.z;

    orientation.m15 = 1.0f;

    // See MatrixLookAt for reference on row vs column major
    return orientation;
}

Matrix MatrixTransform(const Transform& transform)
{
    Matrix scale = Scale(transform.scale.x, transform.scale.y, transform.scale.z);
    Matrix rotation = ToMatrix(transform.rotation);
    Matrix translation = Translate(transform.translation.x, transform.translation.y, transform.translation.z);
    return Multiply(scale, Multiply(rotation, translation));
}

void LineTowards(const Vector3& seeker, const Vector3& target, float distance = 100.0f, Color color = RED)
{
    Vector3 forward = Normalize(target - seeker);
    DrawLine3D(seeker, seeker + forward * distance, color);
}

void Print3(const Vector3& vec)
{
    printf("x: %f y: %f z: %f\n", vec.x, vec.y, vec.z);
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Sunshine");
    rlImGuiSetup(true);

    string assets = "../game/assets";
    Model plane = LoadModel((assets + "/models/plane.obj").c_str());
    Texture2D texture = LoadTexture((assets + "/textures/plane_diffuse.png").c_str());
    plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    Camera camera = { 0 };
    camera.position = { 0.0f, 750.0f, 10.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 30.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Camera2D camera2D = { 0 };
    camera2D.target = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera2D.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera2D.rotation = 0.0f;
    camera2D.zoom = 1.0f;

    Rigidbody seekerBody;
    Vector3 seekerTargetOrigin{ -250.0f, 0.0f, 0.0f };
    Vector3 seekerTarget{ 0.0f, 0.0f, 0.0f };
    Vector3 seekerPosition{ -300.0f, 0.0f, 0.0f };
    Matrix seekerTransform;

    Rigidbody arriverBody;
    Vector3 arriverTarget{ 0.0f, 0.0f, 0.0f };
    Vector3 arriverPosition{ 300.0f, 0.0f, 0.0f };
    Matrix arriverTransform;

    Transform testTransform;
    testTransform.translation = { 0.0f, 0.0f, 100.0f };
    testTransform.rotation = FromEuler(0.0f, 0.0f, 0.0f);
    testTransform.scale = Vector3One();

    bool demoGUI = true;
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Seek begin
        seekerBody.acc = Seek(seekerTarget, seekerPosition, seekerBody.vel, 100.0f);
        seekerPosition = Integrate(seekerPosition, seekerBody, dt);

        seekerTarget.x = seekerTargetOrigin.x + sin(GetTime()) * 100.0f;
        seekerTarget.z = seekerTargetOrigin.z + cos(GetTime()) * 100.0f;

        Matrix seekerTranslation = Translate(seekerPosition.x, 0.0f, seekerPosition.z);
        Matrix seekerRotation = RotateAt(seekerPosition, seekerTarget);
        seekerTransform = Multiply(seekerRotation, seekerTranslation);
        // Seek end

        // Arrive begin
        Vector3 arriverSteeringForce = Seek(arriverTarget, arriverPosition, arriverBody.vel, 1000.0f);
        arriverBody.vel = arriverBody.vel + arriverSteeringForce * dt;
        arriverBody.acc = Decelerate(arriverTarget, arriverPosition, arriverBody.vel);
        arriverPosition = Integrate(arriverPosition, arriverBody, dt);

        Matrix arriverTranslation = Translate(arriverPosition.x, 0.0f, arriverPosition.z);
        Matrix arriverRotation = RotateAt(arriverPosition, arriverTarget);
        arriverTransform = Multiply(arriverRotation, arriverTranslation);
        // Arrive end

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);

        if (IsKeyPressed(KEY_GRAVE)) demoGUI = !demoGUI;
        if (demoGUI)
        {
            rlImGuiBegin();
            ImGui::ShowDemoWindow(nullptr);
            rlImGuiEnd();
        }

        BeginMode3D(camera);
            plane.transform = seekerTransform;
            DrawModel(plane, Vector3Zero(), 1.0f, WHITE);
            DrawSphere(seekerTarget, 50.0f, ORANGE);
            LineTowards(seekerPosition, seekerTarget);

            plane.transform = arriverTransform;
            DrawModel(plane, Vector3Zero(), 1.0f, WHITE);
            DrawSphere(arriverTarget, 50.0f, ORANGE);
            LineTowards(arriverPosition, arriverTarget);
        EndMode3D();

        EndDrawing();
    }

    UnloadModel(plane);
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
