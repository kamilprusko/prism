/*
 * Copyright (c) 2010,2026  Kamil Prusko
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TORCH_WIDGET_H
#define TORCH_WIDGET_H

#include <vtkLineWidget.h>
#include <vtkInteractorStyleTerrain.h>

class TorchMovedCallback;

/**
 * vtkLineWidget normally translates the whole line on left-click along the line body.
 * Left on the shaft is ignored so orbit uses left-drag; middle mouse translates the line.
 */
class TorchLineWidget : public vtkLineWidget
{
  public:
    static TorchLineWidget *New();
    vtkTypeMacro(TorchLineWidget, vtkLineWidget);
    void PrintSelf(ostream &os, vtkIndent indent) override;

  protected:
    TorchLineWidget();
    ~TorchLineWidget() override = default;

    static void ProcessEvents(vtkObject *, unsigned long event, void *clientdata, void *);

  private:
    void onLeftButtonIgnoreLineBody();
};

/**
 * Terrain style with lower priority than the torch widget; refreshes ray trace while dragging.
 */
class TorchInteractorStyle : public vtkInteractorStyleTerrain
{
  public:
    static TorchInteractorStyle *New();
    vtkTypeMacro(TorchInteractorStyle, vtkInteractorStyleTerrain);

    void SetTorchCallback(TorchMovedCallback *cb);
    void SetLineWidget(vtkLineWidget *w);
    void PrintSelf(ostream &os, vtkIndent indent) override;

  protected:
    TorchInteractorStyle() = default;
    ~TorchInteractorStyle() override = default;

    void OnLeftButtonDown() override;
    void OnMiddleButtonDown() override {}
    void OnMiddleButtonUp() override {}
    void OnMouseMove() override;

  private:
    TorchMovedCallback *torchCallback = nullptr;
    vtkLineWidget *lineWidget = nullptr;
};

#endif
