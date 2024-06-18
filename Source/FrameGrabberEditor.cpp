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

#include "FrameGrabberEditor.h"
#include "FrameGrabberCanvas.h"

#include <stdio.h>

FrameGrabberEditor::FrameGrabberEditor (GenericProcessor* parentNode)
    : VisualizerEditor (parentNode, "FrameGrabber"), lastFrameCount (0)

{
    desiredWidth = 280;

    FrameGrabber* thread = (FrameGrabber*) parentNode;

    addComboBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "video_source", 10, 29);
    addSelectedStreamParameterEditor (Parameter::PROCESSOR_SCOPE, "stream_source", 10, 54);
    addComboBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "image_quality", 10, 79);
    addPathParameterEditor (Parameter::PROCESSOR_SCOPE, "directory_name", 10, 104);

    for (auto& p : { "video_source", "stream_source", "image_quality", "directory_name" })
    {
        auto* ed = getParameterEditor (p);
        ed->setBounds (ed->getX(), ed->getY(), desiredWidth, ed->getHeight());
    }

    checkForCanvas();
}

FrameGrabberEditor::~FrameGrabberEditor()
{
}

Visualizer* FrameGrabberEditor::createNewCanvas (void)
{
    FrameGrabber* thread = (FrameGrabber*) getProcessor();
    canvas = new FrameGrabberCanvas (thread, this);
    return canvas;
}

void FrameGrabberEditor::setCameraViewportSize (int width, int height)
{
    if (canvas != nullptr)
    {
        canvas->cameraViewport->setBounds (0, 0, width, height);
        canvas->cameraView->setBounds (0, 0, width, height);
    }
}

void FrameGrabberEditor::updateSettings()
{
}

void FrameGrabberEditor::comboBoxChanged (ComboBox* cb)
{
    FrameGrabber* thread = (FrameGrabber*) getProcessor();

    if (cb == qualityCombo)
    {
        int index = cb->getSelectedItemIndex();
        thread->setImageQuality (5 * (index));
    }
    else if (cb == colorCombo)
    {
        int index = cb->getSelectedItemIndex();
        thread->setColorMode (index);
    }
    else if (cb == videoSourceCombo)
    {
        int index = cb->getSelectedItemIndex();
        if (thread->isCameraRunning())
        {
            thread->stopCamera();
        }
        thread->startCamera (index);
    }
    else if (cb == streamSourceCombo)
    {
        int index = cb->getSelectedItemIndex();
        thread->setCurrentStreamIndex (index);
    }
    else if (cb == writeModeCombo)
    {
        int index = cb->getSelectedItemIndex();
        thread->setWriteMode (index);
    }
}

void FrameGrabberEditor::buttonClicked (Button* button)
{
    FrameGrabber* thread = (FrameGrabber*) getProcessor();

    if (button == refreshButton)
    {
        updateDevices();
    }
    else if (button == resetCounterButton)
    {
        UtilityButton* btn = (UtilityButton*) button;
        bool state = btn->getToggleState();
        thread->setResetFrameCounter (state);
    }
}

void FrameGrabberEditor::labelTextChanged (juce::Label* label)
{
    FrameGrabber* thread = (FrameGrabber*) getProcessor();

    if (label == dirNameEdit)
    {
        String name = label->getTextValue().getValue();

        thread->setDirectoryName (name);
    }
}

void FrameGrabberEditor::updateDevices()
{
    /*
	FrameGrabber* thread = (FrameGrabber*) getProcessor();

	videoSourceCombo->clear(dontSendNotification);
	std::vector<std::string> formats = thread->getFormats();
	for (unsigned int i=0; i<formats.size(); i++)
	    videoSourceCombo->addItem(formats.at(i), i+1);
	videoSourceCombo->setSelectedItemIndex(thread->getCurrentFormatIndex(), dontSendNotification);
	*/
}

void FrameGrabberEditor::timerCallback()
{
    FrameGrabber* thread = (FrameGrabber*) getProcessor();

    int64 frameCount = thread->getFrameCount();
    int64 fps = frameCount - lastFrameCount;
    lastFrameCount = frameCount;

    //fpsLabel->setText(String("FPS: ") + String(fps), dontSendNotification);
}