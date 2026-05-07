/*
 * Copyright (c) 2010,2026  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "materials.h"
#include "config.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

#include <vtkActor.h>
#include <vtkJPEGReader.h>
#include <vtkNew.h>
#include <vtkOpenGLShaderProperty.h>
#include <vtkProperty.h>
#include <vtkTexture.h>

static void
phong_lighting (vtkProperty *p)
{
  p->SetInterpolationToPhong ();
  // p->SetInterpolationToPBR ();  // not supported by lower-end GPUs
  p->LightingOn ();
}



// TODO: remove this
/* Fallback if glass_vtk.inl is missing from PACKAGE_DATADIR (mirrors data/shaders/glass_vtk.inl). */
static const char kGlassVtkInlFallback[] = R"(###VERTEX_POSITIONVC_DEC###
###VERTEX_POSITIONVC_IMPL###
###FRAGMENT_LIGHT_DEC###
###FRAGMENT_LIGHT_IMPL###
  {
    vec3  N    = normalize(normalVCVSOutput);
    vec3  V    = normalize(-vertexVC.xyz);
    float cosT = abs(dot(N, V));
    float silhouette = 1.0 - cosT;
    vec2 envUV = N.xy * 0.5 + 0.5;
    vec3 sky   = texture(actortexture, envUV).rgb;
    vec3 base  = mix(vec3(0.78, 0.80, 0.84), sky * 1.25, 0.45);
    vec3 edge  = vec3(0.92, 0.94, 0.98) * pow(silhouette, 3.0) * 0.45;
    vec3 col   = base + edge + specular;
    float alpha = clamp(0.38 + 0.12 * silhouette, 0.34, 0.52);
    gl_FragData[0] = vec4(col, alpha);
  }
###END###
)";

static std::unordered_map<std::string, std::string>
parse_glass_vtk_inl (const std::string &contents)
{
  std::unordered_map<std::string, std::string> out;
  std::istringstream stream (contents);
  std::string line;
  std::string cur_key;
  std::ostringstream cur_body;
  while (std::getline (stream, line))
    {
      if (line.size () >= 6 && line.compare (0, 3, "###") == 0
          && line.compare (line.size () - 3, 3, "###") == 0)
        {
          if (!cur_key.empty ())
            {
              out[cur_key] = cur_body.str ();
              cur_body.str ("");
              cur_body.clear ();
            }
          cur_key = line.substr (3, line.size () - 6);
          if (cur_key == "END")
            {
              cur_key.clear ();
            }
          continue;
        }
      cur_body << line << '\n';
    }
  if (!cur_key.empty ())
    {
      out[cur_key] = cur_body.str ();
    }
  return out;
}

static std::unordered_map<std::string, std::string>
load_glass_vtk_sections (void)
{
  const std::string path = std::string (PACKAGE_DATADIR) + "/shaders/glass_vtk.inl";
  std::ifstream in (path);
  if (!in)
    {
      return parse_glass_vtk_inl (kGlassVtkInlFallback);
    }
  std::ostringstream buf;
  buf << in.rdbuf ();
  return parse_glass_vtk_inl (buf.str ());
}

static void
prism_attach_vtk_shader_replacements (vtkActor *actor)
{
  vtkNew<vtkOpenGLShaderProperty> shader_storage;
  actor->SetShaderProperty (shader_storage.Get ());
  vtkOpenGLShaderProperty *sp =
    vtkOpenGLShaderProperty::SafeDownCast (actor->GetShaderProperty ());
  if (!sp)
    return;

  const std::unordered_map<std::string, std::string> sec = load_glass_vtk_sections ();
  const auto get = [&sec] (const char *k) -> std::string {
    const auto it = sec.find (k);
    return it == sec.end () ? std::string () : it->second;
  };

  /* --- Vertex: view-space position (PositionVC) -------------------------- */
  /* Required for lighting (V, N in view space).                            */
  const std::string vdec  = get ("VERTEX_POSITIONVC_DEC");
  const std::string vimpl = get ("VERTEX_POSITIONVC_IMPL");
  if (!vdec.empty ())
    sp->AddVertexShaderReplacement ("//VTK::PositionVC::Dec", true,
                                    "//VTK::PositionVC::Dec\n" + vdec, false);
  if (!vimpl.empty ())
    sp->AddVertexShaderReplacement ("//VTK::PositionVC::Impl", true,
                                    "//VTK::PositionVC::Impl\n" + vimpl, false);

  /* --- Vertex: world-space position (PositionWC) ------------------------- */
  /* NEW: needed so the fragment shader can compute a world-space reflection  */
  /* vector that actually moves as the camera orbits.                        */
  /* VTK's hook is //VTK::PositionWC::Dec / ::Impl — it injects              */
  /* vertexWCVSOutput as a vec4 varying.                                     */
  const std::string wdec  = get ("VERTEX_POSITIONWC_DEC");
  const std::string wimpl = get ("VERTEX_POSITIONWC_IMPL");
  if (!wdec.empty ())
    sp->AddVertexShaderReplacement ("//VTK::PositionWC::Dec", true,
                                    "//VTK::PositionWC::Dec\n" + wdec, false);
  if (!wimpl.empty ())
    sp->AddVertexShaderReplacement ("//VTK::PositionWC::Impl", true,
                                    "//VTK::PositionWC::Impl\n" + wimpl, false);

  /* --- Fragment: lighting dec / impl ------------------------------------- */
  const std::string fdec  = get ("FRAGMENT_LIGHT_DEC");
  const std::string fimpl = get ("FRAGMENT_LIGHT_IMPL");
  if (!fdec.empty ())
    sp->AddFragmentShaderReplacement ("//VTK::Light::Dec", true,
                                      "//VTK::Light::Dec\n" + fdec, false);
  if (!fimpl.empty ())
    sp->AddFragmentShaderReplacement ("//VTK::Light::Impl", true,
                                      "//VTK::Light::Impl\n" + fimpl, false);
}

