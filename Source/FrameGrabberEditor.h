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

#ifndef __FRAMEGRABBEREDITOR_H__
#define __FRAMEGRABBEREDITOR_H__

#include "FrameGrabber.h"
#include <VisualizerEditorHeaders.h>

class FrameGrabberCanvas;

/**

  User interface for the FrameGrabber plugin

  @see FrameGrabber

*/

class FrameGrabberEditor : public VisualizerEditor,
                           public ComboBox::Listener,
                           public Label::Listener,
                           public Button::Listener,
                           public Timer

{
public:
    FrameGrabberEditor (GenericProcessor* parentNode);
    virtual ~FrameGrabberEditor();

    /** Called when editor is collapsed */
    void collapsedStateChanged() override { /*TODO*/ };

    /** Respond to combo box changes*/
    void comboBoxChanged (ComboBox*);

    /** Respond to button presses */
    void buttonClicked (Button* button) override;

    /** Save editor parameters (e.g. sync settings)*/
    void saveVisualizerEditorParameters (XmlElement*) override { /*TODO*/ };

    /** Load editor parameters (e.g. sync settings)*/
    void loadVisualizerEditorParameters (XmlElement*) override { /*TODO*/ };

    /** Called just prior to the start of acquisition, to allow custom commands. */
    void startAcquisition() override { /*TODO*/ };

    /** Called after the end of acquisition, to allow custom commands .*/
    void stopAcquisition() override { /*TODO*/ };

    /** Creates the Neuropixels settings interface*/
    Visualizer* createNewCanvas (void);

    /** Initializes the probes in a background thread */
    void initialize (bool signalChainIsLoading);

    /** Update settings */
    void update();

    void updateSettings();
    void updateDevices();

    void labelTextChanged (juce::Label*);
    void timerCallback();

    void setCameraViewportSize (int width, int height);

private:
    FrameGrabberCanvas* canvas;

    ScopedPointer<ComboBox> videoSourceCombo;
    ScopedPointer<Label> videoSourceLabel;
    ScopedPointer<ComboBox> streamSourceCombo;
    ScopedPointer<Label> streamSourceLabel;
    ScopedPointer<ComboBox> qualityCombo;
    ScopedPointer<Label> qualityLabel;
    ScopedPointer<ComboBox> colorCombo;
    ScopedPointer<Label> colorLabel;
    ScopedPointer<ComboBox> writeModeCombo;
    ScopedPointer<Label> writeModeLabel;
    ScopedPointer<Label> fpsLabel;
    ScopedPointer<UtilityButton> refreshButton;
    ScopedPointer<UtilityButton> resetCounterButton;
    ScopedPointer<Label> dirNameEdit;

    juce::int64 lastFrameCount;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrameGrabberEditor);
};

#endif // __FRAMEGRABBEREDITOR_H__
