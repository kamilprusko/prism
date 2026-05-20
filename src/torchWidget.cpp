/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <vtkAbstractPropPicker.h>
#include <vtkAssemblyPath.h>
#include <vtkCallbackCommand.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkIndent.h>
#include <vtkLineWidget.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

#include "torchMovedCallback.h"
#include "torchWidget.h"

vtkStandardNewMacro(TorchLineWidget);
vtkStandardNewMacro(TorchInteractorStyle);

TorchLineWidget::TorchLineWidget()
{
    this->EventCallbackCommand->SetCallback(TorchLineWidget::ProcessEvents);
}

void TorchLineWidget::onLeftButtonIgnoreLineBody()
{
    const int X = this->Interactor->GetEventPosition()[0];
    const int Y = this->Interactor->GetEventPosition()[1];

    if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y)) {
        this->State = vtkLineWidget::Outside;
        return;
    }

    vtkAssemblyPath *path =
        this->GetAssemblyPath(X, Y, 0., static_cast<vtkAbstractPropPicker *>(this->HandlePicker));

    if (path != nullptr) {
        this->vtkLineWidget::OnLeftButtonDown();
        return;
    }

    path = this->GetAssemblyPath(X, Y, 0., static_cast<vtkAbstractPropPicker *>(this->LinePicker));
    if (path != nullptr) {
        this->State = vtkLineWidget::Outside;
        return;
    }

    this->State = vtkLineWidget::Outside;
    this->HighlightHandle(nullptr);
}

void TorchLineWidget::ProcessEvents(vtkObject *, unsigned long event, void *clientdata, void *)
{
    auto *self = static_cast<TorchLineWidget *>(clientdata);

    switch (event) {
    case vtkCommand::LeftButtonPressEvent:
        self->onLeftButtonIgnoreLineBody();
        break;

    case vtkCommand::LeftButtonReleaseEvent:
        self->vtkLineWidget::OnLeftButtonUp();
        break;

    case vtkCommand::MiddleButtonPressEvent:
        self->vtkLineWidget::OnMiddleButtonDown();
        break;

    case vtkCommand::MiddleButtonReleaseEvent:
        self->vtkLineWidget::OnMiddleButtonUp();
        break;

    case vtkCommand::RightButtonPressEvent:
    case vtkCommand::RightButtonReleaseEvent:
        break;

    case vtkCommand::MouseMoveEvent:
        self->vtkLineWidget::OnMouseMove();
        break;

    default:
        break;
    }
}

void TorchLineWidget::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
}

void TorchInteractorStyle::SetTorchCallback(TorchMovedCallback *cb)
{
    this->torchCallback = cb;
}

void TorchInteractorStyle::SetLineWidget(vtkLineWidget *w)
{
    this->lineWidget = w;
}

void TorchInteractorStyle::OnLeftButtonDown()
{
    if (this->State != VTKIS_NONE)
        this->StopState();

    this->vtkInteractorStyleTerrain::OnLeftButtonDown();
}

void TorchInteractorStyle::OnMouseMove()
{
    this->vtkInteractorStyleTerrain::OnMouseMove();

    if (this->torchCallback && this->lineWidget && this->torchCallback->IsWidgetDragging())
        this->torchCallback->SyncTorchFromWidget(this->lineWidget);
}

void TorchInteractorStyle::PrintSelf(ostream &os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "torchCallback: " << this->torchCallback << "\n";
    os << indent << "lineWidget: " << this->lineWidget << "\n";
}
