/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TORCH_MOVED_CALLBACK_H
#define TORCH_MOVED_CALLBACK_H

#include <vtkCommand.h>

class RayTracer;

class vtkLineWidget;
class vtkRenderWindowInteractor;
class vtkTransform;
class vtkTransformPolyDataFilter;

class TorchMovedCallback : public vtkCommand
{
  public:
    void SetRaytracer(RayTracer *raytracer);
    void SetTorchTransform(vtkTransformPolyDataFilter *torchTransform);
    void ReleaseVtkResources();
    void AttachForLiveDrag(vtkLineWidget *widget, vtkRenderWindowInteractor *interactor);
    void Execute(vtkObject *caller, unsigned long, void *) override;
    bool IsWidgetDragging() const { return widgetDragging; }
    void SyncTorchFromWidget(vtkLineWidget *widget);

  private:
    RayTracer *raytracer = nullptr;
    vtkTransformPolyDataFilter *torchTransform = nullptr;
    vtkLineWidget *lineWidget = nullptr;
    bool widgetDragging = false;

    void syncFromWidget(vtkLineWidget *widget);
    void setTransformDirection(vtkTransform *transform, const double direction[3]);

    static void onWidgetStart(vtkObject *, unsigned long, void *clientdata, void *);
    static void onWidgetEnd(vtkObject *, unsigned long, void *clientdata, void *);
};

#endif
