/*
 * Copyright (c) 2010,2026  Kamil Prusko
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

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

#include <vtkActor.h>
#include <vtkAssemblyPath.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkCylinderSource.h>
#include <vtkFollower.h>
#include <vtkInteractorStyleTerrain.h>
#include <vtkLight.h>
#include <vtkLineWidget.h>
#include <vtkObjectFactory.h>
#include <vtkMapper.h>
#include <vtkOBJReader.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTubeFilter.h>
#include <vtkVectorText.h>

#include "main.h"
#include "config.h"
#include "materials.h"

/**
 * vtkLineWidget normally translates the whole line on left-click along the line body (MovingLine).
 * We route left-drag orbit to the terrain style instead: only endpoint handles use left; line-body
 * translation matches VTK's middle-button path (see OnMiddleButtonDown).
 *
 * Right mouse is not forwarded — VTK would scale line length; TorchInteractorStyleTerrain uses
 * right-drag for camera dolly (zoom).
 *
 * vtkLineWidget::ProcessEvents is non-virtual and points at vtkLineWidget::ProcessEvents, so we
 * replace the callback in this subclass ctor (same pattern VTK uses internally).
 */
class TorchLineWidget : public vtkLineWidget
{
public:
  static TorchLineWidget *New ();
  vtkTypeMacro (TorchLineWidget, vtkLineWidget);

protected:
  TorchLineWidget ();
  ~TorchLineWidget () override = default;

  static void ProcessEvents (vtkObject *, unsigned long event, void *clientdata, void *);

private:
  void OnLeftButtonIgnoreLineBody ();
};

vtkStandardNewMacro (TorchLineWidget);

TorchLineWidget::TorchLineWidget ()
{
    this->EventCallbackCommand->SetCallback (TorchLineWidget::ProcessEvents);
}

void TorchLineWidget::ProcessEvents (vtkObject *, unsigned long event, void *clientdata,
                                     void *)
{
    auto *self = static_cast<TorchLineWidget *> (clientdata);

    switch (event)
    {
    case vtkCommand::LeftButtonPressEvent:
        self->OnLeftButtonIgnoreLineBody ();
        break;
    case vtkCommand::LeftButtonReleaseEvent:
        self->vtkLineWidget::OnLeftButtonUp ();
        break;
    case vtkCommand::MiddleButtonPressEvent:
        self->vtkLineWidget::OnMiddleButtonDown ();
        break;
    case vtkCommand::MiddleButtonReleaseEvent:
        self->vtkLineWidget::OnMiddleButtonUp ();
        break;
    case vtkCommand::RightButtonPressEvent:
    case vtkCommand::RightButtonReleaseEvent:
        /* Leave right-click to vtkInteractorStyleTerrain (dolly); skip vtkLineWidget scaling. */
        break;
    case vtkCommand::MouseMoveEvent:
        self->vtkLineWidget::OnMouseMove ();
        break;
    default:
        break;
    }
}

void TorchLineWidget::OnLeftButtonIgnoreLineBody ()
{
    int X = this->Interactor->GetEventPosition ()[0];
    int Y = this->Interactor->GetEventPosition ()[1];

    if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport (X, Y))
    {
        this->State = vtkLineWidget::Outside;
        return;
    }

    vtkAssemblyPath *path = this->GetAssemblyPath (X, Y, 0., this->HandlePicker);

    if (path != nullptr)
    {
        this->vtkLineWidget::OnLeftButtonDown ();
        return;
    }

    path = this->GetAssemblyPath (X, Y, 0., this->LinePicker);
    if (path != nullptr)
    {
        /* Left on shaft only — defer translation to middle mouse (Superclass::OnMiddleButton*). */
        this->State = vtkLineWidget::Outside;
        return;
    }

    this->State = vtkLineWidget::Outside;
    this->HighlightHandle (nullptr);
}

/**
 * Terrain camera uses priority below the torch line widget so handles win left-click before orbit.
 * Middle-button camera pan is disabled so vtkLineWidget keeps MiddleButton* for line translation.
 * Left button down clears stale vtkInteractorStyle State so StartRotate() is not skipped.
 * After superclass MouseMove (runs after the widget because style priority is lower), refresh
 * torch + beams while widget_dragging — vtkLineWidget sets AbortFlag on MouseMove which can
 * block separate vtkRenderWindowInteractor observers for the same event.
 */
class TorchInteractorStyle : public vtkInteractorStyleTerrain
{
public:
  static TorchInteractorStyle *New ();
  vtkTypeMacro (TorchInteractorStyle, vtkInteractorStyleTerrain);

  void SetTorchCallback (TorchMovedCallback *cb) { this->torch_cb = cb; }
  void SetLineWidget (vtkLineWidget *w) { this->line_w = w; }

protected:
  TorchInteractorStyle () = default;
  ~TorchInteractorStyle () override = default;

  void OnLeftButtonDown () override;
  void OnMiddleButtonDown () override {}
  void OnMiddleButtonUp () override {}
  void OnMouseMove () override;

private:
  TorchMovedCallback *torch_cb = nullptr;
  vtkLineWidget     *line_w = nullptr;
};

