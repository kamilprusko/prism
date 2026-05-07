/*
 * Copyright (c) 2010  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRISM_MATERIALS_H
#define PRISM_MATERIALS_H

class vtkActor;
class vtkProperty;

void prism_apply_material (vtkActor *actor);
void beam_apply_material (vtkActor *actor);
void torch_apply_actor_material (vtkProperty *prop);
void torch_apply_handle_material (vtkProperty *handle, vtkProperty *selected_handle);
void torch_apply_line_material (vtkProperty *line, vtkProperty *selected_line);
void label_apply_material (vtkProperty *prop);
void ground_apply_material (vtkActor *actor);
/** Textured and shadow ground planes: single-sided (invisible from below). */
void ground_plane_one_sided (vtkProperty *prop);

#endif
