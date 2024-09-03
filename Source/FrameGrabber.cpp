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
#include <stdio.h>

class FrameGrabberEditor;

FrameGrabber::FrameGrabber()
    : GenericProcessor ("Frame Grabber"),
      currentDeviceIndex (-1),
      currentStreamId (-1),
      frameCount (0),
      isRecording (false),
      imageQuality(),
      experimentNumber (1),
      recordingNumber (0)
{
    if (CameraDevice::getAvailableDevices().size())
    {
        hasCameraDevice = true;
        currentDeviceIndex = 0;
        cameraDevice = CameraDevice::openDevice (currentDeviceIndex);

        for (auto& device : CameraDevice::getAvailableDevices())
            availableDevices.add (device.toStdString());
    }

    /* Create a Camera Device */
    DeviceInfo::Settings settings {
        "Frame Grabber",
        "description",
        "identifier",
        "00000x003",
        "Open Ephys"
    };

    devices.add (new DeviceInfo (settings));

    writeThread = std::make_unique<WriteThread>();

    isEnabled = hasCameraDevice;
}

FrameGrabber::~FrameGrabber()
{
    cameraDevice->removeListener (this);
    delete cameraDevice;
}

AudioProcessorEditor* FrameGrabber::createEditor()
{
    editor = std::make_unique<FrameGrabberEditor> (this);
    return editor.get();
}

void FrameGrabber::registerParameters()
{
    addCategoricalParameter (Parameter::PROCESSOR_SCOPE, "video_source", "Video Source", "The device used to grab frames", availableDevices, 0, true);
    addSelectedStreamParameter (Parameter::PROCESSOR_SCOPE, "stream_source", "Stream Source", "The stream to synchronize frames with", {}, 0);

#ifdef _WIN32
    Array<String> imageQualityOptions = { "High", "Medium", "Low" };
#else
    Array<String> imageQualityOptions = { "High" };
#endif
    addCategoricalParameter (Parameter::PROCESSOR_SCOPE, "image_quality", "Image Quality", "The quality of the saved images", imageQualityOptions, 0, true);

    String defaultRecordDirectory = CoreServices::getRecordingParentDirectory().getFullPathName();
    addPathParameter (Parameter::PROCESSOR_SCOPE, "directory_name", "Write Directory", "The directory where video files will be saved", defaultRecordDirectory, {}, true);
}

void FrameGrabber::parameterValueChanged (Parameter* p)
{
	if (p->getName() == "video_source")
	{
		currentDeviceIndex = p->getValue();
        /*
		cameraDevice.release();
		Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 1000);
		cameraDevice.reset(CameraDevice::openDevice(currentDeviceIndex));
		getEditor()->updateSettings();
        */
	}
	else if (p->getName() == "stream_source")
	{
        setCurrentStreamIdFromIndex((((SelectedStreamParameter*)p)->getSelectedIndex()));
	}
    /*
	else if (parameter->isParameter ("image_quality"))
	{
		setImageQuality (parameter->getValue());
	}
	else if (parameter->isParameter ("color_mode"))
	{
		setColorMode (parameter->getValue());
	}
	else if (parameter->isParameter ("write_mode"))
	{
		setWriteMode (parameter->getValue());
	}
	else if (parameter->isParameter ("reset_frame_counter"))
	{
		setResetFrameCounter (parameter->getValue());
	}
	else if (parameter->isParameter ("directory_name"))
	{
		setDirectoryName (parameter->getValueAsString());
	}
    */
}

void FrameGrabber::updateSettings()
{
}

void FrameGrabber::imageReceived(const juce::Image& image)
{
    int64 receivedTime = CoreServices::getSystemTime();
    writeThread->addSyncFrame (receivedTime);
}

void FrameGrabber::handleTTLEvent (TTLEventPtr event)
{
    /*
    if (isRecording) {

        const int eventId = event->getState() ? 1 : 0;
        const int eventChannel = event->getLine();
        const uint16 eventStreamId = event->getChannelInfo()->getStreamId();
        const int eventSourceNodeId = event->getChannelInfo()->getSourceNodeId();
        const int eventTime = int (event->getSampleNumber() - getFirstSampleNumberForBlock (eventStreamId));
        const int eventSampleNumber = event->getSampleNumber();

        if (eventStreamId == currentStreamId)
        {
            LOGC("FrameGrabber: TTL event from current stream: ", eventStreamId);
            cameraDevice->takeStillPicture([this, eventSampleNumber](const juce::Image& image) { 
                //imageReceived(image);
                writeThread->addSyncFrame (image, eventSampleNumber, CoreServices::getSystemTime());
            });
        }
    }
    */
}

bool FrameGrabber::startAcquisition()
{
    cameraDevice->addListener (this);
    experimentNumber++;
    recordingNumber = 0;
    return true;
}

bool FrameGrabber::stopAcquisition()
{
    cameraDevice->removeListener (this);
    return true;
}

