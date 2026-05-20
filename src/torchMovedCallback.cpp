/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkLineWidget.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSetGet.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "rayTracer.h"
#include "torchMovedCallback.h"

void TorchMovedCallback::SetRaytracer(RayTracer *raytracer)
{
    this->raytracer = raytracer;
}

void TorchMovedCallback::SetTorchTransform(vtkTransformPolyDataFilter *torchTransform)
{
    this->torchTransform = torchTransform;
}

void TorchMovedCallback::ReleaseVtkResources()
{
    if (this->torchTransform) {
        this->torchTransform->Delete();
        this->torchTransform = nullptr;
    }

    this->lineWidget = nullptr;
}

void TorchMovedCallback::AttachForLiveDrag(vtkLineWidget *widget,
                                           vtkRenderWindowInteractor *interactor)
{
    vtkNotUsed(interactor);
    this->lineWidget = widget;

    vtkNew<vtkCallbackCommand> start;
    start->SetClientData(this);
    start->SetCallback(TorchMovedCallback::onWidgetStart);
    widget->AddObserver(vtkCommand::StartInteractionEvent, start);

    vtkNew<vtkCallbackCommand> end;
    end->SetClientData(this);
    end->SetCallback(TorchMovedCallback::onWidgetEnd);
    widget->AddObserver(vtkCommand::EndInteractionEvent, end);
}

void TorchMovedCallback::onWidgetStart(vtkObject *, unsigned long, void *clientdata, void *)
{
    static_cast<TorchMovedCallback *>(clientdata)->widgetDragging = true;
}

void TorchMovedCallback::onWidgetEnd(vtkObject *, unsigned long, void *clientdata, void *)
{
    TorchMovedCallback *self = static_cast<TorchMovedCallback *>(clientdata);
    self->widgetDragging = false;
    if (self->lineWidget)
        self->syncFromWidget(self->lineWidget);
}

void TorchMovedCallback::Execute(vtkObject *caller, unsigned long, void *)
{
    vtkLineWidget *widget = vtkLineWidget::SafeDownCast(caller);
    if (!widget)
        return;
    this->syncFromWidget(widget);
}

void TorchMovedCallback::SyncTorchFromWidget(vtkLineWidget *widget)
{
    this->syncFromWidget(widget);
}

void TorchMovedCallback::syncFromWidget(vtkLineWidget *widget)
{
    if (!widget)
        return;

    const double *a = widget->GetPoint1();
    const double *b = widget->GetPoint2();
    double direction[3];
    double origin[3];
    double color[4] = {1.0, 1.0, 1.0, 0.8};

    vtkMath::Subtract(b, a, direction);
    vtkMath::Normalize(direction);
    vtkMath::MultiplyScalar(direction, 15.0);
    vtkMath::Subtract(a, direction, origin);

    vtkNew<vtkTransform> transform;
    transform->Identity();
    transform->PostMultiply();
    transform->RotateZ(90.0);

    this->setTransformDirection(transform, direction);
    transform->Translate(origin[0], origin[1], origin[2]);

    if (this->torchTransform) {
        this->torchTransform->SetTransform(transform);
        this->torchTransform->Update();
    }

    if (this->raytracer)
        this->raytracer->Render(origin, direction, color);

    vtkRenderWindowInteractor *widgetInteractor = widget->GetInteractor();
    if (widgetInteractor)
        widgetInteractor->Render();
}

void TorchMovedCallback::setTransformDirection(vtkTransform *transform, const double direction[3])
{
    double directionNormalized[3] = {direction[0], direction[1], direction[2]};
    vtkMath::Normalize(directionNormalized);

    double x[3] = {1.0, 0.0, 0.0};
    const double c = vtkMath::Dot(x, directionNormalized);

    if (c > 1.0 - 1.0e-7)
        return;

    if (c < -1.0 + 1.0e-7) {
        transform->RotateWXYZ(180.0, 0.0, 0.0, 1.0);
        return;
    }

    double axis[3];
    vtkMath::Cross(x, directionNormalized, axis);
    const double theta = acos(c);
    transform->RotateWXYZ(theta * 180.0 / vtkMath::Pi(), axis[0], axis[1], axis[2]);
}
