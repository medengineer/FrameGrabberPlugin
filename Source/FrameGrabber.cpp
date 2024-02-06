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


#include <stdio.h>
#include "FrameGrabber.h"

class FrameGrabberEditor;

/*
We have to put some class definitions that are using opencv here because
there will be a data type conflict (int64) between opencv and JUCE if these
classes are declared in the header file.
*/

class WriteThread : public Thread
{
	OwnedArray<FrameWithTS> frameBuffer;
	juce::int64 frameCount;
	File framePath;
	File timestampFile;
	bool isRecording;
	CriticalSection lock;

	int experimentNumber;
	int recordingNumber;

public:
    WriteThread()
        : Thread ("WriteThread"),
		frameCount(0),
		framePath(),
		timestampFile(), 
		isRecording(false),
		experimentNumber(0),
		recordingNumber(0)
    {
		frameBuffer.clear();
        startThread();
    }

    ~WriteThread()
    {
        stopThread(1000);
		clearBuffer();
    }

	void setFramePath(File &f)
	{
		lock.enter();
		framePath = File(f);
		lock.exit();
	}

	void createTimestampFile(String name = "frame_timestamps")
	{

		String filePath = framePath.getFullPathName()
			   + File::getSeparatorString()
			   + name
			   + ".csv";

		timestampFile = File(filePath);
		if (!timestampFile.exists())
		{
			timestampFile.create();
			timestampFile.appendText("# Frame index, Recording number, Experiment number, Source timestamp, Software timestamp\n");
		}
	}

	int64 getFrameCount()
	{
		int count;
		lock.enter();
		count = frameCount;
		lock.exit();

		return count;
	}

	void resetFrameCounter()
	{
		lock.enter();
		frameCount = 0;
		lock.exit();
	}

	void setExperimentNumber(int n)
	{
		lock.enter();
		experimentNumber = n;
		lock.exit();
	}

	void setRecordingNumber(int n)
	{
		lock.enter();
		recordingNumber = n;
		lock.exit();
	}

	bool addFrame(int64 srcTs, int64 swTs, int quality = 95)
	{
		bool status;

		if (isThreadRunning())
		{
			lock.enter();
			frameBuffer.add(new FrameWithTS(srcTs, swTs, quality));
			lock.exit();
			status = true;
		}
		else
		{
			status = false;
		}

		return status;
	}

	void clearBuffer()
	{
		lock.enter();
		/* for some reason frameBuffer.clear() didn't delete the content ... */
		while (frameBuffer.size() > 0)
		{
			frameBuffer.removeAndReturn(0);
		}
		lock.exit();
	}

	bool hasValidPath()
	{
		bool status;

		lock.enter();
		status = (framePath.exists() && timestampFile.exists());
		lock.exit();

		return status;
	}

	void setRecording(bool status)
	{
		lock.enter();
		isRecording = status;
		lock.exit();
	}

