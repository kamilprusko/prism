/*
 * Copyright (c) 2010,2026  Kamil Prusko
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <string>
#include <unordered_map>

#include <vtkActor.h>
#include <vtkOpenGLShaderProperty.h>
#include <vtkProperty.h>

#include "materials.h"
#include "utils.h"

namespace {

void applyPhongLighting(vtkProperty *prop)
{
    prop->SetInterpolationToPhong();
    prop->LightingOn();
}

void applyGlassShader(vtkActor *actor)
{
    vtkOpenGLShaderProperty *sp = vtkOpenGLShaderProperty::New();
    actor->SetShaderProperty(sp);
    sp->Delete();

    const std::unordered_map<std::string, std::string> sec = LoadShader("glass.inl");
    const auto get = [&sec](const char *k) -> std::string {
        const auto it = sec.find(k);
        return it == sec.end() ? std::string() : it->second;
    };

    /* Vertex shader: view-space position (PositionVC) is required for lighting (V, N in view space)
     */
    const std::string vdec = get("VERTEX_POSITIONVC_DEC");
    const std::string vimpl = get("VERTEX_POSITIONVC_IMPL");
    if (!vdec.empty())
        sp->AddVertexShaderReplacement(
            "//VTK::PositionVC::Dec", true, "//VTK::PositionVC::Dec\n" + vdec, false);
    if (!vimpl.empty())
        sp->AddVertexShaderReplacement(
            "//VTK::PositionVC::Impl", true, "//VTK::PositionVC::Impl\n" + vimpl, false);

    /* Vertex shader: world-space position (PositionWC) is needed by the fragment shader,
     * so it can compute a world-space reflection vector that actually moves as the camera orbits.
     */
    const std::string wdec = get("VERTEX_POSITIONWC_DEC");
    const std::string wimpl = get("VERTEX_POSITIONWC_IMPL");
    if (!wdec.empty())
        sp->AddVertexShaderReplacement(
            "//VTK::PositionWC::Dec", true, "//VTK::PositionWC::Dec\n" + wdec, false);
    if (!wimpl.empty())
        sp->AddVertexShaderReplacement(
            "//VTK::PositionWC::Impl", true, "//VTK::PositionWC::Impl\n" + wimpl, false);

    /* Fragment shader */
    const std::string fdec = get("FRAGMENT_LIGHT_DEC");
    const std::string fimpl = get("FRAGMENT_LIGHT_IMPL");
    if (!fdec.empty())
        sp->AddFragmentShaderReplacement(
            "//VTK::Light::Dec", true, "//VTK::Light::Dec\n" + fdec, false);
    if (!fimpl.empty())
        sp->AddFragmentShaderReplacement(
            "//VTK::Light::Impl", true, "//VTK::Light::Impl\n" + fimpl, false);
}

} // namespace

void ApplyOneSidedMaterial(vtkProperty *prop)
{
    prop->FrontfaceCullingOff();
    prop->BackfaceCullingOn();
}

void ApplyPrismMaterial(vtkActor *actor)
{
    vtkProperty *prop = actor->GetProperty();
    prop->SetSpecular(1.0);
    prop->SetSpecularColor(1.0, 1.0, 1.0);
    prop->SetSpecularPower(200.0); // 80-200 for glass
    prop->FrontfaceCullingOff();
    prop->BackfaceCullingOff();

    applyGlassShader(actor);
    applyPhongLighting(prop);
}

void ApplyBeamMaterial(vtkActor *actor)
{
    vtkProperty *prop = actor->GetProperty();
    prop->LightingOff();
    prop->SetOpacity(1.0);
    prop->FrontfaceCullingOff();
    prop->BackfaceCullingOff();
    prop->SetAmbient(1.0);
    prop->SetAmbientColor(1.0, 1.0, 1.0);
    prop->SetDiffuse(0.0);
    prop->SetSpecular(0.0);
}

void ApplyTorchMaterial(vtkProperty *prop)
{
    prop->SetOpacity(1.0);
    prop->SetAmbient(0.5);
    prop->SetAmbientColor(0.0, 0.6, 1.0);
    prop->SetDiffuse(0.25);
    prop->SetDiffuseColor(0.5, 0.8, 1.0);
    prop->SetSpecular(0.0);
    prop->SetSpecularColor(1.0, 1.0, 1.0);
    prop->SetSpecularPower(2.0);
    prop->FrontfaceCullingOff();
    prop->BackfaceCullingOff();

    applyPhongLighting(prop);
}

void ApplyTorchHandleMaterial(vtkProperty *handle, vtkProperty *selectedHandle)
{
    ApplyTorchMaterial(handle);
    ApplyTorchMaterial(selectedHandle);
}

void ApplyTorchLineMaterial(vtkProperty *line, vtkProperty *selectedLine)
{
    // Use VTK default style for lines
}

void ApplyLabelMaterial(vtkProperty *prop)
{
    prop->SetOpacity(1.0);
    prop->SetAmbient(1.0);
    prop->SetAmbientColor(1.0, 1.0, 1.0);
    prop->SetDiffuse(0.0);
    prop->SetDiffuseColor(0.5, 0.8, 1.0);
    prop->SetSpecular(0.0);
    prop->SetSpecularColor(1.0, 1.0, 1.0);
    prop->SetSpecularPower(2.0);

    applyPhongLighting(prop);
}

void ApplyGroundMaterial(vtkActor *actor)
{
    vtkProperty *prop = actor->GetProperty();
    prop->SetOpacity(1.0);
    prop->SetAmbient(0.0);
    prop->SetAmbientColor(0.0, 0.6, 1.0);
    prop->SetDiffuse(1.0);
    prop->SetDiffuseColor(1.0, 1.0, 1.0);
    prop->SetSpecular(0.0);
    prop->SetSpecularColor(1.0, 1.0, 1.0);
    prop->SetSpecularPower(3.0);

    ApplyOneSidedMaterial(prop);
    applyPhongLighting(prop);

    LoadTexture(actor, "ground.jpg");
}
