/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRISM_SCENE_H
#define PRISM_SCENE_H

#include <memory>

class BeamActor;
class PrismActor;
class RayTracer;
class TorchLineWidget;
class TorchMovedCallback;

class vtkActor;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class TorchInteractorStyle;

/** Builds and tears down scene content; does not own the render window or renderer. */
class PrismScene
{
  public:
    PrismScene();
    ~PrismScene();

    PrismScene(const PrismScene &) = delete;
    PrismScene &operator=(const PrismScene &) = delete;

    void Build(vtkRenderer *renderer, vtkRenderWindow *window);
    void Run();
    void Shutdown();

  private:
    std::unique_ptr<RayTracer> raytracer;
    std::unique_ptr<TorchMovedCallback> torchCallback;

    vtkRenderer *renderer = nullptr;
    vtkRenderWindowInteractor *interactor = nullptr;
    TorchInteractorStyle *style = nullptr;
    PrismActor *prism = nullptr;
    BeamActor *beam = nullptr;
    vtkActor *torchProp = nullptr;
    TorchLineWidget *torchWidget = nullptr;
};

#endif
