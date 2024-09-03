#include "WriteThread.h"

WriteThread::WriteThread() : Thread("WriteThread"),
                            isRecording(false),
                            recordingDirectory(""),
                            experimentNumber(1),
                            recordingNumber(0)
                            {
                                startThread();
                            }

WriteThread::~WriteThread()
{
    stopThread(1000);
}

void WriteThread::setRecording(bool state)
{
    isRecording = state;
}

void WriteThread::setRecordPath(File path)
{
    recordingDirectory = path;
}

void WriteThread::setExperimentNumber(int n)
{
    experimentNumber = n;
}

void WriteThread::setRecordingNumber(int n)
{
    recordingNumber = n;
}

void WriteThread::createTimestampFile()
{
    String filePath = recordingDirectory.getFullPathName()
                      + File::getSeparatorString()
                      + "frame_timestamps.csv";

    timestampFile = File (filePath);
    if (! timestampFile.exists())
    {
        timestampFile.create();
        timestampFile.appendText ("# Frame index, Recording number, Experiment number, Source timestamp, Software timestamp\n");
    }
}

void WriteThread::addBlockTimestamp(int64 softwareTime, int64 sampleNumber)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    blockTimestamps[softwareTime] = sampleNumber;
}

void WriteThread::addSyncFrame(int64 receivedTime)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    imageReceivedTimes.push(receivedTime);
}

void WriteThread::run()
{
    while (!threadShouldExit())
    {
        if (!processNextImageTime())
        {
            // Sleep to reduce CPU usage when there is no work to do
            wait(10); // wait for 10 ms
        }
    }
}

bool WriteThread::processNextImageTime()
{
    std::lock_guard<std::mutex> lock(queueMutex);

    if (imageReceivedTimes.empty() || blockTimestamps.size() < 2)
        return false;

    int64 receivedTime = imageReceivedTimes.front();
    auto upperIt = blockTimestamps.upper_bound(receivedTime);

    if (upperIt == blockTimestamps.end() || upperIt == blockTimestamps.begin())
        return false;

    auto lowerIt = std::prev(upperIt);
    if (lowerIt->first > receivedTime)
        return false;

    // Interpolation code here
    double factor = double(receivedTime - lowerIt->first) / (upperIt->first - lowerIt->first);
    int64 interpolatedSample = lowerIt->second + int64(factor * (upperIt->second - lowerIt->second));

    // You can add code here to handle the interpolated sample as needed.

    imageReceivedTimes.pop(); // Successfully processed this time
    return true;
}

void WriteThread::writeFirstRecordedFrameTime(int64 time)
{
    String filePath = recordingDirectory.getFullPathName() + File::getSeparatorString() + "sync_messages.txt";
    File syncFile = File (filePath);
    Result res = syncFile.create();
    if (res.failed())
    {
        LOGE ("Error creating sync text file: ", res.getErrorMessage());
    }
    else
    {
        std::unique_ptr<FileOutputStream> syncTextFile = syncFile.createOutputStream();
        if (syncTextFile != nullptr)
        {
            syncTextFile->writeText ("First recorded frame time: " + String (time) + "\r\n", false, false, nullptr);
            syncTextFile->flush();
        }
        else
        {
            LOGE("Error creating sync text file: " + res.getErrorMessage());
        }
    }
}