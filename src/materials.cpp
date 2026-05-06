/*
 * Copyright (c) 2010  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "materials.h"
#include "config.h"

#include <vtkActor.h>
#include <vtkJPEGReader.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkTexture.h>

static void
phong_lighting (vtkProperty *p)
{
  p->SetInterpolationToPhong ();
  p->LightingOn ();
}

void
prism_apply_material (vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty ();
  phong_lighting (prop);
  /* glass.xml.in Property (shaders not ported; use Phong + sky texture) */
  prop->SetOpacity (0.99);
  prop->SetAmbient (0.0);
  prop->SetAmbientColor (0.8, 0.95, 1.0);
  prop->SetDiffuse (0.0);
  prop->SetDiffuseColor (0.0, 1.0, 0.2);
  prop->SetSpecular (0.05);
  prop->SetSpecularColor (0.6, 0.4, 1.0);
  prop->SetSpecularPower (9.0);
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOff ();

  vtkNew<vtkJPEGReader> sky;
  sky->SetFileName (PACKAGE_DATADIR "/textures/sky.jpg");
  sky->Update ();

  vtkNew<vtkTexture> tex;
  tex->SetInputConnection (sky->GetOutputPort ());
  tex->InterpolateOn ();
  actor->SetTexture (tex);
}

void
beam_apply_material (vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty ();
  phong_lighting (prop);
  /* beam.xml.in Property only (custom GLSL not wired here) */
  prop->SetOpacity (0.99);
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOff ();
  prop->SetAmbient (0.35);
  prop->SetAmbientColor (0.85, 0.92, 1.0);
  prop->SetDiffuse (0.65);
  prop->SetDiffuseColor (0.55, 0.78, 1.0);
  prop->SetSpecular (0.35);
  prop->SetSpecularColor (1.0, 1.0, 1.0);
  prop->SetSpecularPower (32.0);
}

void
torch_apply_actor_material (vtkProperty *prop)
{
  phong_lighting (prop);
  /* torch.xml.in Property */
  prop->SetOpacity (1.0);
  prop->SetAmbient (0.5);
  prop->SetAmbientColor (0.0, 0.6, 1.0);
  prop->SetDiffuse (0.25);
  prop->SetDiffuseColor (0.5, 0.8, 1.0);
  prop->SetSpecular (0.0);
  prop->SetSpecularColor (1.0, 1.0, 1.0);
  prop->SetSpecularPower (2.0);
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOff ();
}

void
torch_apply_handle_material (vtkProperty *handle, vtkProperty *selected_handle)
{
  torch_apply_actor_material (handle);
  torch_apply_actor_material (selected_handle);
}

void
torch_apply_line_material (vtkProperty *line, vtkProperty *selected_line)
{
  line->SetOpacity (0.01);
  selected_line->SetOpacity (0.01);
}

void
label_apply_material (vtkProperty *prop)
{
  phong_lighting (prop);
  /* text.xml.in */
  prop->SetOpacity (1.0);
  prop->SetAmbient (1.0);
  prop->SetAmbientColor (1.0, 1.0, 1.0);
  prop->SetDiffuse (0.0);
  prop->SetDiffuseColor (0.5, 0.8, 1.0);
  prop->SetSpecular (0.0);
  prop->SetSpecularColor (1.0, 1.0, 1.0);
  prop->SetSpecularPower (2.0);
}

void
ground_apply_material (vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty ();
  phong_lighting (prop);
  /* ground.xml.in Property */
  prop->SetOpacity (1.0);
  prop->SetAmbient (0.0);
  prop->SetAmbientColor (0.0, 0.6, 1.0);
  prop->SetDiffuse (1.0);
  prop->SetDiffuseColor (1.0, 1.0, 1.0);
  prop->SetSpecular (0.0);
  prop->SetSpecularColor (1.0, 1.0, 1.0);
  prop->SetSpecularPower (3.0);
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOn ();

  vtkNew<vtkJPEGReader> ground;
  ground->SetFileName (PACKAGE_DATADIR "/textures/ground.jpg");
  ground->Update ();

  vtkNew<vtkTexture> tex;
  tex->SetInputConnection (ground->GetOutputPort ());
  tex->InterpolateOn ();
  actor->SetTexture (tex);
}
