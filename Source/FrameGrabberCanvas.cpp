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

CameraView::CameraView(FrameGrabber* thread_) : thread(thread_)
{
    if (thread->hasCameraDevice)
        thread->cameraDevice->addListener(this);
}

CameraView::~CameraView()
{
}

void CameraView::paint(juce::Graphics& g)
{

    g.fillAll(juce::Colours::black);

    Rectangle<int> bounds = getLocalBounds();

    g.drawImageWithin(
        cameraImage,
        bounds.getX(), 
        bounds.getY(),
        bounds.getWidth(),
        bounds.getHeight(),
        juce::RectanglePlacement::stretchToFit, false);
}

void CameraView::resized()
{
}

void CameraView::imageReceived(const juce::Image& image)
{
    //TODO: This hits an assert in debug mode on GUI quit.
    // Need to stop the camera device before the GUI quits.
    MessageManagerLock mml;

    if (thread->getColorMode() == 0)
    {
        // Generate a grayscale image
        Image grayscaleImage = Image(Image::PixelFormat::RGB, image.getWidth(), image.getHeight(), true);
        for (int w = 0; w < image.getWidth(); w++)
        {
            for (int h = 0; h < image.getHeight(); h++)
            {
                auto pixel = image.getPixelAt(w, h);
                auto gray = (pixel.getRed() + pixel.getGreen() + pixel.getBlue()) / 3.0f;
                grayscaleImage.setPixelAt(w, h, Colour(gray, gray, gray));
            }
        }
        cameraImage = grayscaleImage;
    }
    else
    {
        cameraImage = image;
    }
    repaint();
}


FrameGrabberCanvas::FrameGrabberCanvas(FrameGrabber* thread_, FrameGrabberEditor* editor_) : 
    thread(thread_),
    editor(editor_)
{
    cameraViewport = new Viewport();

    cameraView = std::make_unique<CameraView>(thread);

    cameraView->setBounds(100, 100, 640, 480);
    cameraView->setVisible(true);

    cameraViewport->setViewedComponent(cameraView.get(), false);
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