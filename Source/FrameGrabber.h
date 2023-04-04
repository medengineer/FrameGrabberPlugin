/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2023 Allen Institute for Brain Science and Open Ephys

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
#include "FrameGrabberEditor.h"

//TODO: Support other source types
enum SOURCE_TYPE {
	WEBCAM = 0,
	OTHER,
};

//TODO: Support other image types
using T = juce::Image;

class FrameWithTS 
{
    public:
	    FrameWithTS(
            const T frame_,
            int64 ts_,
            int64 swTs_,
			int imQ_
        ) : frame(frame_), ts(ts_), swTs(swTs_), imQ(imQ_) {};

        const T &getFrame() const { return frame; }
        int64 getTS() const { return ts; }
        int64 getSWTS() const { return swTs; }
        int getImQ() const { return imQ; }

    private:
        const T frame;
        int64 ts, swTs;
        int imQ;
};

class WriteThread;

enum ImageWriteMode {NEVER = 0, RECORDING = 1, ACQUISITION = 2};
enum ColorMode {GRAY = 0, RGB = 1};


class FrameGrabber : public GenericProcessor, private Thread, public juce::CameraDevice::Listener
{
public:

    FrameGrabber();
    ~FrameGrabber();

	AudioProcessorEditor* createEditor() override;

	String handleConfigMessage(String msg) override { return "TODO"; }

	/* Updates the FileReader settings*/
    void updateSettings() override;

    void process(AudioBuffer<float>& buffer);

	void startRecording();
	void stopRecording();

    bool generatesTimestamps() const override { return true; }

	/* Called at start of acquisition */
	bool startAcquisition() override { /* TODO */ return true; };

    /* Called at end of acquisition */
	bool stopAcquisition() override { /* TODO */ return true; };

	int startCamera(int fmt_index) { /* TODO */ return 0; }
	int stopCamera() { /* TODO */ return 0; }
	bool isCameraRunning() { /* TODO */ return false; }

	std::vector<std::string> getFormats() { return formats; }
	int getCurrentFormatIndex() { return currentFormatIndex; }
	void setCurrentFormatIndex(int index);

	int getCurrentStreamIndex() { return currentStreamIndex; }
	void setCurrentStreamIndex(int index);

	void setImageQuality(int q);
	int getImageQuality();

	void setColorMode(int value);
	int getColorMode();

	void setWriteMode(int value);
	int getWriteMode();

	void setResetFrameCounter(bool enable);
	bool getResetFrameCounter();

	int64 getFrameCount();

	void setDirectoryName(String name);
	String getDirectoryName();

	void saveCustomParametersToXml(XmlElement* parentElement);
	void loadCustomParametersFromXml();

	CameraDevice* cameraDevice;
	bool hasCameraDevice { false };

private:

    void imageReceived(const juce::Image& image) override;

	void run() override;

	std::vector<std::string> formats;

	int64 frameCount;

	bool threadRunning;
	bool isRecording;
	File framePath;
	int imageQuality;
	int colorMode;
	int writeMode;
	bool resetFrameCounter;
	String dirName;
	int currentFormatIndex;
	int currentStreamIndex;
	WriteThread* writeThread;

	CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrameGrabber);
};


#endif  // FrameGrabber_H_INCLUDED

