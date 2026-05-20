/*
 * Copyright (c) 2010  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <cstddef>
#include <functional>

#include <vtkType.h>

class BeamActor;
class vtkCellArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkUnsignedCharArray;

/** Per-hit data for optional color hooks (indices are trace-mesh cell ids). */
struct RayHitColorContext {
    vtkIdType PrimaryCellId;
    const vtkIdType *RelatedCellIds;
    std::size_t RelatedCellIdsCapacity;
    double HitPoint[3];
    double Normal[3];
    bool RayOriginInside;
    double N1;
    double N2;
    const double *IncomingRgba;
    double RefractedRgba[4];
    double ReflectedRgba[4];
};

using RayTracerColorCallback = std::function<void(RayHitColorContext &)>;

class RayTracer
{
  public:
    struct Limits {
        static constexpr int MaxCastDepth = 64;
        static constexpr int MaxBeamPoints = 512;
        static constexpr int ExclusionIdSlack = 8;
        static constexpr std::size_t ExcludedFaceIdCapacity =
            static_cast<std::size_t>(MaxCastDepth + ExclusionIdSlack);
        static constexpr double MaxDistance = 1.0e15;
        static constexpr double MinRayLength = 1.0e-6;
        static constexpr double MinBranchAlpha = 0.01;
    };

    RayTracer();
    ~RayTracer();
    void SetBeamData(vtkPolyData *data);
    void SetBeamActor(BeamActor *beam);
    void SetTraceSurfaceMapper(vtkPolyDataMapper *mapper);
    void SetHitColorCallback(RayTracerColorCallback cb);
    /** Drop VTK/scene pointers before destroying connected actors. */
    void DisconnectFromScene();
    void Render(const double origin[3], const double direction[3], const double color[4]);

  private:
    vtkPoints *points;
    vtkCellArray *lines;
    vtkUnsignedCharArray *colors;
    vtkPolyData *beamData;
    BeamActor *beamActor;
    vtkPolyDataMapper *traceSurfaceMapper;
    RayTracerColorCallback onHitColor;

    void cast(const double origin[3], const double direction[3], const double color[4],
              vtkIdType originPointId, const vtkIdType *originPolyIds,
              std::size_t originPolyIdsCount, int castDepth);
    void update();
};

#endif