vtkStandardNewMacro (TorchInteractorStyle);

void TorchInteractorStyle::OnLeftButtonDown ()
{
    if (this->State != VTKIS_NONE)
    {
        this->StopState ();
    }
    this->vtkInteractorStyleTerrain::OnLeftButtonDown ();
}

void TorchInteractorStyle::OnMouseMove ()
{
    this->vtkInteractorStyleTerrain::OnMouseMove ();
    if (this->torch_cb && this->line_w && this->torch_cb->is_widget_dragging ())
    {
        this->torch_cb->sync_torch_from_widget (this->line_w);
    }
}

namespace {

/* Cell ids in data/objects/prism.obj — per-face channel splitters for this scene. */
constexpr vtkIdType kBlueSensorFaceA = 138;
constexpr vtkIdType kBlueSensorFaceB = 139;
constexpr vtkIdType kRgSplitterFaceA = 26;
constexpr vtkIdType kRgSplitterFaceB = 27;

static void
prism_apply_sensor_faces (RayHitColorContext &ctx)
{
    const vtkIdType p0 = ctx.primary_cell_id;
    const vtkIdType *ids = ctx.related_cell_ids;

    const bool blue_split_face =
        (p0 == kBlueSensorFaceA || p0 == kBlueSensorFaceB);
    const bool rg_split_face =
        (p0 == kRgSplitterFaceA || p0 == kRgSplitterFaceB)
        || (ctx.related_cell_ids_capacity >= 2
            && (ids[1] == kRgSplitterFaceA || ids[1] == kRgSplitterFaceB));

    if (blue_split_face)
    {
        ctx.refracted_rgba[2] = 0.0;
        ctx.reflected_rgba[0] = ctx.reflected_rgba[1] = 0.0;
        ctx.refracted_rgba[3] = ctx.reflected_rgba[3] = ctx.incoming_rgba[3];
    }

    if (rg_split_face)
    {
        ctx.refracted_rgba[0] = ctx.refracted_rgba[2] = 0.0;
        ctx.reflected_rgba[1] = ctx.reflected_rgba[2] = 0.0;
        ctx.refracted_rgba[3] = ctx.reflected_rgba[3] = ctx.incoming_rgba[3];
    }
}

} // namespace

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
    vtkOpenGLPolyDataMapper *prism_mapper;

    prism_source = vtkOBJReader::New();
    prism_source->SetFileName (PACKAGE_DATADIR "/objects/prism.obj");

    prism_mapper = vtkOpenGLPolyDataMapper::New();
    prism_mapper->SetInputConnection (prism_source->GetOutputPort ());
    /* Ray tracing runs from TorchMovedCallback::Execute before the first Render(); lazy VTK
       pipelines would otherwise leave the mapper input empty on that first cast — no hits,
       no beams until something (e.g. moving the torch) triggered another update. */
    prism_mapper->Update ();

    prism = vtkActor::New();
    prism->SetMapper (prism_mapper);
    prism_apply_material (prism);
    /* Custom shader sets alpha in gl_FragData; still force translucent pass so VTK does not
       classify the prop as opaque in some views (which caused opaque vs transparent flicker). */
    prism->ForceTranslucentOn ();

    renderer->AddActor (prism);
    raytracer->set_trace_surface_mapper (prism_mapper);
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
    tubes->SetInputData (beam_data);
    tubes->SetRadius (0.5);
    tubes->SetNumberOfSides (12);

    beam_mapper = vtkPolyDataMapper::New();
    beam_mapper->SetInputConnection (tubes->GetOutputPort());
    beam_mapper->SetColorModeToDirectScalars ();   // RGBA unsigned-char cell data → used as-is
    beam_mapper->SetScalarModeToUseCellData ();    // one colour per line segment
    /* Shift tube polygons slightly toward the viewer so they do not share the exact same depth
       as nearby prism faces (reduces z-fighting when blending translucent beam + glass). */
    beam_mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters (-6.0, -6.0);

    beam = vtkActor::New();
    beam->SetMapper (beam_mapper);
    beam_apply_material (beam);
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
    torch_source->SetResolution (20);

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
    torch_apply_actor_material (torch->GetProperty());

    renderer->AddActor (torch);
    callback->set_torch_transform (torch_transform);
    return torch;
}

