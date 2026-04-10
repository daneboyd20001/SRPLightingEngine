#pragma once

#include "movement.h"

void resetButton(player &cam);
void sdfSelection(const char *sdfNames[], int &currentSDF, int sdfCount);
void lightingSelection(const char *lightingNames[], int &currentLight,
                       int lightingCount);
void scalarDistSlider(float &scalarDist);
void minDistSlider(float &minDist);
void lampStrSlider(player &cam);
void fovSlider(player &cam);
void sensSlider(player &cam);
