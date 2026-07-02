#include <bits/stdc++.h>
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlImGui.h>
#include <vector>

class gui {
private:
  std::vector<std::string> sdfNames = read("../include/names/sdf-names.txt");
  std::vector<std::string> lightNames =
      read("../include/names/shader-names.txt");

public:
  gui(bool isDark) {
    rlImGuiSetup(isDark);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  };
  ~gui() { rlImGuiShutdown(); };

  int currSDF = 0;
  int currLight = 0;

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

  void General() {
    ImGui::Begin("Settings");

    ImGui::Text("FPS: %i", GetFPS());
    ImGui::Text("FrameTime: %f", GetFrameTime());

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

  void CamSettings(Camera3D *cam, float *sens, float *lampStr,
                   float *scalarDist, float *minDist) {
    Vector3 pos = cam->position;
    ImGui::Begin("Camera");

    if (ImGui::Button("Reset camera")) {
      cam->position = ((Vector3){0.0f, 0.0f, -5.0f});
    }

    ImGui::Text("Position : ");
    ImGui::Text("x: %.2f, y: %.2f, z: %.2f", pos.x, pos.y, pos.z);

    ImGui::SliderFloat("FOV", &cam->fovy, 10.0f, 120.0f);
    ImGui::SliderFloat("Sensitivity", sens, 0.01f, 0.5f);
    ImGui::SliderFloat("Lamp Strength", lampStr, 0.1f, 1000.0f);
    ImGui::SliderFloat("Scalar Distance", scalarDist, 0.1f, 1.0f);
    ImGui::SliderFloat("Min Distance", minDist, 0.0001f, 0.1f);

    ImGui::End();
  }
};
