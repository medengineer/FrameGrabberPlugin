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

#include "FrameGrabber.h"
#include <VisualizerEditorHeaders.h>

class FrameGrabberEditor;

/** 

	Holds the visualizer for additional probe settings

*/
class FrameGrabberCanvas : public Visualizer
{
public:
    /** Constructor */
    FrameGrabberCanvas (FrameGrabber*, FrameGrabberEditor*);

    /** Destructor */
    ~FrameGrabberCanvas();

    /** Fills background */
    void paint (Graphics& g);

    /** Renders the Visualizer on each animation callback cycle */
    void refresh();

    /** Called when the Visualizer's tab becomes visible after being hidden */
    void refreshState();

    /** Called when the Visualizer is first created, and optionally when
		the parameters of the underlying processor are changed */
    void update();

    /** Starts animation of sub-interfaces */
    void startAcquisition();

    /** Stops animation of sub-interfaces */
    void stopAcquisition();

    /** Saves custom UI settings */
    void saveCustomParametersToXml (XmlElement* xml) override;

    /** Loads custom UI settings*/
    void loadCustomParametersFromXml (XmlElement* xml) override;

    /** Sets bounds of sub-components*/
    void resized();

    std::unique_ptr<Viewport> cameraViewport;

    Component* cameraView;

    FrameGrabberEditor* editor;
    FrameGrabber* thread;

    GenericProcessor* processor;
};