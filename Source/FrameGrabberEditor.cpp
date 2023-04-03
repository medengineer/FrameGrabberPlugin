/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

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


FrameGrabberEditor::FrameGrabberEditor(GenericProcessor* parentNode)
    : VisualizerEditor(parentNode, "FrameGrabber"), lastFrameCount(0)

{
    desiredWidth = 350;

	FrameGrabber* thread = (FrameGrabber*) parentNode;

	sourceLabel = new Label("video source", "Source");
	sourceLabel->setBounds(10,25,90,20);
    sourceLabel->setFont(Font("Small Text", 12, Font::plain));
    sourceLabel->setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(sourceLabel);

    sourceCombo = new ComboBox();
    sourceCombo->setBounds(110,25,220,20);
    sourceCombo->addListener(this);

	// Populate devices and select the first available device
	std::vector<std::string> formats = thread->getFormats();
    for (unsigned int i=0; i<formats.size(); i++)
        sourceCombo->addItem(formats[i], i+1);
	addAndMakeVisible(sourceCombo);

	qualityLabel = new Label("image quality label", "Image quality");
	qualityLabel->setBounds(10,50,90,20);
    qualityLabel->setFont(Font("Small Text", 12, Font::plain));
    qualityLabel->setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(qualityLabel);

    qualityCombo = new ComboBox();
    qualityCombo->setBounds(110,50-3,75,20);
    qualityCombo->addListener(this);

    for (unsigned int i=0; i<=100; i+=5)
		qualityCombo->addItem(String(i)+"%", i/5);
	addAndMakeVisible(qualityCombo);

	colorLabel = new Label("color mode label", "Color");
	colorLabel->setBounds(10,75,50,20);
    colorLabel->setFont(Font("Small Text", 12, Font::plain));
    colorLabel->setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(colorLabel);

    colorCombo = new ComboBox();
    colorCombo->setBounds(110,75,75,20);
    colorCombo->addListener(this);
    colorCombo->addItem("Gray", 1);
	colorCombo->addItem("RGB", 2);
	colorCombo->setSelectedItemIndex(thread->getColorMode(), dontSendNotification);
	addAndMakeVisible(colorCombo);

	writeModeLabel = new Label("write mode label", "Save frames");
	writeModeLabel->setBounds(10,100,150,20);
    writeModeLabel->setFont(Font("Small Text", 12, Font::plain));
    writeModeLabel->setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(writeModeLabel);

    writeModeCombo = new ComboBox();
    writeModeCombo->setBounds(110,100,75,20);
    writeModeCombo->addListener(this);
	writeModeCombo->addItem("Never", ImageWriteMode::NEVER+1);
    writeModeCombo->addItem("Recording", ImageWriteMode::RECORDING+1);
//	writeModeCombo->addItem("Acquisition", ImageWriteMode::ACQUISITION+1);
	writeModeCombo->setSelectedItemIndex(thread->getWriteMode(), dontSendNotification);
	addAndMakeVisible(writeModeCombo);

	fpsLabel = new Label("fps label", "FPS:");
	fpsLabel->setBounds(200, 50, 50, 20); // 200,100,50,20);
    fpsLabel->setFont(Font("Small Text", 12, Font::plain));
    fpsLabel->setColour(Label::textColourId, Colours::darkgrey);
	addAndMakeVisible(fpsLabel);

    refreshButton = new UtilityButton("Refresh", Font ("Small Text", 12, Font::plain));
    refreshButton->addListener(this);
    refreshButton->setBounds(260, 50, 70, 20);
    addAndMakeVisible(refreshButton);

    resetCounterButton = new UtilityButton("Reset counter",Font("Small Text", 12, Font::plain));
    resetCounterButton->addListener(this);
    resetCounterButton->setBounds(200,75,130,20);
    resetCounterButton->setClickingTogglesState(true);
	resetCounterButton->setToggleState(thread->getResetFrameCounter(), dontSendNotification);
    resetCounterButton->setTooltip("When this button is on, the frame counter will be reset for each new recording");
    addAndMakeVisible(resetCounterButton);

	dirNameEdit = new Label("dirName", thread->getDirectoryName());
    dirNameEdit->setBounds(200,100,130,20);
    dirNameEdit->setFont(Font("Default", 15, Font::plain));
    dirNameEdit->setColour(Label::textColourId, Colours::white);
	dirNameEdit->setColour(Label::backgroundColourId, Colours::grey);
    dirNameEdit->setEditable(true);
    dirNameEdit->addListener(this);
	dirNameEdit->setTooltip("Frame directory name");
	addAndMakeVisible(dirNameEdit);

	//startTimer(1000);  // TODO: update FPS label once per second
}


