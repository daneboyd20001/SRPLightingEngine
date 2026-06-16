#pragma once

#include "Controller.hpp"
#include <imgui.h>
#include <vector>

#define WIDTH 150.0f

class GUI {
public:
  GUI() = default;
  ~GUI() = default;

  void resetButton(Controller &cam) {
    ImGui::SetNextItemWidth(WIDTH);
    if (ImGui::Button("Reset camera")) {
      cam.Init(0.0f, 0.0f, -5.0f);
    }
  }

  void sdfSelection(std::vector<std::string> sdfNames, int &currentSDF,
                    int sdfCount) {
    ImGui::SetNextItemWidth(WIDTH);
    if (ImGui::BeginCombo("Active Shape", sdfNames[currentSDF].c_str())) {
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.5f, 0.5f));

      for (int n = 0; n < sdfCount; n++) {
        bool is_selected = (currentSDF == n);
        if (ImGui::Selectable(sdfNames[n].c_str(), is_selected)) {
          currentSDF = n;
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::PopStyleVar();
      ImGui::EndCombo();
    }
  }

  void lightingSelection(std::vector<std::string> lightingNames,
                         int &currentLight, int lightingCount) {
    ImGui::SetNextItemWidth(WIDTH);
    if (ImGui::BeginCombo("Lighting Model",
                          lightingNames[currentLight].c_str())) {
      ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,
                          ImVec2(0.5f, 0.5f));
      for (int n = 0; n < lightingCount; n++) {
        bool is_selected = (currentLight == n);
        if (ImGui::Selectable(lightingNames[n].c_str(), is_selected)) {
          currentLight = n;
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::PopStyleVar();
      ImGui::EndCombo();
    }
  }

  void scalarDistSlider(Controller &cam) {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::SliderFloat("Scalar Distance", &cam.scalarDist, 0.1f, 1.0f);
  }

  void minDistSlider(Controller &cam) {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::SliderFloat("Min Distance", &cam.minDist, 0.0001f, 0.1f);
  }

  void lampStrSlider(Controller &cam) {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::SliderFloat("Lamp Strength", &cam.lampStr, 0.1f, 1000.0f);
  }

  void fovSlider(Controller &cam) {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::SliderFloat("FOV", &cam.fov, 0.1f, 2.0f);
  }

  void sensSlider(Controller &cam) {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::SliderFloat("Sensitivity", &cam.sens, 0.0001f, 0.01f);
  }

  void camPos(Controller cam) {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::Text("x: %f, y: %f, z: %f", cam.pos.x, cam.pos.y, cam.pos.z);
  }

  void showFPS() {
    ImGui::SetNextItemWidth(WIDTH);
    ImGui::Text("FPS: %i", GetFPS());
    ImGui::Text("FrameTime: %f", GetFrameTime());
  }
};
