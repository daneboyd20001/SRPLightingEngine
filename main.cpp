#include <raylib.h>
#include <rlgl.h>

#include <imgui.h>
#include <rlImGui.h>

#include <fstream>
#include <iostream>
#include <string>

#include "header/Controller.hpp"
#include "header/GUI.hpp"
#include "header/ShaderLogic.hpp"

using namespace std;
using namespace ImGui;

int main() {
  InitWindow(1920, 1080, "SDF Engine");
  SetTargetFPS(200);

  rlImGuiSetup(true);

  ShaderLogic shaderLogic;
  Controller cam;
  GUI gui;

  // TODO : Change this into maybe a .txt file of names?
  const char *sdfNames[] = {
      "Gyroid Torus",  "Sphere",    "Plane", "Cross", "Weird Triangle",
      "Twisty Sphere", "Danes SDF", "SDF 1", "SDF 2", "SDF 3",
      "SDF 4",         "SDF 5",     "SDF 6", "AABB",  "NoiseSDF",
      "orbitSDF",      "hunterSDF"};
  int sdfCount = sizeof(sdfNames) / sizeof(sdfNames[0]);
  int currentSDF = 0;

  const char *lightingNames[] = {"Flat", "Rim-Lighting", "Lambertian",
                                 "Fog",  "Anti-Fog",     "Dane Lighting"};
  int lightingCount = sizeof(lightingNames) / sizeof(lightingNames[0]);
  int currentLight = 0;

  string comCode = "#version 430 core\n" +
                   shaderLogic.ReadFile("../shader/noise.glsl") + "\n" +
                   shaderLogic.ReadFile("../shader/sdfs.glsl") + "\n" +
                   shaderLogic.ReadFile("../shader/lighting.glsl") + "\n" +
                   shaderLogic.ReadFile("../shader/raymarcher.glsl");

  // TODO : Hot reloader
  ofstream shader("../shader/shader-code.glsl");
  shader << comCode;
  shader.close();

  char *sdfCode = LoadFileText("../shader/shader-code.glsl");
  shaderLogic.computeShader = rlLoadShader(sdfCode, RL_COMPUTE_SHADER);
  shaderLogic.computeProgram =
      rlLoadShaderProgramCompute(shaderLogic.computeShader);
  UnloadFileText(sdfCode);

  RenderTexture2D target =
      LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  shaderLogic.load();

  cam.Init(0.0f, 0.0f, -5.0f);

  while (!WindowShouldClose()) {

    shaderLogic.setShader(cam, currentSDF, currentLight);

    cam.UpdateMouse();
    cam.UpdateMove();

    rlEnableShader(shaderLogic.computeProgram);
    rlBindImageTexture(target.texture.id, 0, target.texture.format, false);
    rlComputeShaderDispatch(GetScreenWidth() / 16, GetScreenHeight() / 16, 1);
    rlDisableShader();

    BeginDrawing();
    ClearBackground(BLACK);

    DrawTexture(target.texture, 0, 0, WHITE);

    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("Frametime: %f", GetFrameTime()), 10, 30, 20,
             DARKGREEN);

    rlImGuiBegin();
    SetNextWindowPos({70, 50}, ImGuiCond_Once);
    Begin("Controls");

    gui.sdfSelection(sdfNames, currentSDF, sdfCount);
    gui.lightingSelection(lightingNames, currentLight, lightingCount);

    End();

    SetNextWindowPos({70, 250}, ImGuiCond_Once);
    Begin("Camera Controls");

    gui.resetButton(cam);
    gui.fovSlider(cam);
    gui.sensSlider(cam);
    gui.lampStrSlider(cam);
    gui.scalarDistSlider(cam);
    gui.minDistSlider(cam);

    End();
    rlImGuiEnd();

    EndDrawing();
  }

  rlUnloadShader(shaderLogic.computeShader);
  rlUnloadShader(shaderLogic.computeProgram);
  UnloadTexture(target.texture);
  rlImGuiShutdown();
  CloseWindow();
  return 0;
}
