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
	int experimentNumber;
	int recordingNumber;
	File framePath;
	File timestampFile;
	bool isRecording;
	CriticalSection lock;

public:
    WriteThread()
        : Thread ("WriteThread"),
		frameCount(0),
		framePath(),
		timestampFile(), 
		experimentNumber(1),
		recordingNumber(0),
		isRecording(false)
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

		if (!framePath.exists() || !framePath.isDirectory())
		{
			 framePath.createDirectory();
		}

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

	bool addFrame(const juce::Image &frame, int64 srcTs, int64 swTs, int quality = 95)
	{
		bool status;

		if (isThreadRunning())
		{
			lock.enter();
			frameBuffer.add(new FrameWithTS(frame.createCopy(), srcTs, swTs, quality));
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
					lock.enter();

					//fileName = String::formatted("frame_%.10lld_%d_%d.jpg", frameCounter, experimentNumber, recordingNumber);
					fileName = String::formatted("frame at %.10lld.jpg", frameCount);
		            filePath = String(framePath.getFullPathName() + File::getSeparatorString() + fileName);

					/* TODO: Write to disk
					std::vector<int> compression_params;
					compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
					compression_params.push_back(frame_ts->getImageQuality());
					cv::imwrite(filePath.toRawUTF8(), (*frame_ts->getFrame()), compression_params);
					*/

					juce::File outputFile(filePath);
				    juce::FileOutputStream stream (outputFile);
					JPEGImageFormat jpegFormat;
					jpegFormat.setQuality(frame_ts->getImQ());
					jpegFormat.writeImageToStream(frame_ts->getFrame(), stream);

					lock.exit();

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
		currentFormatIndex(-1),
		currentStreamIndex(-1),
		frameCount(0),
		Thread("FrameGrabberThread"),
		isRecording(false),
		framePath(""),
		imageQuality(25),
		colorMode(ColorMode::GRAY),
		writeMode(ImageWriteMode::RECORDING),
		resetFrameCounter(false)
{

	//TODO: Update this from camera device
	int width = 960;
	int height = 720;
	int maxWidth = 960;
	int maxHeight = 720;
	bool highQuality = false;

	if (CameraDevice::getAvailableDevices().size())
	{
		hasCameraDevice = true;
		currentFormatIndex = 0;
		cameraDevice = CameraDevice::openDevice(
			currentFormatIndex);
			// width,
			// height,
			// maxWidth,
			// maxHeight,
			// highQuality);
		for (auto& device : CameraDevice::getAvailableDevices())
			formats.push_back(device.toStdString());

		cameraDevice->addListener(this);
	}

	/* Create a Camera Device */
    DeviceInfo::Settings settings {
        "Frame Grabber",
        "description",
        "identifier",
        "00000x003",
        "Open Ephys"
    };
    devices.add(new DeviceInfo(settings));

	writeThread = new WriteThread();

	isEnabled = hasCameraDevice;

}

FrameGrabber::~FrameGrabber()
{
    signalThreadShouldExit();
    notify();
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
	//Gets called ~15 fps w/ Logitech C920 @ 960x720

	/* Whenever a frame is received, timestamp it and add it to a buffer */
	juce::int64 srcTS = CoreServices::getGlobalTimestamp();
	juce::int64 swTS = CoreServices::getSoftwareTimestamp();

	//LOGD("Received image with timestamp: ", srcTS);

	writeThread->addFrame(image, srcTS, swTS, getImageQuality());

	frameCount++;
}

void FrameGrabber::startRecording()
{

	dirName = "Frame Grabber " + String(getNodeId());

	if (writeMode == RECORDING)
	{
		File recPath = CoreServices::getRecordingParentDirectory();

		framePath = File(recPath.getFullPathName() + File::getSeparatorString() + CoreServices::getRecordingDirectoryBaseText() + File::getSeparatorString() + dirName);

		LOGD("Writing frames to: ", framePath.getFullPathName().toRawUTF8());

		if (!framePath.exists() && !framePath.isDirectory())
		{
			LOGC("Creating directory at ", framePath.getFullPathName().toRawUTF8());
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
			//writeThread->setExperimentNumber(CoreServices::RecordNode::getExperimentNumber());
			//writeThread->setRecordingNumber(CoreServices::RecordNode::getRecordingNumber());
			writeThread->createTimestampFile();
			if (resetFrameCounter)
			{
				writeThread->resetFrameCounter();
			}
			writeThread->setRecording(true);

			FrameGrabberEditor* e = (FrameGrabberEditor*) editor.get();
			e->disableControls();
		}
	}

	isRecording = true;
}

void FrameGrabber::stopRecording()
{
	isRecording = false;
	if (writeMode == RECORDING)
	{
		writeThread->setRecording(false);
		FrameGrabberEditor* e = (FrameGrabberEditor*) editor.get();
		e->enableControls();
	}
}

void FrameGrabber::process(AudioSampleBuffer& buffer)
{
	if (CoreServices::getRecordingStatus() && !isRecording)
	{
		startRecording();
	}
	else if (!CoreServices::getRecordingStatus() && isRecording)
	{
		stopRecording();
	}
}

void FrameGrabber::run()
{
	/*
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
	*/

}

void FrameGrabber::setCurrentFormatIndex(int index)
{
	lock.enter();
	//TODO: Connect to a different device (test on Mac with native cam + USB webcam)
	currentFormatIndex = index;
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
	if (currentFormatIndex >= 0)
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