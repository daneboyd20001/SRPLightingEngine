#include <raylib.h>
#include <raymath.h>
#include <rlImGui.h>
#include <rlgl.h>

#include "gui.hpp"

int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "SDF Engine");
  {
    gui gui(true);

    Camera3D cam = {
        .position = {0.0f, 0.0f, -5.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 90.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    // TODO : Why is only the first frame covering the window?
    RenderTexture mainTex =
        LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    // TODO : Compile the shader files together.
    char *compShader = LoadFileText("../shader/shader-code.glsl");

    unsigned int compShaderId = rlLoadShader(compShader, RL_COMPUTE_SHADER);
    unsigned int compProgram = rlLoadShaderProgramCompute(compShaderId);
    UnloadFileText(compShader);

    int resolutionLoc = rlGetLocationUniform(compProgram, "ScreenSize");
    int timeLoc = rlGetLocationUniform(compProgram, "time");
    int camPosLoc = rlGetLocationUniform(compProgram, "camPos");
    int forwardLoc = rlGetLocationUniform(compProgram, "camForward");
    int rightLoc = rlGetLocationUniform(compProgram, "camRight");
    int upLoc = rlGetLocationUniform(compProgram, "camUp");
    int activeSDFLoc = rlGetLocationUniform(compProgram, "activeSDF");
    int activeLightingLoc = rlGetLocationUniform(compProgram, "activeLighting");
    int lampDistLoc = rlGetLocationUniform(compProgram, "lampDist");
    int minDistLoc = rlGetLocationUniform(compProgram, "minDist");
    int fovLoc = rlGetLocationUniform(compProgram, "FOV_Tan");
    int scalarDistLoc = rlGetLocationUniform(compProgram, "scalarDist");
    int lampStrLoc = rlGetLocationUniform(compProgram, "lampStrength");

    float minDist = 0.01f;
    float scalarDist = 0.5f;
    float lampStr = 100.0f;
    float sens = 0.1;

    while (!WindowShouldClose()) {
      if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        float speed = 0.1f;
        float sprint = 3.5f;
        speed = IsKeyDown(KEY_LEFT_SHIFT) ? speed * sprint : speed;
        Vector3 velocity = Vector3Zero();
        Vector3 rotation = Vector3Zero();

        rotation.x = GetMouseDelta().x * sens;
        rotation.y = GetMouseDelta().y * sens;

        velocity.x = IsKeyDown(KEY_W) ? velocity.x + speed : velocity.x;
        velocity.x = IsKeyDown(KEY_S) ? velocity.x - speed : velocity.x;
        velocity.y = IsKeyDown(KEY_D) ? velocity.y + speed : velocity.y;
        velocity.y = IsKeyDown(KEY_A) ? velocity.y - speed : velocity.y;
        velocity.z = IsKeyDown(KEY_SPACE) ? velocity.z + speed : velocity.z;
        velocity.z =
            IsKeyDown(KEY_LEFT_CONTROL) ? velocity.z - speed : velocity.z;

        UpdateCameraPro(&cam, velocity, rotation, 0.0f);
      }

      Vector3 forward =
          Vector3Normalize(Vector3Subtract(cam.target, cam.position));
      Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
      Vector3 up = Vector3CrossProduct(right, forward);

      rlEnableShader(compProgram);
      float resolution[4] = {(float)GetScreenWidth(), (float)GetScreenHeight(),
                             0.0f, 0.0f};
      float time = (float)GetTime();
      float fovTan = tanf((cam.fovy * 0.5f) * DEG2RAD);

      rlSetUniform(resolutionLoc, resolution, SHADER_UNIFORM_VEC4, 1);
      rlSetUniform(timeLoc, &time, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(camPosLoc, &cam.position, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(forwardLoc, &forward, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(rightLoc, &right, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(upLoc, &up, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(activeSDFLoc, &gui.currSDF, SHADER_UNIFORM_INT, 1);
      rlSetUniform(activeLightingLoc, &gui.currLight, SHADER_UNIFORM_INT, 1);
      rlSetUniform(minDistLoc, &minDist, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(scalarDistLoc, &scalarDist, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(fovLoc, &fovTan, SHADER_UNIFORM_FLOAT, 1);
      rlSetUniform(lampStrLoc, &lampStr, SHADER_UNIFORM_FLOAT, 1);

      rlBindImageTexture(mainTex.texture.id, 0,
                         PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);
      rlComputeShaderDispatch(GetScreenWidth() / 8, GetScreenHeight() / 8, 1);
      rlDisableShader();

      BeginDrawing();
      rlImGuiBegin();
      ImGui::DockSpaceOverViewport(0, NULL,
                                   ImGuiDockNodeFlags_PassthruCentralNode);
      ImGui::Begin("main");
      rlImGuiImageRenderTextureFit(&mainTex, true);
      ImGui::End();

      gui.General();
      gui.CamSettings(&cam, &sens, &lampStr, &scalarDist, &minDist);
      rlImGuiEnd();
      EndDrawing();
    }
    UnloadRenderTexture(mainTex);
    rlUnloadShader(compShaderId);
    rlUnloadShaderProgram(compProgram);
  }
  CloseWindow();
  return 0;
}
