#include "math.h"

#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "rlgl.h"

#include "imgui.h"

#include "gui.h"
#include "movement.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::string ReadFile(const std::string &filepath);
unsigned int loadCompute(std::string shaderCode);
std::string TrimString(const std::string &str);
std::vector<std::string> LoadList(const std::string &filepath);

int main() {
  InitWindow(1920, 1080, "SDF Engine");
  SetTargetFPS(200);

  rlImGuiSetup(true);

  std::vector<std::string> sdfStrings = LoadList("../include/SDFList.txt");
  std::vector<std::string> lightingStrings =
      LoadList("../include/lightingList.txt");

  std::vector<const char *> sdfList;
  for (const auto &str : sdfStrings)
    sdfList.push_back(str.c_str());

  int sdfCount = (int)sdfList.size();
  int defaultSDF = 0;

  std::vector<const char *> lightingNames;
  for (const auto &str : lightingStrings)
    lightingNames.push_back(str.c_str());

  int lightingCount = (int)lightingNames.size();
  int defaultLight = 0;

  player cam;
  cameraInitialize(cam, 0.0f, 0.0f,
                   -5.0f); // Initializes the camera at the center of the scene.

  std::string shaderCode = "#version 430 core\n" +
                           ReadFile("../shaders/noise.glsl") + "\n" +
                           ReadFile("../shaders/sdfs.glsl") + "\n" +
                           ReadFile("../shaders/lighting.glsl") + "\n" +
                           ReadFile("../shaders/raymarcher.glsl");

  unsigned int computeID = loadCompute(shaderCode);

  RenderTexture2D target =
      LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  int camToWorldLoc = rlGetLocationUniform(computeID, "CamToWorld");
  int screenSizeLoc = rlGetLocationUniform(computeID, "ScreenSize");
  int timeLoc = rlGetLocationUniform(computeID, "time");
  int activeSDFLoc = rlGetLocationUniform(computeID, "activeSDF");
  int activeLightingLoc = rlGetLocationUniform(computeID, "activeLighting");
  int lampDistLoc = rlGetLocationUniform(computeID, "lampDist");
  int lampStrengthLoc = rlGetLocationUniform(computeID, "lampStrength");
  int fovLoc = rlGetLocationUniform(computeID, "FOV_Tan");
  int scalarDistLoc = rlGetLocationUniform(computeID, "scalarDist");
  int minDistLoc = rlGetLocationUniform(computeID, "minDist");

  float scalarDist = 0.5f;
  float minDist = 0.001f;

  while (!WindowShouldClose()) {
    rlEnableShader(computeID);

    // Camera controls.
    cameraMouse(cam);
    cameraMove(cam);
    rlEnableShader(computeID);

    float screen[4] = {(float)GetScreenWidth(), (float)GetScreenHeight(),
                       (float)GetScreenWidth() / (float)GetScreenHeight(),
                       1.0f};
    rlSetUniform(screenSizeLoc, screen, RL_SHADER_UNIFORM_VEC4, 1);

    Matrix camMatrix = QuaternionToMatrix(cam.rotation);
    rlSetUniformMatrix(camToWorldLoc, camMatrix);
    float time = (float)GetTime();
    rlSetUniform(timeLoc, &time, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(fovLoc, &cam.fov, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(lampDistLoc, &cam.lampDist, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(lampStrengthLoc, &cam.lampStr, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(activeSDFLoc, &defaultSDF, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(activeLightingLoc, &defaultLight, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(scalarDistLoc, &scalarDist, RL_SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(minDistLoc, &minDist, RL_SHADER_UNIFORM_FLOAT, 1);

    if (IsKeyPressed(KEY_R) && IsKeyPressed(KEY_LEFT_SHIFT)) {
      rlUnloadShaderProgram(computeID);
      std::string shaderCode = "#version 430 core\n" +
                               ReadFile("../shaders/noise.glsl") + "\n" +
                               ReadFile("../shaders/sdfs.glsl") + "\n" +
                               ReadFile("../shaders/lighting.glsl") + "\n" +
                               ReadFile("../shaders/raymarcher.glsl");

      unsigned int computeID = loadCompute(shaderCode);

      RenderTexture2D target =
          LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

      int camPosLoc = rlGetLocationUniform(computeID, "camPos");
      int camToWorldLoc = rlGetLocationUniform(computeID, "CamToWorld");
      int screenSizeLoc = rlGetLocationUniform(computeID, "ScreenSize");
      int timeLoc = rlGetLocationUniform(computeID, "time");
      int activeSDFLoc = rlGetLocationUniform(computeID, "activeSDF");
      int activeLightingLoc = rlGetLocationUniform(computeID, "activeLighting");
      int lampDistLoc = rlGetLocationUniform(computeID, "lampDist");
      int lampStrengthLoc = rlGetLocationUniform(computeID, "lampStrength");
      int fovLoc = rlGetLocationUniform(computeID, "fov");
      int scalarDistLoc = rlGetLocationUniform(computeID, "scalarDist");
      int minDistLoc = rlGetLocationUniform(computeID, "minDist");
    }

    rlBindImageTexture(target.texture.id, 0, target.texture.format, false);

    rlComputeShaderDispatch(ceil(GetScreenWidth() / 8.0f),
                            ceil(GetScreenHeight() / 8.0f), 1);

    rlDisableShader();

    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle sourceRec = {0.0f, 0.0f, (float)target.texture.width,
                           -(float)target.texture.height};
    DrawTextureRec(target.texture, sourceRec, Vector2{0, 0}, WHITE);

    // FPS and frametime.
    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("Frametime: %f", GetFrameTime()), 10, 30, 20,
             DARKGREEN);

    rlImGuiBegin(); // Gui that is drawn on the screen. For controlling of the
                    // camera, shaders, SDFs, and other things.

    ImGui::SetNextWindowPos({70, 50}, ImGuiCond_Once);
    ImGui::Begin("Controls");

    sdfSelection(sdfList.data(), defaultSDF, sdfCount);
    lightingSelection(lightingNames.data(), defaultLight, lightingCount);
    scalarDistSlider(scalarDist);
    minDistSlider(minDist);

    ImGui::End();

    ImGui::SetNextWindowPos({70, 250}, ImGuiCond_Once);
    ImGui::Begin("Camera Controls");

    resetButton(cam);
    fovSlider(cam);
    sensSlider(cam);
    lampStrSlider(cam);

    ImGui::End();
    rlImGuiEnd();

    EndDrawing();
  }

  rlDisableShader();
  rlImGuiShutdown();
  CloseWindow();
  return 0;
}

std::string ReadFile(const std::string &filepath) {
  std::ifstream file(filepath);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

unsigned int loadCompute(std::string shaderCode) {
  Shader sh;
  sh.id = rlCompileShader(shaderCode.c_str(), RL_COMPUTE_SHADER);
  unsigned int computeID = rlLoadComputeShaderProgram(sh.id);
  return computeID;
}

std::string TrimString(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, (last - first + 1));
}

std::vector<std::string> LoadList(const std::string &filepath) {
  std::vector<std::string> items;
  std::ifstream file(filepath);

  if (!file.is_open()) {
    TraceLog(LOG_WARNING, "Failed to open list file: %s", filepath.c_str());
    return items;
  }

  std::string token;
  while (std::getline(file, token, ',')) {
    std::string cleaned = TrimString(token);
    if (!cleaned.empty())
      items.push_back(cleaned);
  }

  return items;
}
