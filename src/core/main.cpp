#include "math.h"
#include "raylib.h"
#include "rlgl.h"

#include "imgui.h"
#include "rlImGui.h"

#include "gui.h"
#include "movement.h"
#include "shaderLoad.h"

using namespace std;

int main() {
  InitWindow(1920, 1080, "SDF Engine");
  SetTargetFPS(200);

  rlImGuiSetup(true);

  shader sh;
  player cam;

  // I need to find a way to get rid of this.
  float minDist = 0.001f;
  float scalarDist = 1.0f;
  bool isAOActive = true;

  const char
      *sdfList[] = {"Gyroid Torus",   "Sphere",        "Plane",     "Cross",
                    "Weird Triangle", "Twisty Sphere", "Danes SDF", "SDF 1",
                    "SDF 2",          "SDF 3",         "SDF 4",     "SDF 5",
                    "SDF 6",          "AABB",          "NoiseSDF",  "orbitSDF",
                    "hunterSDF"}; // This array is for the SDF drop down menu.
  int sdfCount =
      sizeof(sdfList) / sizeof(sdfList[0]); // This returns the total number
                                            // of entries in the array.
  int currentSDF = 15;

  const char *lightingNames[] = {
      "Flat",     "Rim-Lighting",  "Lambertian",    "Fog",
      "Anti-Fog", "Dane Lighting", "Purple Stripes"}; // This array is for the
                                                      // lighting model drop
                                                      // down menu.
  int lightingCount =
      sizeof(lightingNames) /
      sizeof(lightingNames[0]); // This determins the total number of entries in
                                // the array.
  int currentLight = 2;

  sh.load(); // Loading in the locations of all the needed variables from
             // the glsl files.

  cameraInitialize(cam, 0.0f, 0.0f,
                   -5.0f); // Initializes the camera at the center of the scene.
  RenderTexture2D target =
      LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  int pixels = GetScreenWidth() * GetScreenHeight();
  unsigned int hitBufferId =
      rlLoadShaderBuffer(pixels * 16, NULL, RL_DYNAMIC_COPY);

  while (!WindowShouldClose()) {
    rlEnableShader(sh.computeProgramId);

    // Camera controls.
    cameraMouse(cam);
    cameraMove(cam);
    sh.setShader(cam, currentSDF, currentLight, scalarDist, minDist);
    sh.hotReload();

    rlBindImageTexture(target.texture.id, 0, target.texture.format, false);

    rlBindShaderBuffer(hitBufferId,
                       0); // layout(std430, binding = 0) HitBufferRW
    rlBindShaderBuffer(hitBufferId, 1); // layout(std430, binding = 1) HitBuffer

    int groupsX = ceil(GetScreenWidth() / 8.0f);
    int groupsY = ceil(GetScreenHeight() / 8.0f);
    rlComputeShaderDispatch(groupsX, groupsY, 1);

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

    sdfSelection(sdfList, currentSDF, sdfCount);
    lightingSelection(lightingNames, currentLight, lightingCount);
    scalarDistSlider(scalarDist);
    minDistSlider(minDist);
    aoToggle(isAOActive);

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
  rlUnloadShaderBuffer(hitBufferId);
  rlImGuiShutdown();
  CloseWindow();
  return 0;
}
