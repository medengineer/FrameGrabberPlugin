/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2024 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "FrameGrabberCanvas.h"

#include "FrameGrabber.h"
#include "FrameGrabberEditor.h"

FrameGrabberCanvas::FrameGrabberCanvas (FrameGrabber* thread_, FrameGrabberEditor* editor_) : thread (thread_),
                                                                                              editor (editor_)
{
    cameraViewport = std::make_unique<Viewport>();

    cameraView = thread->cameraDevice->createViewerComponent();
    cameraView->setBounds (0, 0, 640, 480);

    cameraViewport->setViewedComponent (cameraView, false);
    addAndMakeVisible (cameraViewport.get());

    resized();
}

FrameGrabberCanvas::~FrameGrabberCanvas()
{
    delete cameraView;
}

void FrameGrabberCanvas::paint (Graphics& g)
{
    g.fillAll (Colours::darkgrey);
}

void FrameGrabberCanvas::refresh()
{
    repaint();
}

void FrameGrabberCanvas::refreshState()
{
    resized();
}

void FrameGrabberCanvas::update()
{
}

void FrameGrabberCanvas::resized()
{
    cameraViewport->setBounds (10, 10, getWidth() - 10, getHeight() - 10);

    cameraView->setBounds (0, 0, cameraViewport->getWidth(), cameraViewport->getHeight());

    // why is this not working?
    cameraViewport->setScrollBarsShown (true, true, true, true);
    cameraViewport->setScrollBarThickness (10);
}

void FrameGrabberCanvas::startAcquisition()
{
}

void FrameGrabberCanvas::stopAcquisition()
{
}

void FrameGrabberCanvas::saveCustomParametersToXml (XmlElement* xml)
{
}

void FrameGrabberCanvas::loadCustomParametersFromXml (XmlElement* xml)
{
}