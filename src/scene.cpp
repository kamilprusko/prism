/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkCylinderSource.h>
#include <vtkFollower.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>

#include "beamActor.h"
#include "materials.h"
#include "prismActor.h"
#include "rayTracer.h"
#include "scene.h"
#include "torchMovedCallback.h"
#include "torchWidget.h"

namespace {

void addCamera(vtkRenderer *renderer)
{
    vtkNew<vtkCamera> camera;
    camera->SetViewUp(0.0, 0.0, 1.0);
    camera->SetPosition(-120.0, -160.0, 60.0);
    camera->SetFocalPoint(-20.0, 0.0, 10.0);
    camera->SetViewAngle(40.0);
    renderer->SetActiveCamera(camera);
}

void addLights(vtkRenderer *renderer)
{
    vtkNew<vtkLight> light;
    light->SetPosition(100.0, 100.0, 100.0);
    light->SetFocalPoint(0.0, 0.0, 20.0);
    renderer->AddLight(light);

    vtkNew<vtkLight> fillLight;
    fillLight->SetPosition(-50.0, 0.0, 50.0);
    fillLight->SetFocalPoint(0.0, 0.0, 20.0);
    renderer->AddLight(fillLight);
}

void addLabel(vtkRenderer *renderer, const char *text, double x, double y, double z)
{
    vtkNew<vtkVectorText> vectorText;
    vectorText->SetText(text);

    vtkNew<vtkPolyDataMapper> textMapper;
    textMapper->SetInputConnection(vectorText->GetOutputPort());

    vtkNew<vtkFollower> textActor;
    textActor->SetMapper(textMapper);
    ApplyLabelMaterial(textActor->GetProperty());
    textActor->SetScale(1.5, 1.5, 1.5);
    textActor->SetPosition(x, y, z);
    textActor->SetCamera(renderer->GetActiveCamera());
    renderer->AddActor(textActor);
}

void addLabels(vtkRenderer *renderer)
{
    addLabel(renderer, "Scene", -90.0, 0.0, 10.0);
    addLabel(renderer, "Sensor for\nGreen Channel", 50.0, 5.0, 10.0);
    addLabel(renderer, "Sensor for\nRed Channel", -5.0, 45.0, 10.0);
    addLabel(renderer, "Sensor for\nBlue Channel", -5.0, -40.0, 10.0);
}

void addGroundPlane(vtkRenderer *renderer, vtkPlaneSource *planeSource, bool shadowCatcher)
{
    vtkNew<vtkPolyDataMapper> planeMapper;
    planeMapper->SetInputConnection(planeSource->GetOutputPort());
    planeMapper->Update();

    vtkNew<vtkActor> plane;
    plane->SetMapper(planeMapper);

    if (shadowCatcher) {
        plane->GetProperty()->SetAmbient(1.0);
        plane->GetProperty()->SetAmbientColor(0.0, 0.0, 0.0);
        plane->GetProperty()->SetDiffuse(0.0);
        plane->GetProperty()->SetSpecular(0.0);
        ApplyOneSidedMaterial(plane->GetProperty());
    }
    else {
        ApplyGroundMaterial(plane);
    }

    renderer->AddActor(plane);
}

void addGround(vtkRenderer *renderer)
{
    vtkNew<vtkPlaneSource> groundSource;
    groundSource->SetXResolution(1);
    groundSource->SetYResolution(1);
    groundSource->SetOrigin(-300.0, -300.0, -0.05);
    groundSource->SetPoint1(300.0, -300.0, -0.05);
    groundSource->SetPoint2(-300.0, 300.0, -0.05);
    addGroundPlane(renderer, groundSource, false);

    vtkNew<vtkPlaneSource> shadowSource;
    shadowSource->SetXResolution(1);
    shadowSource->SetYResolution(1);
    shadowSource->SetOrigin(-10000.0, -10000.0, -0.2);
    shadowSource->SetPoint1(10000.0, -10000.0, -0.2);
    shadowSource->SetPoint2(-10000.0, 10000.0, -0.2);
    addGroundPlane(renderer, shadowSource, true);
}

PrismActor *addPrism(vtkRenderer *renderer, RayTracer *raytracer)
{
    PrismActor *prism = PrismActor::New();
    prism->BuildFromObj("prism.obj");
    prism->BindToRayTracer(raytracer);
    renderer->AddActor(prism);

    return prism;
}

BeamActor *addBeam(vtkRenderer *renderer, RayTracer *raytracer)
{
    BeamActor *beam = BeamActor::New();
    beam->Build();
    beam->BindToRayTracer(raytracer);
    renderer->AddActor(beam);

    return beam;
}

vtkActor *addTorch(vtkRenderer *renderer, TorchMovedCallback *callback)
{
    vtkNew<vtkCylinderSource> torchSource;
    torchSource->SetCenter(0.0, 0.0, 0.0);
    torchSource->SetRadius(3.0);
    torchSource->SetHeight(9.0);
    torchSource->SetResolution(20);

    vtkNew<vtkTransform> transform;
    transform->Identity();
    transform->RotateZ(90.0);
    transform->RotateX(10.0);
    transform->Translate(0.0, 100.0, 20.0);

    vtkTransformPolyDataFilter *torchTransform = vtkTransformPolyDataFilter::New();
    torchTransform->SetTransform(transform);
    torchTransform->SetInputConnection(torchSource->GetOutputPort());

    vtkNew<vtkPolyDataMapper> torchMapper;
    torchMapper->SetInputConnection(torchTransform->GetOutputPort());

    vtkActor *torch = vtkActor::New();
    torch->SetMapper(torchMapper);
    ApplyTorchMaterial(torch->GetProperty());

    renderer->AddActor(torch);
    callback->SetTorchTransform(torchTransform);

    return torch;
}

TorchLineWidget *addTorchWidget(vtkRenderWindowInteractor *interactor, TorchMovedCallback *callback)
{
    TorchLineWidget *widget = TorchLineWidget::New();
    widget->SetPriority(0.65f);
    widget->SetInteractor(interactor);
    ApplyTorchHandleMaterial(widget->GetHandleProperty(), widget->GetSelectedHandleProperty());
    ApplyTorchLineMaterial(widget->GetLineProperty(), widget->GetSelectedLineProperty());
    widget->ClampToBoundsOn();
    widget->PlaceWidget(-90.0, -22.0, -50.0, 50.0, 5.0, 50.0);
    widget->SetPoint1(-80.0, 0.0, 22.0);
    widget->SetPoint2(-30.0, 0.0, 20.0);
    widget->AddObserver(vtkCommand::InteractionEvent, callback);
    callback->AttachForLiveDrag(widget, interactor);
    widget->On();

    return widget;
}

} // namespace

