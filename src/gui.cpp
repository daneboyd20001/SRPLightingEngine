#include "gui.h"
#include "imgui.h"

void resetButton(player &cam) {
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::Button("Reset camera")) {
    cameraInitialize(cam, 0.0f, 0.0f, -5.0f);
  }
}

void sdfSelection(const char *sdfNames[], int &currentSDF, int sdfCount) {
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("Active Shape", sdfNames[currentSDF])) {
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

    for (int n = 0; n < sdfCount; n++) {
      bool is_selected = (currentSDF == n);
      if (ImGui::Selectable(sdfNames[n], is_selected)) {
        currentSDF = n;
      }
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::PopStyleVar();
    ImGui::EndCombo();
  }
}

void lightingSelection(const char *lightingNames[], int &currentLight,
                       int lightingCount) {
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("Lighting Model", lightingNames[currentLight])) {
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    for (int n = 0; n < lightingCount; n++) {
      bool is_selected = (currentLight == n);
      if (ImGui::Selectable(lightingNames[n], is_selected)) {
        currentLight = n;
      }
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::PopStyleVar();
    ImGui::EndCombo();
  }
}

void scalarDistSlider(float &scalarDist) {
  ImGui::SetNextItemWidth(150.0f);
  ImGui::SliderFloat("Scalar Distance", &scalarDist, 0.1f, 1.0f);
}

void minDistSlider(float &minDist) {
  ImGui::SetNextItemWidth(150.0f);
  ImGui::SliderFloat("Min Distance", &minDist, 0.0001f, 0.1f);
}

void lampStrSlider(player &cam) {
  ImGui::SetNextItemWidth(150.0f);
  ImGui::SliderFloat("Lamp Strength", &cam.lampStr, 0.1f, 1000.0f);
}

void fovSlider(player &cam) {
  ImGui::SetNextItemWidth(150.0f);
  ImGui::SliderFloat("FOV", &cam.fov, 0.1f, 2.0f);
}

void sensSlider(player &cam) {
  ImGui::SetNextItemWidth(150.0f);
  ImGui::SliderFloat("Sensitivity", &cam.sens, 0.0001f, 0.01f);
}
