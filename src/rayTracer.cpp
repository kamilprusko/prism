/*
 * Copyright (c) 2010  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>

#include "rayTracer.h"
#include "beamActor.h"

namespace {

constexpr vtkIdType fallbackMaxVertsPerCell = 16;
constexpr double colorEpsilon = 1.0e-9;

bool isExcludedPoly(int polyId, int polyIdLimit, const vtkIdType *originPolyIds)
{
    for (int i = 0; i < polyIdLimit && originPolyIds[i] >= 0; ++i) {
        if (originPolyIds[i] == polyId)
            return true;
    }
    return false;
}

bool isNan(double var)
{
    return std::isnan(var);
}

double reflect(const double normal[3], const double incident[3], double reflection[3])
{
    const double cosTheta1 = -vtkMath::Dot(incident, normal);

    reflection[0] = incident[0] + 2.0 * cosTheta1 * normal[0];
    reflection[1] = incident[1] + 2.0 * cosTheta1 * normal[1];
    reflection[2] = incident[2] + 2.0 * cosTheta1 * normal[2];

    vtkMath::Normalize(reflection);

    return 1.0;
}

double refract(const double normal[3], const double incident[3], double refraction[3],
               const double n1, const double n2)
{
    const double eta = n1 / n2;
    const double cosTheta1 = -vtkMath::Dot(incident, normal);
    const double cosTheta2 = std::sqrt(1.0 - (eta * eta * (1.0 - cosTheta1 * cosTheta1)));

    if (isNan(cosTheta2))
        return 0.0;

    refraction[0] = eta * incident[0] + (eta * cosTheta1 - cosTheta2) * normal[0];
    refraction[1] = eta * incident[1] + (eta * cosTheta1 - cosTheta2) * normal[1];
    refraction[2] = eta * incident[2] + (eta * cosTheta1 - cosTheta2) * normal[2];

    vtkMath::Normalize(refraction);

    const double fresnel_Rs =
        std::pow((n1 * cosTheta1 - n2 * cosTheta2) / (n1 * cosTheta1 + n2 * cosTheta2), 2);
    const double fresnel_Rp =
        std::pow((n1 * cosTheta2 - n2 * cosTheta1) / (n1 * cosTheta2 + n2 * cosTheta1), 2);

    const double reflectance = (fresnel_Rs + fresnel_Rp) / 2.0;

    return 1.0 - reflectance;
}

bool intersectTriangle(const double orig[3], const double direction[3], const double v0[3],
                       const double v1[3], const double v2[3], double *hitDistance, double *baryU,
                       double *baryV)
{
    double dir[3] = {direction[0], direction[1], direction[2]};
    vtkMath::Normalize(dir);

    double edge1[3];
    double edge2[3];
    double pvec[3];
    double qvec[3];
    double tvec[3];
    double det;
    double invDet;

    vtkMath::Subtract(v1, v0, edge1);
    vtkMath::Subtract(v2, v0, edge2);

    vtkMath::Cross(dir, edge2, pvec);

    det = vtkMath::Dot(edge1, pvec);

    if (det > -VTK_DBL_EPSILON && det < VTK_DBL_EPSILON)
        return false;

    invDet = 1.0 / det;

    vtkMath::Subtract(orig, v0, tvec);

    *baryU = vtkMath::Dot(tvec, pvec) * invDet;
    if (*baryU < 0.0 || *baryU > 1.0)
        return false;

    vtkMath::Cross(tvec, edge1, qvec);

    *baryV = vtkMath::Dot(dir, qvec) * invDet;
    if (*baryV < 0.0 || *baryU + *baryV > 1.0)
        return false;

    *hitDistance = vtkMath::Dot(edge2, qvec) * invDet;

    return true;
}

bool cullTest(const double direction[3], double normal[3])
{
    return vtkMath::Dot(direction, normal) >= 0.0;
}

bool isZeroRgb(const double rgba[4])
{
    return std::abs(rgba[0]) < colorEpsilon && std::abs(rgba[1]) < colorEpsilon &&
           std::abs(rgba[2]) < colorEpsilon;
}

vtkPolyData *traceMeshFromMapper(vtkPolyDataMapper *mapper)
{
    if (!mapper)
        return nullptr;

    mapper->Update();

    vtkPolyData *input = vtkPolyData::SafeDownCast(mapper->GetInput());
    if (input && input->GetNumberOfCells() > 0)
        return input;

    vtkAlgorithm *producer = mapper->GetInputAlgorithm();
    if (!producer)
        return nullptr;

    producer->Update();

    return vtkPolyData::SafeDownCast(producer->GetOutputDataObject(0));
}

} // namespace

RayTracer::RayTracer()
    : points(nullptr)
    , lines(nullptr)
    , colors(nullptr)
    , beamData(nullptr)
    , beamActor(nullptr)
    , traceSurfaceMapper(nullptr)
{
}

RayTracer::~RayTracer()
{
    this->DisconnectFromScene();
    this->points = nullptr;
    this->lines = nullptr;
    this->colors = nullptr;
}

void RayTracer::SetBeamData(vtkPolyData *data)
{
    this->beamData = data;
}

void RayTracer::SetBeamActor(BeamActor *beam)
{
    this->beamActor = beam;
}

void RayTracer::SetTraceSurfaceMapper(vtkPolyDataMapper *mapper)
{
    this->traceSurfaceMapper = mapper;
}

void RayTracer::SetHitColorCallback(RayTracerColorCallback cb)
{
    this->onHitColor = std::move(cb);
}

void RayTracer::DisconnectFromScene()
{
    this->beamData = nullptr;
    this->beamActor = nullptr;
    this->traceSurfaceMapper = nullptr;
    this->onHitColor = {};
    this->points = nullptr;
    this->lines = nullptr;
    this->colors = nullptr;
}

void RayTracer::Render(const double origin[3], const double direction[3], const double color[4])
{
    this->points = vtkPoints::New();
    this->lines = vtkCellArray::New();
    this->colors = vtkUnsignedCharArray::New();
    this->colors->SetNumberOfComponents(4);

    std::vector<vtkIdType> originPolyIds(Limits::ExcludedFaceIdCapacity,
                                         static_cast<vtkIdType>(-1));
    const vtkIdType originPointId = this->points->InsertNextPoint(origin);

    double directionNormalized[3] = {direction[0], direction[1], direction[2]};

    vtkMath::Normalize(directionNormalized);

    this->cast(origin,
               directionNormalized,
               color,
               originPointId,
               originPolyIds.data(),
               originPolyIds.size(),
               0);
    this->update();
}

void RayTracer::cast(const double origin[3], const double direction[3], const double color[4],
                     const vtkIdType originPointId, const vtkIdType *originPolyIds,
                     const std::size_t originPolyIdsCount, const int castDepth)
{
    if (castDepth >= Limits::MaxCastDepth)
        return;

    if (!this->traceSurfaceMapper)
        return;

    vtkPolyData *input = traceMeshFromMapper(this->traceSurfaceMapper);
    if (!input)
        return;

    vtkPoints *inputPoints = input->GetPoints();
    vtkCellArray *inputPolys = input->GetPolys();
    if (!inputPoints || !inputPolys)
        return;

    std::vector<vtkIdType> projectedPolyIds(originPolyIdsCount, static_cast<vtkIdType>(-1));
    long projectedPolyIdsLength = 0;

    double refracted[3];
    double reflected[3];
    double normal[3];
    double length = 10000.0;
    double transmission = 1.0;
    double reflectance = 0.0;
    double n1 = 1.0003;
    double n2 = 1.5000;
    double projectedDistance = Limits::MaxDistance;
    double projected[3] = {
        origin[0] + length * direction[0],
        origin[1] + length * direction[1],
        origin[2] + length * direction[2]
    };

    bool isInside = false;

    vtkIdType pointCount;
    const vtkIdType *pointIds;

    const vtkIdType maxCellSize = input->GetMaxCellSize();
    const vtkIdType minCellVerts = 3;
    std::vector<std::array<double, 3>> p(
        static_cast<size_t>(maxCellSize > minCellVerts ? maxCellSize : fallbackMaxVertsPerCell));

    double distance;
    double hitDistance;
    double baryU;
    double baryV;

    inputPolys->InitTraversal();

    const int polyIdLimit = static_cast<int>(originPolyIdsCount);

    for (int polyId = 0; inputPolys->GetNextCell(pointCount, pointIds); polyId++) {
        if (isExcludedPoly(polyId, polyIdLimit, originPolyIds))
            continue;

        if (pointCount > static_cast<vtkIdType>(p.size()))
            p.resize(static_cast<size_t>(pointCount));

        for (int i = 0; i < pointCount; i++) {
            inputPoints->GetPoint(pointIds[i], p[static_cast<size_t>(i)].data());
        }

        if (!intersectTriangle(origin,
                               direction,
                               p[0].data(),
                               p[1].data(),
                               p[2].data(),
                               &hitDistance,
                               &baryU,
                               &baryV))
            continue;

        if (isNan(hitDistance) || hitDistance < 0.0)
            continue;

        const double intersected[3] = {
            origin[0] + hitDistance * direction[0],
            origin[1] + hitDistance * direction[1],
            origin[2] + hitDistance * direction[2]
        };

        distance = std::sqrt((origin[0] - intersected[0]) * (origin[0] - intersected[0]) +
                             (origin[1] - intersected[1]) * (origin[1] - intersected[1]) +
                             (origin[2] - intersected[2]) * (origin[2] - intersected[2]));

        if (distance < projectedDistance) {
            projected[0] = intersected[0];
            projected[1] = intersected[1];
            projected[2] = intersected[2];

            projectedPolyIdsLength = 0;
            projectedPolyIds[static_cast<size_t>(projectedPolyIdsLength++)] = polyId;
            projectedDistance = distance;

            const double edge1[3] = {p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]};
            const double edge2[3] = {p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]};
            vtkMath::Cross(edge1, edge2, normal);
            vtkMath::Normalize(normal);

            isInside = cullTest(direction, normal);

            if (isInside) {
                normal[0] = -normal[0];
                normal[1] = -normal[1];
                normal[2] = -normal[2];
            }

            if (projectedDistance < Limits::MinRayLength) {
                for (int i = 0; i < polyIdLimit && originPolyIds[i] >= 0 &&
                                projectedPolyIdsLength < polyIdLimit - 1;
                     i++) {
                    projectedPolyIds[static_cast<size_t>(projectedPolyIdsLength++)] =
                        originPolyIds[i];
                }

                break;
            }
        }
    }

    vtkIdType projectedPointId = originPointId;

    double projectedColor[4] = {color[0], color[1], color[2], color[3]};
    for (int c = 0; c < 4; ++c) {
        projectedColor[c] = std::clamp(projectedColor[c], 0.0, 1.0);
    }

    const double vis = projectedColor[3];
    const unsigned char projectedColorUc[4] = {
        static_cast<unsigned char>(255.0 * std::clamp(projectedColor[0], 0.0, 1.0)),
        static_cast<unsigned char>(255.0 * std::clamp(projectedColor[1], 0.0, 1.0)),
        static_cast<unsigned char>(255.0 * std::clamp(projectedColor[2], 0.0, 1.0)),
        static_cast<unsigned char>(255.0 * std::clamp(vis, 0.0, 1.0))
    };

    if (projectedDistance > Limits::MinRayLength) {
        projectedPointId = this->points->InsertNextPoint(projected);

        vtkLine *line = vtkLine::New();
        line->GetPointIds()->SetId(0, originPointId);
        line->GetPointIds()->SetId(1, projectedPointId);
        this->lines->InsertNextCell(line);
        line->Delete();
        this->colors->InsertNextTypedTuple(projectedColorUc);
    }

    if (this->points->GetNumberOfPoints() > Limits::MaxBeamPoints)
        return;

    if (projectedPolyIdsLength + 1 < polyIdLimit) {
        projectedPolyIds[static_cast<size_t>(projectedPolyIdsLength + 1)] =
            static_cast<vtkIdType>(-1);
    }

    if (isInside) {
        const double tmp = n1;
        n1 = n2;
        n2 = tmp;
    }

    if (projectedDistance < Limits::MaxDistance) {
        transmission = refract(normal, direction, refracted, n1, n2);
        reflectance = reflect(normal, direction, reflected);
        reflectance = 1.0 - transmission;

        double refractedColor[4] = {color[0], color[1], color[2], transmission * color[3]},
               reflectedColor[4] = {color[0], color[1], color[2], reflectance * color[3]};

        RayHitColorContext ctx{};
        ctx.PrimaryCellId = projectedPolyIds[0];
        ctx.RelatedCellIds = projectedPolyIds.data();
        ctx.RelatedCellIdsCapacity = originPolyIdsCount;
        ctx.HitPoint[0] = projected[0];
        ctx.HitPoint[1] = projected[1];
        ctx.HitPoint[2] = projected[2];
        ctx.Normal[0] = normal[0];
        ctx.Normal[1] = normal[1];
        ctx.Normal[2] = normal[2];
        ctx.RayOriginInside = isInside;
        ctx.N1 = n1;
        ctx.N2 = n2;
        ctx.IncomingRgba = color;

        if (this->onHitColor) {
            std::memcpy(ctx.RefractedRgba, refractedColor, sizeof(ctx.RefractedRgba));
            std::memcpy(ctx.ReflectedRgba, reflectedColor, sizeof(ctx.ReflectedRgba));
            this->onHitColor(ctx);
            std::memcpy(refractedColor, ctx.RefractedRgba, sizeof(ctx.RefractedRgba));
            std::memcpy(reflectedColor, ctx.ReflectedRgba, sizeof(ctx.ReflectedRgba));
        }

        if (isZeroRgb(reflectedColor))
            reflectedColor[3] = 0.0;

        if (refractedColor[3] > Limits::MinBranchAlpha) {
            this->cast(projected,
                       refracted,
                       refractedColor,
                       projectedPointId,
                       projectedPolyIds.data(),
                       originPolyIdsCount,
                       castDepth + 1);
        }

        if (reflectedColor[3] > Limits::MinBranchAlpha) {
            this->cast(projected,
                       reflected,
                       reflectedColor,
                       projectedPointId,
                       projectedPolyIds.data(),
                       originPolyIdsCount,
                       castDepth + 1);
        }
    }
}

void RayTracer::update()
{
    if (!this->beamData)
        return;

    this->beamData->SetPoints(this->points);
    this->beamData->SetLines(this->lines);
    this->beamData->GetCellData()->SetScalars(this->colors);

    if (this->beamActor)
        this->beamActor->NotifyGeometryChanged();
}