PrismScene::PrismScene()
    : raytracer(std::make_unique<RayTracer>())
    , torchCallback(std::make_unique<TorchMovedCallback>())
{
    torchCallback->SetRaytracer(raytracer.get());
}

PrismScene::~PrismScene()
{
    Shutdown();
}

void PrismScene::Build(vtkRenderer *renderer, vtkRenderWindow *window)
{
    this->renderer = renderer;
    this->interactor = vtkRenderWindowInteractor::New();
    this->interactor->SetRenderWindow(window);

    addCamera(renderer);
    addLights(renderer);
    addLabels(renderer);
    addGround(renderer);
    this->prism = addPrism(renderer, raytracer.get());
    this->beam = addBeam(renderer, raytracer.get());
    this->torchProp = addTorch(renderer, torchCallback.get());
    this->torchWidget = addTorchWidget(this->interactor, torchCallback.get());

    this->style = TorchInteractorStyle::New();
    this->style->SetPriority(0.55f);
    this->style->SetTorchCallback(torchCallback.get());
    this->style->SetLineWidget(this->torchWidget);
    this->interactor->SetInteractorStyle(this->style);
}

void PrismScene::Run()
{
    this->renderer->ResetCameraClippingRange();
    this->interactor->Initialize();

    torchCallback->Execute(torchWidget, 0, nullptr);

    this->interactor->GetRenderWindow()->Render();
    this->interactor->Start();
}

void PrismScene::Shutdown()
{
    if (raytracer)
        raytracer->DisconnectFromScene();

    if (torchCallback) {
        torchCallback->SetRaytracer(nullptr);
        torchCallback->ReleaseVtkResources();
    }

    if (this->style) {
        this->style->SetTorchCallback(nullptr);
        this->style->SetLineWidget(nullptr);
    }

    if (this->torchWidget) {
        this->torchWidget->Off();
        this->torchWidget->RemoveAllObservers();
        this->torchWidget->SetInteractor(nullptr);
    }

    if (this->renderer) {
        if (this->prism)
            this->renderer->RemoveActor(this->prism);

        if (this->beam)
            this->renderer->RemoveActor(this->beam);

        if (this->torchProp)
            this->renderer->RemoveActor(this->torchProp);
    }

    if (this->prism) {
        this->prism->Delete();
        this->prism = nullptr;
    }

    if (this->beam) {
        this->beam->Delete();
        this->beam = nullptr;
    }

    if (this->torchProp) {
        this->torchProp->Delete();
        this->torchProp = nullptr;
    }

    if (this->torchWidget) {
        this->torchWidget->Delete();
        this->torchWidget = nullptr;
    }

    if (this->interactor)
        this->interactor->SetInteractorStyle(nullptr);

    if (this->style) {
        this->style->Delete();
        this->style = nullptr;
    }

    if (this->interactor) {
        this->interactor->Delete();
        this->interactor = nullptr;
    }

    this->renderer = nullptr;
}
