/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <vtkIndent.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>

#include "materials.h"
#include "prismActor.h"
#include "rayTracer.h"
#include "utils.h"

vtkStandardNewMacro(PrismActor);

PrismActor::PrismActor() {}

PrismActor::~PrismActor()
{
    this->SetMapper(nullptr);
}

void PrismActor::BuildFromObj(const char *filename)
{
    LoadObject(this, filename);
    ApplyPrismMaterial(this);
    this->ForceTranslucentOn();
    this->Modified();
}

vtkPolyDataMapper *PrismActor::TraceSurfaceMapper() const
{
    return vtkPolyDataMapper::SafeDownCast(const_cast<PrismActor *>(this)->GetMapper());
}

void PrismActor::ApplySensorColors(RayHitColorContext &ctx) const
{
    const vtkIdType p0 = ctx.PrimaryCellId;
    const vtkIdType *ids = ctx.RelatedCellIds;

    const bool blueSplitFace = (p0 == BlueSensorFaceA || p0 == BlueSensorFaceB);
    const bool rgSplitFace = (p0 == RgSplitterFaceA || p0 == RgSplitterFaceB) ||
                             (ctx.RelatedCellIdsCapacity >= 2 &&
                              (ids[1] == RgSplitterFaceA || ids[1] == RgSplitterFaceB));

    if (blueSplitFace) {
        ctx.RefractedRgba[2] = 0.0;
        ctx.ReflectedRgba[0] = ctx.ReflectedRgba[1] = 0.0;
        ctx.RefractedRgba[3] = ctx.ReflectedRgba[3] = ctx.IncomingRgba[3];
    }

    if (rgSplitFace) {
        ctx.RefractedRgba[0] = ctx.RefractedRgba[2] = 0.0;
        ctx.ReflectedRgba[1] = ctx.ReflectedRgba[2] = 0.0;
        ctx.RefractedRgba[3] = ctx.ReflectedRgba[3] = ctx.IncomingRgba[3];
    }
}

void PrismActor::BindToRayTracer(RayTracer *raytracer)
{
    if (!raytracer)
        return;

    raytracer->SetTraceSurfaceMapper(this->TraceSurfaceMapper());
    raytracer->SetHitColorCallback(
        [this](RayHitColorContext &ctx) { this->ApplySensorColors(ctx); });
}

void PrismActor::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
}
