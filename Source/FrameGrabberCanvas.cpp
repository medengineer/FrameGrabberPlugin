/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright(C) 2023 Allen Institute for Brain Science and Open Ephys

------------------------------------------------------------------

This program is free software : you can redistribute itand /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < http://www.gnu.org/licenses/>.

*/

#include "FrameGrabberCanvas.h"

#include "FrameGrabberEditor.h"
#include "FrameGrabber.h"

WebcamComponent::WebcamComponent()
{

    //Create a webcam component
	CameraDevice* cameraDevice;
	std::unique_ptr<Component> cameraPreviewComp;

    int numDevices = CameraDevice::getAvailableDevices().size();

    if (numDevices > 0)
    {
        camera.reset(CameraDevice::openDevice(0));
        if (camera)
            camera->addListener(this);
    }
}

WebcamComponent::~WebcamComponent()
{
    if (camera)
        camera->removeListener(this);
}

void WebcamComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    Rectangle<int> bounds = getLocalBounds();

    g.drawImageWithin(
        webcamImage, 
        bounds.getX(), 
        bounds.getY(),
        bounds.getWidth(),
        bounds.getHeight(),
        juce::RectanglePlacement::stretchToFit, false);
}

void WebcamComponent::resized()
{
}

void WebcamComponent::imageReceived(const juce::Image& image)
{
    webcamImage = image;
    repaint();
}


FrameGrabberCanvas::FrameGrabberCanvas(GenericProcessor* processor_, FrameGrabberEditor* editor_, FrameGrabber* thread_) : 
    processor(processor_),
    editor(editor_),
    thread(thread_)

{
    cameraViewport = new Viewport();

    webcamComponent = std::make_unique<WebcamComponent>();

    webcamComponent->setBounds(100, 100, 640, 480);
    webcamComponent->setVisible(true);

    cameraViewport->setViewedComponent(webcamComponent.get(), false);
    addAndMakeVisible(cameraViewport);

    resized();

}

FrameGrabberCanvas::~FrameGrabberCanvas()
{

}

void FrameGrabberCanvas::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
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
    cameraViewport->setBounds(10, 10, getWidth()-10, getHeight()-10);

    // why is this not working?
    cameraViewport->setScrollBarsShown(true, true, true, true);
    cameraViewport->setScrollBarThickness(10);
}

void FrameGrabberCanvas::startAcquisition()
{
}

void FrameGrabberCanvas::stopAcquisition()
{
}

void FrameGrabberCanvas::saveCustomParametersToXml(XmlElement* xml)
{
}

void FrameGrabberCanvas::loadCustomParametersFromXml(XmlElement* xml)
{
}