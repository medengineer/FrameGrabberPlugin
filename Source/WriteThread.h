#ifndef WRITETHREAD_H_INCLUDED
#define WRITETHREAD_H_INCLUDED

#include <ProcessorHeaders.h>

class WriteThread : public Thread
{
public:
    WriteThread();
    ~WriteThread();

    void setRecordPath(File path);
    void setRecording(bool status);
    void setExperimentNumber(int n);
    void setRecordingNumber(int n);
    void createTimestampFile();

    void addBlockTimestamp(int64 softwareTime, int64 sampleNumber);
    void addSyncFrame(int64 receivedTime);

    void run() override;

    void writeFirstRecordedFrameTime (int64 time);

private:
    bool processNextImageTime();

    std::map<int64, int64> blockTimestamps;
    std::queue<int64> imageReceivedTimes;
    std::mutex queueMutex;

    bool isRecording;

    File recordingDirectory;
    int experimentNumber;
    int recordingNumber;
    File timestampFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WriteThread)
};

#endif // WRITETHREAD_H_INCLUDED