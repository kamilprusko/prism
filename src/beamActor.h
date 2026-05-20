/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BEAM_ACTOR_H
#define BEAM_ACTOR_H

#include <vtkOpenGLActor.h>

class vtkPolyData;
class vtkTubeFilter;

class RayTracer;

class BeamActor : public vtkOpenGLActor
{
  public:
    static BeamActor *New();
    vtkTypeMacro(BeamActor, vtkOpenGLActor);

    void Build();
    vtkPolyData *PolyData() const;
    void NotifyGeometryChanged();
    void BindToRayTracer(RayTracer *raytracer);
    void PrintSelf(ostream &os, vtkIndent indent) override;

  protected:
    BeamActor();
    ~BeamActor() override;

  private:
    BeamActor(const BeamActor &) = delete;
    void operator=(const BeamActor &) = delete;

    vtkPolyData *beamPolyData;
    vtkTubeFilter *tubes;
};

#endif
