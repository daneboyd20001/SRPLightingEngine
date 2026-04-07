#include "raylib.h"
#include "rlgl.h"

#include "imgui.h"
#include "rlImGui.h"

#include <string>

#include "ImGui.h"
#include "movement.h"
#include "shaderLoad.h"

int main() {
  InitWindow(1920, 1080, "SDF Engine");
  SetTargetFPS(200);

  rlImGuiSetup(true);

  shader sh;
  player cam;

  float minDist = 0.001f;
  float scalarDist = 1.0f;

  const char *sdfNames[] = {"Gyroid Torus", "Sphere",         "Plane",
                            "Cross",        "Weird Triangle", "Twisty Sphere",
                            "Danes SDF",    "SDF 4",          "SDF 5",
                            "SDF 6",        "AABB",           "NoiseSDF"};
  int sdfCount = sizeof(sdfNames) / sizeof(sdfNames[0]);
  int currentSDF = 0;

  const char *lightingNames[] = {"Flat", "Rim-Lighting", "Lambertian", "Fog",
                                 "Anti-Fog"};
  int lightingCount = sizeof(lightingNames) / sizeof(lightingNames[0]);
  int currentLight = 0;

  std::string sdfPath = "../src/sdfs.glsl";
  std::string lightingPath = "../src/lighting.glsl";
  std::string mainPath = "../src/raymarcher.glsl";
  std::string versionHeader = "#version 330 core\n";

  std::string combinedShaderCode = versionHeader + ReadFile(sdfPath) + "\n" +
                                   ReadFile(lightingPath) + "\n" +
                                   ReadFile(mainPath);
  sh.sdfShader = LoadShaderFromMemory(0, combinedShaderCode.c_str());

  load(sh); // Loading in the locations of all the needed variables from the
            // glsl files.

  int noiseTexLoc = GetShaderLocation(sh.sdfShader, "noiseTex");

  cameraInitialize(cam, 0.0f, 0.0f, -5.0f, 2.0f, 0.0f);

  Image noiseImg = GenImagePerlinNoise(512, 512, 0, 0, 4.0f);
  Texture2D noiseTex = LoadTextureFromImage(noiseImg);
  UnloadImage(noiseImg);

  long sdfModTime = GetFileModTime(sdfPath.c_str());
  long lightingModTime = GetFileModTime(lightingPath.c_str());
  long mainModTime = GetFileModTime(mainPath.c_str());

  while (!WindowShouldClose()) {

    hotReload(sdfModTime, lightingModTime, mainModTime, sdfPath, lightingPath,
              mainPath, sh);

    SetShaderValueTexture(sh.sdfShader, noiseTexLoc, noiseTex);

    BeginDrawing();
    ClearBackground(BLACK);

    setShader(sh, cam, minDist, scalarDist, currentSDF, currentLight);

    // Camera controls.
    cameraMouse(cam);
    cameraMove(cam);

    BeginShaderMode(sh.sdfShader);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();

    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("Frametime: %f", GetFrameTime()), 10, 30, 20,
             DARKGREEN);

    /*
     * Gui that is drawn on the screen. For controlling of the camera, shaders,
     * SDFs, and other things.
     */
    rlImGuiBegin();
    ImGui::Begin("Controls");

    resetButton(cam);
    sdfSelection(sdfNames, currentSDF, sdfCount);
    lightingSelection(lightingNames, currentLight, lightingCount);
    scalarDistSlider(scalarDist);
    minDistSlider(minDist);
    lampDistSlider(cam);
    lampStrSlider(cam);
    fovSlider(cam);

    ImGui::End();
    rlImGuiEnd();

    EndDrawing();
  }

  rlImGuiShutdown();
  CloseWindow();
  return 0;
}