void FrameGrabber::startRecording()
{
    recordingNumber++;

    if (true)
    {
        String recPath = getParameter ("directory_name")->getValueAsString();

        recordingDir = File (
            recPath + File::getSeparatorString() + CoreServices::getRecordingDirectoryBaseText() + File::getSeparatorString() + "Frame Grabber " + String (getNodeId()) + File::getSeparatorString() + "experiment" + String (experimentNumber) + File::getSeparatorString() + "recording" + String (recordingNumber) + File::getSeparatorString() + dirName);

        LOGD ("Writing frames to: ", recordingDir.getFullPathName().toRawUTF8());

        if (! recordingDir.exists() && ! recordingDir.isDirectory())
        {
            LOGD ("Creating directory at ", recordingDir.getFullPathName().toRawUTF8());
            Result result = recordingDir.createDirectory();
            if (result.failed())
            {
                LOGC ("FrameGrabber: failed to create frame path ", recordingDir.getFullPathName().toRawUTF8());

                recordingDir = File();
            }
            else
            {
                //Create frames directory
                File (recordingDir.getFullPathName() + File::getSeparatorString() + "frames").createDirectory();
            }
        }

        if (recordingDir.exists())
        {
            writeThread->setRecording (false);
            writeThread->setRecordPath (recordingDir);
            writeThread->setExperimentNumber (experimentNumber);
            writeThread->setRecordingNumber (recordingNumber);
            writeThread->createTimestampFile();
            writeThread->setRecording (true);

            LOGC ("Recording to format: ", cameraDevice->getFileExtension());
            cameraDevice->startRecordingToFile (recordingDir.getChildFile ("video" + cameraDevice->getFileExtension()));
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
        writeThread->setRecording (false);

        int64 recordStartTime = cameraDevice->getTimeOfFirstRecordedFrame().toMilliseconds();
        LOGC ("First recorded frame time: ", recordStartTime);
        writeThread->writeFirstRecordedFrameTime (recordStartTime);
    }
}

void FrameGrabber::process (AudioSampleBuffer& buffer)
{
    int64 firstSampleNumberInBlock = getFirstSampleNumberForBlock (currentStreamId);
    int64 softwareTime = CoreServices::getSystemTime();
    writeThread->addBlockTimestamp (firstSampleNumberInBlock, softwareTime);
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

void FrameGrabber::setCurrentDevice (int index)
{
	LOGC( "Got index: ", index);
    currentDeviceIndex = index;
	cameraDevice = CameraDevice::openDevice (currentDeviceIndex);
	if (cameraDevice != nullptr)
	{
		getEditor()->updateSettings();
	}
	else
		LOGC( "Failed to open device at index: ", index)
}

void FrameGrabber::setCurrentStreamIdFromIndex (int index)
{
    if (index >= 0)
        currentStreamId = getDataStreams()[index]->getStreamId();
}

void FrameGrabber::setImageQuality (int q)
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

void FrameGrabber::setColorMode (int value)
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

void FrameGrabber::setWriteMode (int mode)
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

void FrameGrabber::setResetFrameCounter (bool enable)
{
    resetFrameCounter = enable;
}

bool FrameGrabber::getResetFrameCounter()
{
    return resetFrameCounter;
}

void FrameGrabber::setDirectoryName (String name)
{
    if (name != getDirectoryName())
    {
        if (File::createLegalFileName (name) == name)
        {
            dirName = name;
        }
        else
        {
            LOGC ("FrameGrabber invalid directory name: ", name.toStdString());
        }
    }
}

String FrameGrabber::getDirectoryName()
{
    return dirName;
}

void FrameGrabber::saveCustomParametersToXml (XmlElement* xml)
{
    xml->setAttribute ("Type", "FrameGrabber");

    XmlElement* paramXml = xml->createNewChildElement ("PARAMETERS");
    paramXml->setAttribute ("ImageQuality", getImageQuality());
    paramXml->setAttribute ("ColorMode", getColorMode());
    paramXml->setAttribute ("WriteMode", getWriteMode());
    paramXml->setAttribute ("ResetFrameCounter", getResetFrameCounter());
    paramXml->setAttribute ("DirectoryName", getDirectoryName());

    XmlElement* deviceXml = xml->createNewChildElement ("DEVICE");
    deviceXml->setAttribute ("API", "V4L2");
    if (currentDeviceIndex >= 0)
    {
        //TODO: Actually get and save format
        deviceXml->setAttribute ("Format", "TODO");
    }
    else
    {
        deviceXml->setAttribute ("Format", "");
    }
}

void FrameGrabber::loadCustomParametersFromXml()
{
    forEachXmlChildElementWithTagName (*parametersAsXml, paramXml, "PARAMETERS")
    {
        if (paramXml->hasAttribute ("ImageQuality"))
            setImageQuality (paramXml->getIntAttribute ("ImageQuality"));

        if (paramXml->hasAttribute ("ColorMode"))
            setColorMode (paramXml->getIntAttribute ("ColorMode"));

        if (paramXml->hasAttribute ("WriteMode"))
            setWriteMode (paramXml->getIntAttribute ("WriteMode"));

        if (paramXml->hasAttribute ("ResetFrameCounter"))
            setResetFrameCounter (paramXml->getIntAttribute ("ResetFrameCounter"));

        if (paramXml->hasAttribute ("DirectoryName"))
            setDirectoryName (paramXml->getStringAttribute ("DirectoryName"));
    }

    forEachXmlChildElementWithTagName (*parametersAsXml, deviceXml, "DEVICE")
    {
        String api = deviceXml->getStringAttribute ("API");
        if (api.compareIgnoreCase ("V4L2") == 0)
        {
            String format = deviceXml->getStringAttribute ("Format");
            int index = 0; //Camera::get_format_index(format.toStdString());
            if (index >= 0)
            {
                if (isCameraRunning())
                {
                    stopCamera();
                }
                startCamera (index);
            }
        }
        else
        {
            std::cout << "FrameGrabber API " << api << " not supported\n";
        }
    }

    updateSettings();
}