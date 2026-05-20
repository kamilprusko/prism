/*
 * Copyright (c) 2010,2026  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRISM_UTILS_H
#define PRISM_UTILS_H

#include <string>
#include <unordered_map>

class vtkActor;

void LoadObject(vtkActor *actor, const char *filename);
std::unordered_map<std::string, std::string> LoadShader(const char *filename);
void LoadTexture(vtkActor *actor, const char *filename);

#endif