void
prism_apply_material (vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty ();
  phong_lighting (prop);
  /* Match original glass.xml.in exactly: ambient=0, diffuse=0, specular=0.05/9.
     The fragment shader does all the colour work; VTK's `specular` variable
     therefore contributes only a very small highlight, matching the old look. */
  // prop->SetOpacity (1.0);
  // prop->SetAmbient (1.0);
  // prop->SetAmbientColor (1.0, 0.0, 0.0);
  // prop->SetDiffuse (1.0);
  // prop->SetDiffuseColor (1.0, 0.0, 0.0);
  prop->SetSpecular (1.0);
  prop->SetSpecularColor (1.0, 1.0, 1.0);
  prop->SetSpecularPower (200.0);  // 80-200 for glass
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOff ();

  // vtkNew<vtkJPEGReader> sky;
  // sky->SetFileName (PACKAGE_DATADIR "/textures/sky.jpg");
  // sky->Update ();

  // vtkNew<vtkTexture> tex;
  // tex->SetInputConnection (sky->GetOutputPort ());
  // tex->InterpolateOn ();
  // actor->SetTexture (tex);

  prism_attach_vtk_shader_replacements (actor);
}

void
beam_apply_material (vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty ();
  /* The beam represents light: self-illuminated at its scalar colour, unaffected
     by scene lighting.  Phong shading would make the cylinder look dim or blue
     when the tube normal points away from the light, which changes with the torch
     direction and gives inconsistent colours.  Ambient-only avoids all of that. */
  prop->LightingOff ();
  prop->SetOpacity (1.0);           // per-cell RGBA: alpha carries Fresnel attenuation
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOff ();
  prop->SetAmbient (1.0);
  prop->SetAmbientColor (1.0, 1.0, 1.0);
  prop->SetDiffuse (0.0);
  prop->SetSpecular (0.0);
}

void
torch_apply_actor_material (vtkProperty *prop)
{
  phong_lighting (prop);
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
ground_plane_one_sided (vtkProperty *prop)
{
  prop->FrontfaceCullingOff ();
  prop->BackfaceCullingOn ();
}

void
ground_apply_material (vtkActor *actor)
{
  vtkProperty *prop = actor->GetProperty ();
  phong_lighting (prop);
  prop->SetOpacity (1.0);
  prop->SetAmbient (0.0);
  prop->SetAmbientColor (0.0, 0.6, 1.0);
  prop->SetDiffuse (1.0);
  prop->SetDiffuseColor (1.0, 1.0, 1.0);
  prop->SetSpecular (0.0);
  prop->SetSpecularColor (1.0, 1.0, 1.0);
  prop->SetSpecularPower (3.0);
  ground_plane_one_sided (prop);

  /* One-sided via vtkProperty backface culling only. A fragment `discard` on !gl_FrontFacing
     interacted badly with VTK's textured polydata shader on the first frame (dark ground until
     the next render, e.g. after moving the camera). */

  vtkNew<vtkJPEGReader> ground;
  ground->SetFileName (PACKAGE_DATADIR "/textures/ground.jpg");
  ground->Update ();

  vtkNew<vtkTexture> tex;
  tex->SetInputConnection (ground->GetOutputPort ());
  tex->InterpolateOn ();
  actor->SetTexture (tex);
}
