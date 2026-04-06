#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "imgui.h"
#include "rlImGui.h"
#include <string>

#include <fstream>
#include <sstream>

std::string ReadFile(const std::string &filepath) {
  std::ifstream file(filepath);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main() {
  InitWindow(1920, 1080, "SDF Engine");
  SetTargetFPS(200);

  rlImGuiSetup(true);

  const char *sdfNames[] = {
      "Gyroid Torus",   "Sphere",        "Plane",     "Cross",
      "Weird Triangle", "Twisty Sphere", "Danes SDF", "SDF 4",
      "SDF 5",          "SDF 6",         "AABB"};
  int sdfCount = 11;
  int currentSDFIndex = 0;

  const char *lightingNames[] = {"Oren-Nayar", "Lambertian"};
  int lightingCount = 2;
  int currentLightingIndex = 0;

  std::string sdfPath = "../src/sdfs.glsl";
  std::string lightingPath = "../src/lighting.glsl";
  std::string mainPath = "../src/raymarcher.glsl";
  std::string versionHeader = "#version 330 core\n";

  std::string combinedShaderCode = versionHeader + ReadFile(sdfPath) + "\n" +
                                   ReadFile(lightingPath) + "\n" +
                                   ReadFile(mainPath);
  Shader sdfShader = LoadShaderFromMemory(0, combinedShaderCode.c_str());

  int resolutionLoc = GetShaderLocation(sdfShader, "resolution");
  int timeLoc = GetShaderLocation(sdfShader, "time");
  int camPosLoc = GetShaderLocation(sdfShader, "camPos");
  int forwardLoc = GetShaderLocation(sdfShader, "camForward");
  int rightLoc = GetShaderLocation(sdfShader, "camRight");
  int upLoc = GetShaderLocation(sdfShader, "camUp");
  int activeSDFLoc = GetShaderLocation(sdfShader, "activeSDF");
  int activeLightingLoc = GetShaderLocation(sdfShader, "activeLighting");
  int lanternRadiusLoc = GetShaderLocation(sdfShader, "lanternRadius");

  int noiseTexLoc = GetShaderLocation(sdfShader, "noiseTex");

  float lanternRadius = 25.0f;

  Image noiseImg = GenImagePerlinNoise(512, 512, 0, 0, 4.0f);
  Texture2D noiseTex = LoadTextureFromImage(noiseImg);
  UnloadImage(noiseImg);

  Vector3 camPos = {0.0f, 0.0f, -5.0f};
  float yaw = PI / 2.0f;
  float pitch = 0.0f;

  long sdfModTime = GetFileModTime(sdfPath.c_str());
  long lightingModTime = GetFileModTime(lightingPath.c_str());
  long mainModTime = GetFileModTime(mainPath.c_str());

  while (!WindowShouldClose()) {

    // hot-reloading
    long currentSdfModTime = GetFileModTime(sdfPath.c_str());
    long currentLightingModTime = GetFileModTime(lightingPath.c_str());
    long currentMainModTime = GetFileModTime(mainPath.c_str());

    if (currentSdfModTime != sdfModTime ||
        currentLightingModTime != lightingModTime ||
        currentMainModTime != mainModTime) {

      std::string updatedCombinedCode = ReadFile(sdfPath) + "\n" +
                                        ReadFile(lightingPath) + "\n" +
                                        ReadFile(mainPath);
      Shader updatedShader =
          LoadShaderFromMemory(0, updatedCombinedCode.c_str());

      if (updatedShader.id != rlGetShaderIdDefault()) {
        UnloadShader(sdfShader);
        sdfShader = updatedShader;

        resolutionLoc = GetShaderLocation(sdfShader, "resolution");
        timeLoc = GetShaderLocation(sdfShader, "time");
        camPosLoc = GetShaderLocation(sdfShader, "camPos");
        forwardLoc = GetShaderLocation(sdfShader, "camForward");
        rightLoc = GetShaderLocation(sdfShader, "camRight");
        upLoc = GetShaderLocation(sdfShader, "camUp");
        activeSDFLoc = GetShaderLocation(sdfShader, "activeSDF");
        lanternRadiusLoc = GetShaderLocation(sdfShader, "lanternRadius");
        activeLightingLoc = GetShaderLocation(sdfShader, "activeLighting");
        noiseTexLoc = GetShaderLocation(sdfShader, "noiseTex");
      }
      sdfModTime = currentSdfModTime;
      lightingModTime = currentLightingModTime;
      mainModTime = currentMainModTime;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
      DisableCursor();
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
      EnableCursor();

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 mouseDelta = GetMouseDelta();
      yaw += mouseDelta.x * 0.002f;
      pitch -= mouseDelta.y * 0.002f;
      pitch = Clamp(pitch, -1.5f, 1.5f);
    }

    Vector3 forward = {cosf(pitch) * cosf(yaw), sinf(pitch),
                       cosf(pitch) * sinf(yaw)};
    Vector3 right =
        Vector3Normalize(Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f}));
    Vector3 up = Vector3CrossProduct(right, forward);

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      float speed = 5.0f * GetFrameTime();
      if (IsKeyDown(KEY_LEFT_SHIFT))
        speed *= 2.5f;

      if (IsKeyDown(KEY_W))
        camPos = Vector3Add(camPos, Vector3Scale(forward, speed));
      if (IsKeyDown(KEY_S))
        camPos = Vector3Subtract(camPos, Vector3Scale(forward, speed));
      if (IsKeyDown(KEY_D))
        camPos = Vector3Add(camPos, Vector3Scale(right, speed));
      if (IsKeyDown(KEY_A))
        camPos = Vector3Subtract(camPos, Vector3Scale(right, speed));
      if (IsKeyDown(KEY_E))
        camPos = Vector3Add(camPos, Vector3Scale(up, speed));
      if (IsKeyDown(KEY_Q))
        camPos = Vector3Subtract(camPos, Vector3Scale(up, speed));
      if (IsKeyDown(KEY_SPACE))
        camPos = Vector3Add(camPos, Vector3Scale(up, speed));
      if (IsKeyDown(KEY_C))
        camPos = Vector3Subtract(camPos, Vector3Scale(up, speed));
    }

    float time = GetTime();
    float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};

    SetShaderValue(sdfShader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(sdfShader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sdfShader, camPosLoc, &camPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(sdfShader, forwardLoc, &forward, SHADER_UNIFORM_VEC3);
    SetShaderValue(sdfShader, rightLoc, &right, SHADER_UNIFORM_VEC3);
    SetShaderValue(sdfShader, upLoc, &up, SHADER_UNIFORM_VEC3);
    SetShaderValue(sdfShader, activeSDFLoc, &currentSDFIndex,
                   SHADER_UNIFORM_INT);
    SetShaderValue(sdfShader, lanternRadiusLoc, &lanternRadius,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(sdfShader, activeLightingLoc, &currentLightingIndex,
                   SHADER_UNIFORM_INT);

    SetShaderValueTexture(sdfShader, noiseTexLoc, noiseTex);

    BeginDrawing();
    ClearBackground(BLACK);

    BeginShaderMode(sdfShader);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();

    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("Frametime: %f", GetFrameTime()), 10, 30, 20,
             DARKGREEN);

    rlImGuiBegin();
    ImGui::Begin("Controls");

    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("Active Shape", sdfNames[currentSDFIndex])) {
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.5f, 0.5f));

      for (int n = 0; n < sdfCount; n++) {
        bool is_selected = (currentSDFIndex == n);
        if (ImGui::Selectable(sdfNames[n], is_selected)) {
          currentSDFIndex = n;
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::PopStyleVar();
      ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("Lighting Model",
                          lightingNames[currentLightingIndex])) {
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.5f, 0.5f));
      for (int n = 0; n < lightingCount; n++) {
        bool is_selected = (currentLightingIndex == n);
        if (ImGui::Selectable(lightingNames[n], is_selected)) {
          currentLightingIndex = n;
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::PopStyleVar();
      ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("Light Distance", &lanternRadius, 0.0f, 200.0f);

    ImGui::End();
    rlImGuiEnd();

    EndDrawing();
  }

  UnloadTexture(noiseTex);
  rlImGuiShutdown();
  CloseWindow();
  return 0;
}
