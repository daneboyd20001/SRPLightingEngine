#pragma once

#include <string>

// Application defaults. Edit these until the TUI provides runtime controls.
struct Settings {
  int width = 1280;
  int height = 720;
  bool vsync = true;
  bool validation = true;

  std::string profileCsv; // Empty disables logging; e.g. "profile.csv".
  bool profileSummary = true;

  int sdf = 0;      // 0..16, ordered as in assets/names/sdf-names.txt
  int lighting = 0; // 0..4
  float lampStrength = 100.0f;
  float quality = 0.5f;
  float minClip = 0.0f;
  float maxClip = 100.0f;
};
