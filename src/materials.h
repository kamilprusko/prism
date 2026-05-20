/*
 * Copyright (c) 2010  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRISM_MATERIALS_H
#define PRISM_MATERIALS_H

class vtkActor;
class vtkProperty;

void ApplyPrismMaterial(vtkActor *actor);
void ApplyBeamMaterial(vtkActor *actor);
void ApplyTorchMaterial(vtkProperty *prop);
void ApplyTorchHandleMaterial(vtkProperty *handle, vtkProperty *selectedHandle);
void ApplyTorchLineMaterial(vtkProperty *line, vtkProperty *selectedLine);
void ApplyLabelMaterial(vtkProperty *prop);
void ApplyGroundMaterial(vtkActor *actor);
void ApplyOneSidedMaterial(vtkProperty *prop);

#endif
