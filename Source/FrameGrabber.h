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


#ifndef FRAMEGRABBER_H_INCLUDED
#define FRAMEGRABBER_H_INCLUDED

#include <ProcessorHeaders.h>

#include "WriteThread.h"

#include "FrameGrabberEditor.h"

class FrameGrabber : public GenericProcessor, public CameraDevice::Listener
{
public:
    FrameGrabber();
    ~FrameGrabber();

    void registerParameters() override;

    AudioProcessorEditor* createEditor() override;

    String handleConfigMessage (const String& msg) override { return "TODO"; }

    /* Updates the FileReader settings*/
    void updateSettings() override;

	void parameterValueChanged (Parameter* p) override;

    void process (AudioBuffer<float>& buffer);

    void handleTTLEvent(TTLEventPtr event) override;

    void startRecording();
    void stopRecording();

    bool generatesTimestamps() const override { return true; }

    /* Called at start of acquisition */
    bool startAcquisition() override;

    /* Called at end of acquisition */
    bool stopAcquisition() override;

    int startCamera (int fmt_index) { /* TODO */ return 0; }
    int stopCamera() { /* TODO */ return 0; }
    bool isCameraRunning() { /* TODO */ return false; }

    Array<String> getDevices() { return availableDevices; }
    int getCurrentDevice() { return currentDeviceIndex; }
    void setCurrentDevice (int index);

    int getCurrentStreamId() { return currentStreamId; }
    void setCurrentStreamIdFromIndex (int index);

    void setImageQuality (int q);
    int getImageQuality();

    void setColorMode (int value);
    int getColorMode();

    void setWriteMode (int value);
    int getWriteMode();

    void setResetFrameCounter (bool enable);
    bool getResetFrameCounter();

    int64 getFrameCount();

    void setDirectoryName (String name);
    String getDirectoryName();

    int getExperimentNumber() { return experimentNumber; }
    int getRecordingNumber() { return recordingNumber; }

    void saveCustomParametersToXml (XmlElement* parentElement);
    void loadCustomParametersFromXml();

    CameraDevice* cameraDevice;
    bool hasCameraDevice { false };

    bool threadShouldExit { false };

private:

    void imageReceived (const juce::Image& image) override;

    //void run() override;

    Array<String> availableDevices;

    int64 frameCount;

    bool threadRunning;
    bool isRecording;
    File recordingDir;
    int imageQuality;
    int colorMode;
    int writeMode;
    bool resetFrameCounter;
    String dirName;
    int currentDeviceIndex;
    int currentStreamId;
    std::unique_ptr<WriteThread> writeThread;

    int experimentNumber;
    int recordingNumber;

    CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrameGrabber);
};

#endif // FrameGrabber_H_INCLUDED
