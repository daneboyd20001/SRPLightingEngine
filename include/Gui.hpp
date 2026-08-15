#pragma once

#include <bits/stdc++.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <Math.hpp>
#include <vector>

class Gui {
private:
  std::vector<std::string> read(std::string filename) {
    std::vector<std::string> names;
    std::ifstream f(filename);
    if (!f.is_open())
      std::cerr << "Error opening file";

    std::string s;
    while (getline(f, s, ','))
      names.push_back(s);

    return names;
  }

  std::vector<std::string> sdfNames = read("./assets/names/sdf-names.txt");
  std::vector<std::string> lightNames = read("./assets/names/shader-names.txt");

public:
  Gui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
  };
  ~Gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  };

  int currSDF = 0;
  int currLight = 0;

  void Start() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, NULL,
                                 ImGuiDockNodeFlags_PassthruCentralNode);
  }
  void End() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void General(float frameTime) {
    ImGui::Begin("Settings");

    ImGui::Text("FPS: %i", (int)(1.0f / frameTime));
    ImGui::Text("FrameTime: %.6f", frameTime);

    if (ImGui::BeginCombo("Active Shape", sdfNames[currSDF].c_str())) {
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.5f, 0.5f));

      for (int n = 0; n < sdfNames.size(); n++) {
        bool is_selected = (currSDF == n);
        if (ImGui::Selectable(sdfNames[n].c_str(), is_selected))
          currSDF = n;

        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::PopStyleVar();
      ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Lighting Model", lightNames[currLight].c_str())) {
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.5f, 0.5f));
      for (int n = 0; n < lightNames.size(); n++) {
        bool is_selected = (currLight == n);
        if (ImGui::Selectable(lightNames[n].c_str(), is_selected))
          currLight = n;

        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::PopStyleVar();
      ImGui::EndCombo();
    }

    ImGui::End();
  }

  void CamSettings(Math::vec3 &pos, float &fov, float &sens, float &lampStr,
                   float &scalarDist, float &minDist) {
    ImGui::Begin("Camera");

    if (ImGui::Button("Reset camera")) {
      pos = {0.0f, 0.0f, -5.0f};
    }

    ImGui::Text("Position : ");
    ImGui::Text("x: %.2f, y: %.2f, z: %.2f", pos.x, pos.y, pos.z);

    ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f);
    ImGui::SliderFloat("Sensitivity", &sens, 0.01f, 0.5f);
    ImGui::SliderFloat("Lamp Strength", &lampStr, 0.1f, 1000.0f);
    ImGui::SliderFloat("Scalar Distance", &scalarDist, 0.1f, 1.0f);
    ImGui::SliderFloat("Min Distance", &minDist, 0.0001f, 0.1f);

    ImGui::End();
  }
};
