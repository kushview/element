

GraphPlayer::GraphPlayer()
    : processor (nullptr),
        sampleRate (0),
        blockSize (0),
        isPrepared (false),
        numInputChans (0),
        numOutputChans (0),
        tempBuffer (1, 1)
{
}

GraphPlayer::~GraphPlayer()
{
    setRootGraph (nullptr);
}

void GraphPlayer::setRootGraph (GraphProcessor* const nextGraph)
{
    if (processor != nextGraph)
    {
        if (nextGraph != nullptr && sampleRate > 0 && blockSize > 0)
        {
            nextGraph->setPlayConfigDetails (numInputChans, numOutputChans, sampleRate, blockSize);
            nextGraph->prepareToPlay (sampleRate, blockSize);
        }

        GraphProcessor* oldOne;

        {
            // const ScopedLock sl (lock);
            oldOne = isPrepared ? processor : nullptr;
            processor = nextGraph;
            isPrepared = true;
        }

        if (oldOne != nullptr)
            oldOne->releaseResources();
    }
}

void GraphPlayer::audioDeviceIOCallback (const float** const inputChannelData, const int numInputChannels,
                                         float** const outputChannelData, const int numOutputChannels,
                                         const int numSamples)
{
    jassert (sampleRate > 0 && blockSize > 0);

    incomingMidi.clear();
    messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples);
    int totalNumChans = 0;

    if (numInputChannels > numOutputChannels)
    {
        // if there aren't enough output channels for the number of
        // inputs, we need to create some temporary extra ones (can't
        // use the input data in case it gets written to)
        tempBuffer.setSize (numInputChannels - numOutputChannels, numSamples,
                            false, false, true);

        for (int i = 0; i < numOutputChannels; ++i)
        {
            channels[totalNumChans] = outputChannelData[i];
            memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * (size_t) numSamples);
            ++totalNumChans;
        }

        for (int i = numOutputChannels; i < numInputChannels; ++i)
        {
            channels[totalNumChans] = tempBuffer.getWritePointer (i - numOutputChannels, 0);
            memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * (size_t) numSamples);
            ++totalNumChans;
        }
    }
    else
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            channels[totalNumChans] = outputChannelData[i];
            memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * (size_t) numSamples);
            ++totalNumChans;
        }

        for (int i = numInputChannels; i < numOutputChannels; ++i)
        {
            channels[totalNumChans] = outputChannelData[i];
            zeromem (channels[totalNumChans], sizeof (float) * (size_t) numSamples);
            ++totalNumChans;
        }
    }

    AudioSampleBuffer buffer (channels, totalNumChans, numSamples);

    const ScopedLock sl (lock);

    if (processor != nullptr)
    {
        const ScopedLock sl2 (processor->getCallbackLock());

        if (processor->isSuspended())
        {
            for (int i = 0; i < numOutputChannels; ++i)
                zeromem (outputChannelData[i], sizeof (float) * (size_t) numSamples);
        }
        else
        {
            processor->processBlock (buffer, incomingMidi);
        }
    }
}

void GraphPlayer::audioDeviceAboutToStart (AudioIODevice* const device)
{
    const double newSampleRate = device->getCurrentSampleRate();
    const int newBlockSize     = device->getCurrentBufferSizeSamples();
    const int numChansIn       = device->getActiveInputChannels().countNumberOfSetBits();
    const int numChansOut      = device->getActiveOutputChannels().countNumberOfSetBits();

    const ScopedLock sl (lock);

    sampleRate = newSampleRate;
    blockSize  = newBlockSize;
    numInputChans  = numChansIn;
    numOutputChans = numChansOut;

    messageCollector.reset (sampleRate);
    channels.calloc ((size_t) jmax (numChansIn, numChansOut) + 2);

    if (processor != nullptr)
    {
        if (isPrepared)
        {
            processor->releaseResources();
            isPrepared = false;
        }

        GraphProcessor* const oldProcessor = processor;
        setRootGraph (nullptr);
        setRootGraph (oldProcessor);
    }
}

void GraphPlayer::audioDeviceStopped()
{
    const ScopedLock sl (lock);

    if (processor != nullptr && isPrepared)
        processor->releaseResources();

    sampleRate = 0.0;
    blockSize = 0;
    isPrepared = false;
    tempBuffer.setSize (1, 1);
}

void GraphPlayer::handleIncomingMidiMessage (MidiInput*, const MidiMessage& message)
{
    messageCollector.addMessageToQueue (message);
}
