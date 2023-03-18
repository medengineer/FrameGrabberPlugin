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
public:
    WriteThread()
        : Thread ("WriteThread"), framePath(), timestampFile(), frameCounter(0), experimentNumber(1), recordingNumber(0), isRecording(false)
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

	juce::int64 getFrameCount()
	{
		int count;
		lock.enter();
		count = frameCounter;
		lock.exit();

		return count;
	}

	void resetFrameCounter()
	{
		lock.enter();
		frameCounter = 0;
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

	bool addFrame(juce::Image &frame, juce::int64 srcTs, juce::int64 swTs, int quality = 95)
	{
		bool status;

		if (isThreadRunning())
		{
			lock.enter();
			frameBuffer.add(new FrameWithTS(frame, srcTs, swTs, quality));
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

					++frameCounter;

					fileName = String::formatted("frame_%.10lld_%d_%d.jpg", frameCounter, experimentNumber, recordingNumber);
		            filePath = String(framePath.getFullPathName() + File::getSeparatorString() + fileName);

					lock.exit();

					/* TODO: Write to disk
					std::vector<int> compression_params;
					compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
					compression_params.push_back(frame_ts->getImageQuality());
					cv::imwrite(filePath.toRawUTF8(), (*frame_ts->getFrame()), compression_params);
					*/

					lock.enter();
					line = String::formatted("%lld,%d,%d,%lld,%lld\n", frameCounter, experimentNumber, recordingNumber, frame_ts->getTS(), frame_ts->getSWTS());
					lock.exit();
					timestampFile.appendText(line);

				}

            }
			else
			{
				sleep(50);
			}
        }
    }

private:
	OwnedArray<FrameWithTS> frameBuffer;
	juce::int64 frameCounter;
	int experimentNumber;
	int recordingNumber;
	File framePath;
	File timestampFile;
	bool isRecording;
	CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WriteThread)
};

FrameGrabber::FrameGrabber()
    : GenericProcessor("Frame Grabber"), currentFormatIndex(-1),
	  frameCounter(0), Thread("FrameGrabberThread"), isRecording(false), framePath(""),
	  imageQuality(25), colorMode(ColorMode::GRAY), writeMode(ImageWriteMode::RECORDING),
	  resetFrameCounter(false), dirName("frames")

{

	File recPath = CoreServices::getRecordingParentDirectory();
	framePath = File(recPath.getFullPathName() + File::getSeparatorString() + dirName);

	    /* Create a File Reader device */
    DeviceInfo::Settings settings {
        "Frame Grabber",
        "description",
        "identifier",
        "00000x003",
        "Open Ephys"
    };
    devices.add(new DeviceInfo(settings));

    isEnabled = false;

	writeThread = new WriteThread();
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
 	LOGD("FrameGrabber updating  settings.");

	DataStream::Settings streamSettings{

		"FrameGrabber",
		"description",
		"identifier",
		getDefaultSampleRate()

	};

	LOGD("File Reader adding data stream.");

	dataStreams.add(new DataStream(streamSettings));
	dataStreams.getLast()->addProcessor(processorInfo.get());

	ContinuousChannel::Settings channelSettings
	{
		ContinuousChannel::Type::ELECTRODE,
		"DUMMY_CHANNEL",
		"description",
		"filereader.stream",
		0.195,
		dataStreams.getLast()
	};

	continuousChannels.add(new ContinuousChannel(channelSettings));
	continuousChannels.getLast()->addProcessor(processorInfo.get());

	EventChannel* events;

	EventChannel::Settings eventSettings{
		EventChannel::Type::TTL,
		"All TTL events",
		"All TTL events loaded for the current input data source",
		"filereader.events",
		dataStreams.getLast()
	};

	events = new EventChannel(eventSettings);
	String id = "sourceevent";
	events->setIdentifier(id);
	events->addProcessor(processorInfo.get());
	eventChannels.add(events);

}

void FrameGrabber::startRecording()
{
	/*
	if (writeMode == RECORDING)
	{
		File recPath = CoreServices::RecordNode::getRecordingPath();
		framePath = File(recPath.getFullPathName() + recPath.separatorString + dirName);

		if (!framePath.exists() && !framePath.isDirectory())
		{
			Result result = framePath.createDirectory();
			if (result.failed())
			{
				std::cout << "FrameGrabber: failed to create frame path " << framePath.getFullPathName().toRawUTF8() << "\n";
				framePath = File();
			}
		}

		if (framePath.exists())
		{
			writeThread->setRecording(false);
			writeThread->setFramePath(framePath);
			writeThread->setExperimentNumber(CoreServices::RecordNode::getExperimentNumber());
			writeThread->setRecordingNumber(CoreServices::RecordNode::getRecordingNumber());
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
	*/
}

void FrameGrabber::stopRecording()
{
	/*
	isRecording = false;
	if (writeMode == RECORDING)
	{
		writeThread->setRecording(false);
		FrameGrabberEditor* e = (FrameGrabberEditor*) editor.get();
		e->enableControls();
	}
	*/
}

void FrameGrabber::process(AudioSampleBuffer& buffer)
{
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
	lock.enter();
	colorMode = value;
	lock.exit();
}

int FrameGrabber::getColorMode()
{
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

juce::int64 FrameGrabber::getFrameCount()
{
	int count;
	lock.enter();
	count = frameCounter;
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
			std::cout << "FrameGrabber invalid directory name: " << name.toStdString() << "\n";
		}
	}
}

String FrameGrabber::getDirectoryName()
{
	return dirName;
}

juce::int64 FrameGrabber::getWrittenFrameCount()
{
	return writeThread->getFrameCount();
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