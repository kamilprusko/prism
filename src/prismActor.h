/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRISM_ACTOR_H
#define PRISM_ACTOR_H

#include <vtkOpenGLActor.h>
#include <vtkType.h>

#include "rayTracer.h"

class vtkPolyDataMapper;

class PrismActor : public vtkOpenGLActor
{
  public:
    static PrismActor *New();
    vtkTypeMacro(PrismActor, vtkOpenGLActor);

    /** Cell ids in data/objects/prism.obj — per-face channel splitters for this scene. */
    static constexpr vtkIdType BlueSensorFaceA = 138;
    static constexpr vtkIdType BlueSensorFaceB = 139;
    static constexpr vtkIdType RgSplitterFaceA = 26;
    static constexpr vtkIdType RgSplitterFaceB = 27;

    void BuildFromObj(const char *filename);
    vtkPolyDataMapper *TraceSurfaceMapper() const;
    void ApplySensorColors(RayHitColorContext &ctx) const;
    void BindToRayTracer(RayTracer *raytracer);
    void PrintSelf(ostream &os, vtkIndent indent) override;

  protected:
    PrismActor();
    ~PrismActor() override;

  private:
    PrismActor(const PrismActor &) = delete;
    void operator=(const PrismActor &) = delete;
};

#endif