FrameGrabberEditor::~FrameGrabberEditor()
{
}

Visualizer* FrameGrabberEditor::createNewCanvas(void)
{
	FrameGrabber* thread = (FrameGrabber*) getProcessor();
    canvas = new FrameGrabberCanvas(thread, this);
    return canvas;
}

void FrameGrabberEditor::updateSettings()
{

	FrameGrabber* thread = (FrameGrabber*) getProcessor();

	qualityCombo->setSelectedItemIndex(thread->getImageQuality()/5-1, dontSendNotification);
	colorCombo->setSelectedItemIndex(thread->getColorMode(), dontSendNotification);
	writeModeCombo->setSelectedItemIndex(thread->getWriteMode(), dontSendNotification);

	updateDevices();
	int deviceIndex = thread->getCurrentFormatIndex();
	if (deviceIndex >= 0)
	{
		sourceCombo->setSelectedItemIndex(deviceIndex, sendNotificationAsync);
	}
}

void FrameGrabberEditor::comboBoxChanged(ComboBox* cb)
{

	FrameGrabber* thread = (FrameGrabber*) getProcessor();

    if (cb == qualityCombo)
    {
		int index = cb->getSelectedItemIndex();
		thread->setImageQuality(5*(index+1));
    }
    else if (cb == colorCombo)
    {
		int index = cb->getSelectedItemIndex();
		thread->setColorMode(index);
    }
	else if (cb == sourceCombo)
	{
		int index = cb->getSelectedItemIndex();
		if (thread->isCameraRunning())
		{
			thread->stopCamera();
		}
		thread->startCamera(index);
	}
    else if (cb == writeModeCombo)
    {
		int index = cb->getSelectedItemIndex();
		thread->setWriteMode(index);
    }
}


void FrameGrabberEditor::buttonClicked(Button* button)
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
		thread->setResetFrameCounter(state);
	}
}


void FrameGrabberEditor::labelTextChanged(juce::Label *label)
{
	FrameGrabber* thread = (FrameGrabber*) getProcessor();

	if (label == dirNameEdit)
	{
	    String name = label->getTextValue().getValue();

		thread->setDirectoryName(name);
	}
}


void FrameGrabberEditor::updateDevices()
{
	FrameGrabber* thread = (FrameGrabber*) getProcessor();

	sourceCombo->clear(dontSendNotification);
	std::vector<std::string> formats = thread->getFormats();
	for (unsigned int i=0; i<formats.size(); i++)
	{
	    sourceCombo->addItem(formats.at(i), i+1);
	}
}


void FrameGrabberEditor::timerCallback()
{
	FrameGrabber* thread = (FrameGrabber*) getProcessor(); 

	juce::int64 frameCount = thread->getFrameCount();
	juce::int64 fps = frameCount - lastFrameCount;
	lastFrameCount = frameCount;

	fpsLabel->setText(String("FPS: ") + String(fps), dontSendNotification);
}


void FrameGrabberEditor::disableControls()
{
	FrameGrabber* thread = (FrameGrabber*) getProcessor(); 

	if (thread->getWriteMode() == RECORDING)
	{
		sourceCombo->setEnabled(false);
    	qualityCombo->setEnabled(false);
    	colorCombo->setEnabled(false);
    	writeModeCombo->setEnabled(false);
		refreshButton->setEnabledState(false);
		resetCounterButton->setEnabledState(false);
		dirNameEdit->setEditable(false);
	}
}


void FrameGrabberEditor::enableControls()
{
	FrameGrabber* thread = (FrameGrabber*) getProcessor();
	
	if (thread->getWriteMode() == RECORDING)
	{
		sourceCombo->setEnabled(true);
    	qualityCombo->setEnabled(true);
    	colorCombo->setEnabled(true);
    	writeModeCombo->setEnabled(true);
		refreshButton->setEnabledState(true);
		resetCounterButton->setEnabledState(true);
		dirNameEdit->setEditable(true);
	}
}

