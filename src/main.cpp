/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include "scene.h"

int main()
{
    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.0, 0.0, 0.0);
    renderer->UseDepthPeelingOff();

    vtkNew<vtkRenderWindow> window;
    window->SetSize(1280, 720);
    window->AlphaBitPlanesOn();
    window->SetMultiSamples(0); // sample count, not a boolean flag
    window->AddRenderer(renderer);

    PrismScene scene;
    scene.Build(renderer, window);
    scene.Run();
    scene.Shutdown();

    return 0;
}
