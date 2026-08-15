#include <Window.hpp>

#include <Camera.hpp>
#include <Gui.hpp>
#include <Shader.hpp>

int main() {
  Window main;
  Shader shader("./shader/shader-code.glsl");
  Camera camera;
  Gui gui(main.window);

  float lastTime{};
  while (!glfwWindowShouldClose(main.window)) {
    float now = glfwGetTime();
    float deltaTime = now - lastTime;
    lastTime = now;

    int activeSDF = gui.currSDF, activeLighting = gui.currLight;
    float fovTan = tan(camera.fov * 0.5f * DEG2RAD);

    camera.Controlles(main.window, deltaTime);

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gui.Start();

    ImGui::Begin("Compute");

    ImVec2 scene = ImGui::GetContentRegionAvail();
    int newW = (int)scene.x;
    int newH = (int)scene.y;

    float time = (float)glfwGetTime();
    shader.resizeCompute(newW, newH); // Resize compute tex
    shader.use(camera, time, fovTan, activeSDF,
               activeLighting); // Compute Shader

    ImGui::Image((ImTextureID)(intptr_t)shader.screenTex,
                 ImVec2((float)shader.texWidth, (float)shader.texHeight));
    ImGui::End();

    gui.General(deltaTime);
    gui.CamSettings(camera.pos, camera.fov, camera.sense, shader.lampStr,
                    shader.scalarDist, shader.minDist);

    gui.End();

    glfwSwapBuffers(main.window);
    glfwPollEvents();
  }
  return 0;
}
