#include <raylib.h>
#include <rlgl.h>

#include <imgui.h>
#include <rlImGui.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Controller.hpp"
#include "GUI.hpp"
#include "ShaderLogic.hpp"

using namespace std;
using namespace ImGui;

vector<string> DropBoxNames(string filename) {
  vector<string> names;
  ifstream f(filename);
  if (!f.is_open())
    cerr << "Error opening " << filename << endl;

  string s;
  while (getline(f, s)) {
    names.push_back(s);
  }
  f.close();
  return names;
}

int main() {
  InitWindow(1920, 1080, "SDF Engine");

  rlImGuiSetup(true);

  ShaderLogic shaderLogic;
  Controller cam;
  GUI gui;

  // TODO : Hot reloader
  vector<string> sdfNames = DropBoxNames("../shader/sdf-names.txt");
  int sdfCount = sdfNames.size();
  int currentSDF = 0;

  vector<string> lightingNames = DropBoxNames("../shader/shader-names.txt");
  int lightingCount = lightingNames.size();
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

  Texture2D targetFloat = {0};
  targetFloat.id = rlLoadTexture(NULL, GetScreenWidth(), GetScreenHeight(),
                                 PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
  targetFloat.width = GetScreenWidth();
  targetFloat.height = GetScreenHeight();
  targetFloat.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
  targetFloat.mipmaps = 1;

  shaderLogic.load();

  cam.Init(0.0f, 0.0f, -5.0f);

  while (!WindowShouldClose()) {
    cam.UpdateMouse();
    cam.UpdateMove();

    rlEnableShader(shaderLogic.computeProgram);
    shaderLogic.setShader(cam, currentSDF, currentLight);
    rlBindImageTexture(targetFloat.id, 0, targetFloat.format, false);
    rlComputeShaderDispatch(GetScreenWidth() / 8, GetScreenHeight() / 8, 1);
    rlDisableShader();

    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawTexture(targetFloat, 0, 0, WHITE);

    rlImGuiBegin();
    SetNextWindowPos({70, 50}, ImGuiCond_Once);
    Begin("Settings");

    gui.showFPS();
    gui.sdfSelection(sdfNames, currentSDF, sdfCount);
    gui.lightingSelection(lightingNames, currentLight, lightingCount);

    End();

    SetNextWindowPos({70, 175}, ImGuiCond_Once);
    Begin("Camera");

    gui.resetButton(cam);
    gui.fovSlider(cam);
    gui.sensSlider(cam);
    gui.lampStrSlider(cam);
    gui.scalarDistSlider(cam);
    gui.minDistSlider(cam);
    gui.camPos(cam);

    End();
    rlImGuiEnd();

    EndDrawing();
  }

  rlUnloadShader(shaderLogic.computeShader);
  rlUnloadShader(shaderLogic.computeProgram);
  rlUnloadTexture(targetFloat.id);
  rlImGuiShutdown();
  CloseWindow();
  return 0;
}