vtkObject *renderer_add_torch_widget (vtkRenderer               *renderer,
                                      vtkRenderWindowInteractor *window_interactor,
                                      TorchMovedCallback        *callback)
{
    vtkLineWidget *widget;

    widget = TorchLineWidget::New ();
    /* Above TorchInteractorStyle (0.55): handle picks abort before orbit; line-shaft left passes
       through (TorchLineWidget) so middle still translates the torch line. */
    widget->SetPriority (0.65f);
    widget->SetInteractor (window_interactor);
    torch_apply_handle_material (widget->GetHandleProperty (),
                                 widget->GetSelectedHandleProperty ());
    torch_apply_line_material (widget->GetLineProperty (),
                               widget->GetSelectedLineProperty ());
    widget->ClampToBoundsOn();
    widget->PlaceWidget (
                -90.0, -22.0, // xmin, xmax
                -50.0,  50.0, // ymin, ymax
                  5.0,  50.0  // zmin, zmax
                );
    widget->SetPoint1 (-80.0, 0.0, 22.0);
    widget->SetPoint2 (-30.0, 0.0, 20.0);
    widget->AddObserver (vtkCommand::InteractionEvent, callback);
    callback->attach_for_live_drag (widget, window_interactor);
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
    label_apply_material (text_actor->GetProperty ());
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
    label_apply_material (text_actor->GetProperty ());
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
    label_apply_material (text_actor->GetProperty ());
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
    label_apply_material (text_actor->GetProperty ());
    text_actor->SetScale (1.5, 1.5, 1.5);
    text_actor->SetPosition (-5.0, -40.0, 10.0);
    text_actor->SetCamera (renderer->GetActiveCamera());
    renderer->AddActor (text_actor);
}

void renderer_add_ground (vtkRenderer *renderer)
{
    vtkPlaneSource   *plane_source;
    vtkPolyDataMapper *plane_mapper;
    vtkActor         *plane;

    // Create a textured plane
    plane_source = vtkPlaneSource::New();
    plane_source->SetXResolution (1);
    plane_source->SetYResolution (1);
    plane_source->SetOrigin (-300.0, -300.0, -0.05);
    plane_source->SetPoint1 ( 300.0, -300.0, -0.05);
    plane_source->SetPoint2 (-300.0,  300.0, -0.05);

    plane_mapper = vtkPolyDataMapper::New();
    plane_mapper->SetInputConnection (plane_source->GetOutputPort ());
    plane_mapper->Update ();

    plane = vtkActor::New();
    plane->SetMapper (plane_mapper);
    ground_apply_material (plane);
    renderer->AddActor (plane);

    // Create a wide plane to cover the beam
    plane_source = vtkPlaneSource::New();
    plane_source->SetXResolution (1);
    plane_source->SetYResolution (1);
    plane_source->SetOrigin (-10000.0, -10000.0, -0.2);
    plane_source->SetPoint1 ( 10000.0, -10000.0, -0.2);
    plane_source->SetPoint2 (-10000.0,  10000.0, -0.2);

    plane_mapper = vtkPolyDataMapper::New();
    plane_mapper->SetInputConnection (plane_source->GetOutputPort ());
    plane_mapper->Update ();

    plane = vtkActor::New();
    plane->SetMapper (plane_mapper);
    plane->GetProperty()->SetAmbient (1.0);
    plane->GetProperty()->SetAmbientColor (0.0, 0.0, 0.0);
    plane->GetProperty()->SetDiffuse (0.0);
    plane->GetProperty()->SetSpecular (0.0);
    ground_plane_one_sided (plane->GetProperty ());
    renderer->AddActor (plane);
}

int main()
{
    vtkRenderer               *renderer;
    vtkRenderWindow           *window;
    vtkRenderWindowInteractor *window_interactor;
    TorchInteractorStyle      *window_interactor_style;
    RayTracer                 *raytracer;
    TorchMovedCallback        *torch_moved_callback;
    vtkObject                 *torch_widget;

    renderer = vtkRenderer::New();
    renderer->SetBackground (0.0, 0.0, 0.0);
    /* Depth peeling sorts translucent props into fixed peel layers; RGBA beam tubes + RGBA prism
       fighting for those layers caused beams to disappear or punch holes when the torch moved.
       Classical alpha blending uses approximate back-to-front sorting — good enough here and
       keeps beams visible through the glass. */
    renderer->SetUseDepthPeeling (0);

    /* Relative polygon offsets need this global mode; net offset stays 0 except where mappers
       set their own relative values (beam tubes). */
    vtkMapper::SetResolveCoincidentTopologyToPolygonOffset ();
    vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters (0.0, 0.0);
    vtkMapper::SetResolveCoincidentTopologyLineOffsetParameters (0.0, 0.0);

    window = vtkRenderWindow::New();
    window->SetSize (1280, 720);
    window->SetAlphaBitPlanes (1);
    window->SetMultiSamples (0);   /* MSAA + layered transparency is driver‑sensitive; keep off */
    window->AddRenderer (renderer);

    window_interactor = vtkRenderWindowInteractor::New();
    window_interactor->SetRenderWindow (window);

    window_interactor_style = TorchInteractorStyle::New ();
    window_interactor_style->SetPriority (0.55f);
    window_interactor->SetInteractorStyle (window_interactor_style);

    raytracer = new RayTracer ();
    raytracer->set_refract_color_callback (prism_apply_sensor_faces);
    raytracer->set_reflect_color_callback (prism_apply_sensor_faces);

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
    window_interactor_style->SetTorchCallback (torch_moved_callback);
    window_interactor_style->SetLineWidget (
        vtkLineWidget::SafeDownCast (torch_widget));

    /* Match first interactive render: terrain style calls ResetCameraClippingRange on motion; do it
       once here so the opening frame uses correct near/far (avoids a dark / clipped ground until
       the camera moves). */
    renderer->ResetCameraClippingRange ();

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
