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
    reset();

    numChannels = juce::jmax (1, numChannels);

    if (processors.size() <= 0 || channels != numChannels || buffer != blockSize)
    {
        buffer = blockSize;
        channels = numChannels;
        processors.clear();
        for (int f = 0; f < maxProc; ++f)
            processors.add (new ProcessorType (channels, f + 1, ProcessorType::FilterType::filterHalfBandPolyphaseIIR));
    }

    for (auto* proc : processors)
        proc->initProcessing (buffer);
}

template <typename T>
void Oversampler<T>::reset()
{
    for (auto* const proc : processors)
        proc->reset();
}

template class Oversampler<float>;
template class Oversampler<double>;

} // namespace element
