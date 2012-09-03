/*
 * Copyright (c) 2010  Kamil Prusko
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCylinderSource.h"
#include "vtkDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkFollower.h"
#include "vtkInteractorStyleTerrain.h"
#include "vtkLight.h"
#include "vtkLineWidget.h"
#include "vtkOBJReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextActor.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTubeFilter.h"
#include "vtkVectorText.h"

#include "main.h"
#include "config.h"


vtkCamera *renderer_add_camera (vtkRenderer *renderer)
{
    vtkCamera *camera;

    camera = vtkCamera::New();
    camera->SetViewUp (0.0, 0.0, 1.0);
    camera->SetPosition (-120.0, -160.0, 60.0);
    camera->SetFocalPoint (-20.0, 0.0, 10.0);
    camera->SetViewAngle (40.0);
    // camera->Zoom (2.0);
    renderer->SetActiveCamera (camera);
    return camera;
}

void renderer_add_lights (vtkRenderer *renderer)
{
    vtkLight *light;

    light = vtkLight::New();
    light->SetPosition (100.0, 100.0, 100.0);
    light->SetFocalPoint (0.0, 0.0, 20.0);
    renderer->AddLight (light);

    light = vtkLight::New();
    light->SetPosition (-50.0, 0.0, 50.0);
    light->SetFocalPoint (0.0, 0.0, 20.0);
    renderer->AddLight (light);
}

vtkActor *renderer_add_prism (vtkRenderer *renderer,
                              RayTracer *raytracer)
{
    vtkActor          *prism;
    vtkOBJReader      *prism_source;
    vtkPolyDataMapper *prism_mapper;

    prism_source = vtkOBJReader::New();
    prism_source->SetFileName (PACKAGE_DATADIR "/objects/prism.obj");

    prism_mapper = vtkPolyDataMapper::New();
    prism_mapper->SetInputConnection (prism_source->GetOutputPort());

    prism = vtkActor::New();
    prism->SetMapper (prism_mapper);
    prism->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/glass.xml");
    prism->GetProperty()->ShadingOn();

    renderer->AddActor (prism);
    raytracer->set_prism_mapper (prism_mapper);
    return prism;
}

vtkActor *renderer_add_beam (vtkRenderer *renderer,
                             RayTracer *raytracer)
{
    vtkActor          *beam;
    vtkPolyData       *beam_data;
    vtkPolyDataMapper *beam_mapper;
    vtkTubeFilter     *tubes;

    beam_data = vtkPolyData::New();

    tubes = vtkTubeFilter::New();
    tubes->SetInput (beam_data);
    tubes->SetRadius (0.5);
    tubes->SetNumberOfSides (12);

    beam_mapper = vtkPolyDataMapper::New();
    beam_mapper->SetInputConnection (tubes->GetOutputPort());

    beam = vtkActor::New();
    beam->SetMapper (beam_mapper);
    beam->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/beam.xml");
    beam->GetProperty()->ShadingOn();
    beam->GetProperty()->SetLineWidth (4.0);

    renderer->AddActor (beam);
    raytracer->set_beam_data (beam_data);
    return beam;
}

vtkActor *renderer_add_torch (vtkRenderer        *renderer,
                              TorchMovedCallback *callback)
{
    vtkActor                   *torch;
    vtkCylinderSource          *torch_source;
    vtkTransformPolyDataFilter *torch_transform;
    vtkPolyDataMapper          *torch_mapper;

    torch_source = vtkCylinderSource::New();
    torch_source->SetCenter (0.0, 0.0, 0.0);
    torch_source->SetRadius (3.0);
    torch_source->SetHeight (9.0);
    torch_source->SetResolution (20.0);

    vtkTransform *transform = vtkTransform::New();
    transform->Identity();
    transform->RotateZ (90.0);
    transform->RotateX (10.0);
    transform->Translate (0.0, 100.0, 20.0);

    torch_transform = vtkTransformPolyDataFilter::New();
    torch_transform->SetTransform (transform);
    torch_transform->SetInputConnection (torch_source->GetOutputPort());

    torch_mapper = vtkPolyDataMapper::New();
    torch_mapper->SetInputConnection (torch_transform->GetOutputPort());

    torch = vtkActor::New();
    torch->SetMapper (torch_mapper);
    torch->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/torch.xml");

    renderer->AddActor (torch);
    callback->set_torch_transform (torch_transform);
    return torch;
}

vtkObject *renderer_add_torch_widget (vtkRenderer               *renderer,
                                      vtkRenderWindowInteractor *window_interactor,
                                      TorchMovedCallback        *callback)
{
    vtkLineWidget *widget;

    widget = vtkLineWidget::New();
    widget->SetInteractor (window_interactor);
    widget->GetHandleProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/torch.xml");
    widget->GetLineProperty()->SetOpacity (0.01);
    widget->GetSelectedLineProperty()->SetOpacity (0.01);
    widget->ClampToBoundsOn();
    widget->PlaceWidget (
                -90.0, -22.0, // xmin, xmax
                -50.0,  50.0, // ymin, ymax
                  5.0,  50.0  // zmin, zmax
                );
    widget->SetPoint1 (-80.0, 0.0, 22.0);
    widget->SetPoint2 (-30.0, 0.0, 20.0);
    widget->AddObserver (vtkCommand::InteractionEvent, callback);
    widget->On();
    return (vtkObject*) widget;
}

void renderer_add_labels (vtkRenderer *renderer)
{
    vtkVectorText     *text;
    vtkPolyDataMapper *text_mapper;
    vtkFollower       *text_actor;

    // Create scene Label
    text        = vtkVectorText::New();
    text->SetText ("Scene");

    text_mapper = vtkPolyDataMapper::New();
    text_mapper->SetInputConnection (text->GetOutputPort());

    text_actor  = vtkFollower::New();
    text_actor->SetMapper (text_mapper);
    text_actor->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/text.xml");
    text_actor->SetScale (1.5, 1.5, 1.5);
    text_actor->SetPosition (-90.0, 0.0, 10.0);
    text_actor->SetCamera (renderer->GetActiveCamera());
    renderer->AddActor (text_actor);

    // Create red label
    text        = vtkVectorText::New();
    text->SetText ("Sensor for\nGreen Channel");

    text_mapper = vtkPolyDataMapper::New();
    text_mapper->SetInputConnection (text->GetOutputPort());

    text_actor  = vtkFollower::New();
    text_actor->SetMapper (text_mapper);
    text_actor->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/text.xml");
    text_actor->SetScale (1.5, 1.5, 1.5);
    text_actor->SetPosition (50.0, 5.0, 10.0);
    text_actor->SetCamera (renderer->GetActiveCamera());
    renderer->AddActor (text_actor);

    // Create green label
    text        = vtkVectorText::New();
    text->SetText ("Sensor for\nRed Channel");

    text_mapper = vtkPolyDataMapper::New();
    text_mapper->SetInputConnection (text->GetOutputPort());

    text_actor  = vtkFollower::New();
    text_actor->SetMapper (text_mapper);
    text_actor->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/text.xml");
    text_actor->SetScale (1.5, 1.5, 1.5);
    text_actor->SetPosition (-5.0, 45.0, 10.0);
    text_actor->SetCamera (renderer->GetActiveCamera());
    renderer->AddActor (text_actor);

    // Create blue label
    text        = vtkVectorText::New();
    text->SetText ("Sensor for\nBlue Channel");

    text_mapper = vtkPolyDataMapper::New();
    text_mapper->SetInputConnection (text->GetOutputPort());

    text_actor  = vtkFollower::New();
    text_actor->SetMapper (text_mapper);
    text_actor->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/text.xml");
    text_actor->SetScale (1.5, 1.5, 1.5);
    text_actor->SetPosition (-5.0, -40.0, 10.0);
    text_actor->SetCamera (renderer->GetActiveCamera());
    renderer->AddActor (text_actor);
}

void renderer_add_ground (vtkRenderer *renderer)
{
    vtkPlaneSource   *plane_source;
    vtkDataSetMapper *plane_mapper;
    vtkActor         *plane;

    // Create a textured plane
    plane_source = vtkPlaneSource::New();
    plane_source->SetXResolution (1);
    plane_source->SetYResolution (1);
    plane_source->SetOrigin (-300.0, -300.0, -0.05);
    plane_source->SetPoint1 ( 300.0, -300.0, -0.05);
    plane_source->SetPoint2 (-300.0,  300.0, -0.05);

    plane_mapper = vtkDataSetMapper::New();
    plane_mapper->SetInput (plane_source->GetOutput());

    plane = vtkActor::New();
    plane->SetMapper (plane_mapper);
    plane->GetProperty()->LoadMaterial (PACKAGE_DATADIR "/materials/ground.xml");
    renderer->AddActor (plane);

    // Create a wide plane to cover the beam
    plane_source = vtkPlaneSource::New();
    plane_source->SetXResolution (1);
    plane_source->SetYResolution (1);
    plane_source->SetOrigin (-10000.0, -10000.0, -0.2);
    plane_source->SetPoint1 ( 10000.0, -10000.0, -0.2);
    plane_source->SetPoint2 (-10000.0,  10000.0, -0.2);

    plane_mapper = vtkDataSetMapper::New();
    plane_mapper->SetInput (plane_source->GetOutput());

    plane = vtkActor::New();
    plane->SetMapper (plane_mapper);
    plane->GetProperty()->SetAmbient (1.0);
    plane->GetProperty()->SetAmbientColor (0.0, 0.0, 0.0);
    plane->GetProperty()->SetDiffuse (0.0);
    plane->GetProperty()->SetSpecular (0.0);
    renderer->AddActor (plane);
}

int main()
{
    vtkRenderer               *renderer;
    vtkRenderWindow           *window;
    vtkRenderWindowInteractor *window_interactor;
    vtkInteractorStyleTerrain *window_interactor_style;
    RayTracer                 *raytracer;
    TorchMovedCallback        *torch_moved_callback;
    vtkObject                 *torch_widget;

    renderer = vtkRenderer::New();
    renderer->SetBackground (0.0, 0.0, 0.0);

    window = vtkRenderWindow::New();
    window->SetSize (1000, 600);
    window->SetAlphaBitPlanes (1);
    window->SetMultiSamples (4);
    window->AddRenderer (renderer);

    window_interactor = vtkRenderWindowInteractor::New();
    window_interactor->SetRenderWindow (window);

    window_interactor_style = vtkInteractorStyleTerrain::New();
    window_interactor->SetInteractorStyle (window_interactor_style);

    raytracer = new RayTracer ();

    torch_moved_callback = new TorchMovedCallback ();
    torch_moved_callback->set_raytracer (raytracer);

    // Build scene
    renderer_add_camera (renderer);
    renderer_add_lights (renderer);
    renderer_add_labels (renderer);
    renderer_add_ground (renderer);
    renderer_add_prism  (renderer, raytracer);
    renderer_add_beam   (renderer, raytracer);
    renderer_add_torch  (renderer, torch_moved_callback);

    torch_widget = renderer_add_torch_widget (renderer, window_interactor, torch_moved_callback);

    // Run
    window_interactor->Initialize();

    torch_moved_callback->Execute (torch_widget, 0, NULL);

    window->Render();
    window_interactor->Start();

    // Clean up
    window->Delete();
    renderer->Delete();

    return 0;
}
