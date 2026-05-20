/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <vtkIndent.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTubeFilter.h>

#include "beamActor.h"
#include "materials.h"
#include "rayTracer.h"

vtkStandardNewMacro(BeamActor);

BeamActor::BeamActor() : beamPolyData(nullptr), tubes(nullptr) {}

BeamActor::~BeamActor()
{
    this->SetMapper(nullptr);

    if (this->tubes) {
        this->tubes->Delete();
        this->tubes = nullptr;
    }

    if (this->beamPolyData) {
        this->beamPolyData->Delete();
        this->beamPolyData = nullptr;
    }
}

void BeamActor::Build()
{
    this->beamPolyData = vtkPolyData::New();

    this->tubes = vtkTubeFilter::New();
    this->tubes->SetInputData(this->beamPolyData);
    this->tubes->SetRadius(0.5);
    this->tubes->SetNumberOfSides(12);

    vtkNew<vtkPolyDataMapper> beamMapper;
    beamMapper->SetInputConnection(this->tubes->GetOutputPort());
    beamMapper->SetColorModeToDirectScalars();
    beamMapper->SetScalarModeToUseCellData();
    beamMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-6.0, -6.0);

    this->SetMapper(beamMapper);

    ApplyBeamMaterial(this);
    this->GetProperty()->SetLineWidth(4.0);
    this->Modified();
}

vtkPolyData *BeamActor::PolyData() const
{
    return this->beamPolyData;
}

void BeamActor::NotifyGeometryChanged()
{
    if (this->beamPolyData)
        this->beamPolyData->Modified();

    if (this->tubes)
        this->tubes->Update();

    this->Modified();
}

void BeamActor::BindToRayTracer(RayTracer *raytracer)
{
    if (!raytracer)
        return;

    raytracer->SetBeamData(this->beamPolyData);
    raytracer->SetBeamActor(this);
}

void BeamActor::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "beamPolyData: " << this->beamPolyData << "\n";
    os << indent << "tubes: " << this->tubes << "\n";
}