    void run() override
    {
		FrameWithTS* frame_ts;
		int imgQuality;
		String fileName;
		String filePath;
		String line;

        while (!threadShouldExit())
        {

			if (isRecording && hasValidPath()) 
			{
				if (frameBuffer.size() > 0)
				{
					frame_ts = frameBuffer.removeAndReturn(0);
				}
				else
				{
					frame_ts = NULL;
				}

				if (frame_ts != NULL)
				{

					if (SAVE_IMAGE_FRAMES)
					{
						lock.enter();

						//fileName = String::formatted("frame_%.10lld_%d_%d.jpg", frameCounter, experimentNumber, recordingNumber);
						fileName = String::formatted("frame at %.10lld.jpg", frameCount);
						filePath = String(framePath.getFullPathName() + File::getSeparatorString() + fileName);

						juce::File outputFile(filePath);
						juce::FileOutputStream stream (outputFile);
						JPEGImageFormat jpegFormat;
						jpegFormat.setQuality(frame_ts->getImQ());
						//jpegFormat.writeImageToStream(frame_ts->getFrame(), stream);

						lock.exit();
					}

					lock.enter();
					line = String::formatted("%lld,%d,%d,%lld,%lld\n", frameCount, experimentNumber, recordingNumber, frame_ts->getTS(), frame_ts->getSWTS());
					lock.exit();
					timestampFile.appendText(line);

					frameCount++;

				}

            }
			else
			{
				sleep(50);
			}
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WriteThread)
};

FrameGrabber::FrameGrabber()
    : GenericProcessor("Frame Grabber"), 
		currentDeviceIndex(-1),
		currentStreamIndex(-1),
		frameCount(0),
		isRecording(false),
		framePath(""),
		imageQuality(),
		experimentNumber(0),
		recordingNumber(0)
{

	if (CameraDevice::getAvailableDevices().size())
	{
		hasCameraDevice = true;
		currentDeviceIndex = 0;
		cameraDevice.reset(CameraDevice::openDevice(currentDeviceIndex));

		for (auto& device : CameraDevice::getAvailableDevices())
			availableDevices.add(device.toStdString());
	}

	addCategoricalParameter(Parameter::PROCESSOR_SCOPE, "video_source", "Video Source", "The device used to grab frames", availableDevices, 0, true);
	addSelectedStreamParameter(Parameter::PROCESSOR_SCOPE, "stream_source", "Stream Source", "The stream to synchronize frames with", {}, 0);

	Array<String> imageQualityOptions = { "High", "Medium", "Low" };
	addCategoricalParameter(Parameter::PROCESSOR_SCOPE, "image_quality", "Image Quality", "The quality of the saved images", imageQualityOptions, 0, true);

	String defaultRecordDirectory = CoreServices::getRecordingParentDirectory().getFullPathName();
	addPathParameter(Parameter::PROCESSOR_SCOPE, "directory_name", "Write Directory", "The directory where video files will be saved", defaultRecordDirectory, {}, true);

	/* Create a Camera Device */
    DeviceInfo::Settings settings {
        "Frame Grabber",
        "description",
        "identifier",
        "00000x003",
        "Open Ephys"
    };

    devices.add(new DeviceInfo(settings));

	writeThread = std::make_unique<WriteThread>();

	isEnabled = hasCameraDevice;
}

FrameGrabber::~FrameGrabber()
{
	cameraDevice->removeListener(this);
}

AudioProcessorEditor* FrameGrabber::createEditor()
{
    editor = std::make_unique<FrameGrabberEditor>(this);

    return editor.get();
}

void FrameGrabber::updateSettings()
{
	if (currentStreamIndex < 0)
		currentStreamIndex = 0;
}

void FrameGrabber::imageReceived(const juce::Image& image)
{

	//LOGD("Width: ", image.getWidth(), " Height: ", image.getHeight());
	if (!headlessMode && !firstImageReceived)
	{
		static_cast<FrameGrabberEditor*>(editor.get())->setCameraViewportSize(image.getWidth(), image.getHeight());
		firstImageReceived = true;
	}
	
	//Gets called ~15 fps w/ Logitech C920 @ 960x720
	if (isRecording)
	{
		int64 swTs = CoreServices::getSoftwareTimestamp();
		int streamId = getDataStreams()[currentStreamIndex]->getStreamId();
		int64 ts = getFirstSampleNumberForBlock(streamId);
		int64 offset_in_ms = swTs - blockTimestamps[ts];

		int64 synchronized_ts = ts + offset_in_ms * getDataStreams()[currentStreamIndex]->getSampleRate() / 1000.0f;

		writeThread->addFrame(synchronized_ts, swTs, getImageQuality());
	}

	frameCount++;
}

bool FrameGrabber::startAcquisition()
{
	cameraDevice->addListener(this);
	experimentNumber++;
	recordingNumber = 0;
	return true;
}

bool FrameGrabber::stopAcquisition()
{
	cameraDevice->removeListener(this);
	return true;
}

void FrameGrabber::startRecording()
{

	recordingNumber++;

	if (true)
	{
		String recPath = getParameter("directory_name")->getValueAsString();

		framePath = File(
			recPath + File::getSeparatorString() + 
			CoreServices::getRecordingDirectoryBaseText() + File::getSeparatorString() + 
			"Frame Grabber " + String(getNodeId()) + File::getSeparatorString() +
			"experiment" + String(experimentNumber) + File::getSeparatorString() +
			"recording" + String(recordingNumber) + File::getSeparatorString() +
			dirName);

		LOGD("Writing frames to: ", framePath.getFullPathName().toRawUTF8());

		if (!framePath.exists() && !framePath.isDirectory())
		{
			LOGD("Creating directory at ", framePath.getFullPathName().toRawUTF8());
			Result result = framePath.createDirectory();
			if (result.failed())
			{
				LOGC("FrameGrabber: failed to create frame path ", framePath.getFullPathName().toRawUTF8());
				framePath = File();
			}
		}

		if (framePath.exists())
		{
			writeThread->setRecording(false);
			writeThread->setFramePath(framePath);
			writeThread->setExperimentNumber(experimentNumber);
			writeThread->setRecordingNumber(recordingNumber);
			writeThread->createTimestampFile();
			if (resetFrameCounter)
			{
				writeThread->resetFrameCounter();
			}
			writeThread->setRecording(true);

			LOGC("Recording to format: ", cameraDevice->getFileExtension());
			cameraDevice->startRecordingToFile(framePath.getChildFile("video" + cameraDevice->getFileExtension()));
		}
	}

	isRecording = true;
}

void FrameGrabber::stopRecording()
{
	isRecording = false;
	if (true)
	{
		cameraDevice->stopRecording();
		writeThread->setRecording(false);

		int64 recordStartTime = cameraDevice->getTimeOfFirstRecordedFrame().toMilliseconds();
		LOGD("First recorded frame time: ", recordStartTime);

	}
	blockTimestamps.clear();
	writeThread->clearBuffer();
}

void FrameGrabber::process(AudioSampleBuffer& buffer)
{
	int streamId = getDataStreams()[currentStreamIndex]->getStreamId();
	int64 ts = getFirstSampleNumberForBlock(streamId);
	blockTimestamps[ts] = CoreServices::getSoftwareTimestamp();
}

/*
void FrameGrabber::run()
{

	juce::int64 srcTS;
	juce::int64 swTS;
	bool recStatus;
	int imgQuality;
	//bool winState;
	//bool cMode;
	int wMode;

    while (!threadShouldExit())
    {
		if (camera != NULL && camera->is_running())
		{

			cv::Mat frame = camera->read_frame();
			if (!frame.empty())
			{
				bool writeImage = false;

				lock.enter();
				imgQuality = imageQuality;
				recStatus = isRecording;
				wMode = writeMode;
				//cMode = colorMode;

				writeImage = (wMode == ImageWriteMode::ACQUISITION) || (wMode == ImageWriteMode::RECORDING && recStatus);
				lock.exit();

				if (writeImage)
				{
					srcTS = CoreServices::getGlobalTimestamp();
					swTS = CoreServices::getSoftwareTimestamp();
					writeThread->addFrame(&frame, srcTS, swTS, imgQuality);
				}

				cv::imshow("FrameGrabber", frame);
				cv::waitKey(1);

				frameCounter++;
			}
		}
    }

}
*/

void FrameGrabber::setCurrentDevice(int index)
{
	lock.enter();
	//TODO: Connect to a different device (test on Mac with native cam + USB webcam)
	currentDeviceIndex = index;
	lock.exit();
}

void FrameGrabber::setCurrentStreamIndex(int index)
{
	lock.enter();
	currentStreamIndex = index;
	//Next acquisition will use the timestamps from the new stream
	lock.exit();
}

void FrameGrabber::setImageQuality(int q)
{
	lock.enter();
	imageQuality = q;
	if (imageQuality <= 0)
	{
		imageQuality = 1;
	}
	else if (imageQuality > 100)
	{
		imageQuality = 100;
	}
	lock.exit();
}

int FrameGrabber::getImageQuality()
{
	int q;
	lock.enter();
	q = imageQuality;
	lock.exit();

	return q;
}

void FrameGrabber::setColorMode(int value)
{
	// 0 = grayscale, 1 = color
	lock.enter();
	colorMode = value;
	lock.exit();
}

int FrameGrabber::getColorMode()
{
	// 0 = grayscale, 1 = color
	int mode;
	lock.enter();
	mode = colorMode;
	lock.exit();

	return mode;
}

void FrameGrabber::setWriteMode(int mode)
{
	writeMode = mode;
}

int FrameGrabber::getWriteMode()
{
	return writeMode;
}

int64 FrameGrabber::getFrameCount()
{
	int count;

	lock.enter();
	count = frameCount;
	lock.exit();

	return count;
}

void FrameGrabber::setResetFrameCounter(bool enable)
{
	resetFrameCounter = enable;
}

bool FrameGrabber::getResetFrameCounter()
{
	return resetFrameCounter;
}

void FrameGrabber::setDirectoryName(String name)
{
	if (name != getDirectoryName())
	{
		if (File::createLegalFileName(name) == name)
		{
			dirName = name;
		}
		else
		{
			LOGC("FrameGrabber invalid directory name: ", name.toStdString());
		}
	}
}

String FrameGrabber::getDirectoryName()
{
	return dirName;
}

void FrameGrabber::saveCustomParametersToXml(XmlElement* xml)
{
    xml->setAttribute("Type", "FrameGrabber");

    XmlElement* paramXml = xml->createNewChildElement("PARAMETERS");
    paramXml->setAttribute("ImageQuality", getImageQuality());
	paramXml->setAttribute("ColorMode", getColorMode());
	paramXml->setAttribute("WriteMode", getWriteMode());
	paramXml->setAttribute("ResetFrameCounter", getResetFrameCounter());
	paramXml->setAttribute("DirectoryName", getDirectoryName());

	XmlElement* deviceXml = xml->createNewChildElement("DEVICE");
	deviceXml->setAttribute("API", "V4L2");
	if (currentDeviceIndex >= 0)
	{
		//TODO: Actually get and save format
		deviceXml->setAttribute("Format", "TODO");
	}
	else
	{
		deviceXml->setAttribute("Format", "");
	}
}

void FrameGrabber::loadCustomParametersFromXml()
{
	forEachXmlChildElementWithTagName(*parametersAsXml,	paramXml, "PARAMETERS")
	{
		if (paramXml->hasAttribute("ImageQuality"))
			setImageQuality(paramXml->getIntAttribute("ImageQuality"));

		if (paramXml->hasAttribute("ColorMode"))
			setColorMode(paramXml->getIntAttribute("ColorMode"));

    	if (paramXml->hasAttribute("WriteMode"))
			setWriteMode(paramXml->getIntAttribute("WriteMode"));

		if (paramXml->hasAttribute("ResetFrameCounter"))
			setResetFrameCounter(paramXml->getIntAttribute("ResetFrameCounter"));

		if (paramXml->hasAttribute("DirectoryName"))
			setDirectoryName(paramXml->getStringAttribute("DirectoryName"));
	}

	forEachXmlChildElementWithTagName(*parametersAsXml,	deviceXml, "DEVICE")
	{
		String api = deviceXml->getStringAttribute("API");
		if (api.compareIgnoreCase("V4L2") == 0)
		{
			String format = deviceXml->getStringAttribute("Format");
			int index = 0; //Camera::get_format_index(format.toStdString());
			if (index >= 0)
			{
				if (isCameraRunning())
				{
					stopCamera();
				}
				startCamera(index);
			}
		}
		else
		{
			std::cout << "FrameGrabber API " << api << " not supported\n";
		}
	}

	updateSettings();
}