// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/juce/events.hpp>
#include <element/oversampler.hpp>

namespace element {

template <typename T>
Oversampler<T>::~Oversampler()
{
    reset();
    processors.clear (true);
}

template <typename T>
float Oversampler<T>::getLatencySamples (int index) const
{
    if (auto* const proc = getProcessor (index))
        return proc->getLatencyInSamples();
    return 0.f;
}

template <typename T>
int Oversampler<T>::getFactor (int index) const
{
    if (auto* const proc = getProcessor (index))
        return static_cast<int> (proc->getOversamplingFactor());
    return 1;
}

template <typename T>
void Oversampler<T>::prepare (int numChannels, int blockSize)
{
    numChannels = juce::jmax (1, numChannels);
    const bool procSpecChanged = channels != numChannels || buffer != blockSize;
    channels = numChannels;
    buffer = blockSize;

    if (procSpecChanged)
    {
        // Existing chains were built for the old spec. Drop them; chains are
        // (re)built on demand by ensureProcessor().
        processors.clear();
    }

    for (auto* proc : processors)
        if (proc != nullptr)
            proc->initProcessing ((size_t) buffer);
}

template <typename T>
typename Oversampler<T>::ProcessorType* Oversampler<T>::ensureProcessor (int index)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (index < 0 || index >= maxProc || channels <= 0 || buffer <= 0)
        return nullptr;

    while (processors.size() <= index)
        processors.add (nullptr);

    if (auto* const existing = processors.getUnchecked (index))
        return existing;

    auto* const proc = new ProcessorType (channels, index + 1, ProcessorType::FilterType::filterHalfBandPolyphaseIIR);
    proc->initProcessing ((size_t) buffer);
    processors.set (index, proc, true);
    return proc;
}

template <typename T>
void Oversampler<T>::reset()
{
    for (auto* const proc : processors)
        if (proc != nullptr)
            proc->reset();
}

template class Oversampler<float>;
template class Oversampler<double>;

} // namespace element
