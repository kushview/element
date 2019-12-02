
#pragma once

namespace Element {
namespace Lua {

struct MidiBufferForeach
{
    MidiBufferForeach (const MidiBuffer& buffer)
        : iter (buffer) {}
    MidiBufferForeach (const MidiBufferForeach&& o)
        : iter(std::move (o.iter)), frame (o.frame), message (o.message)
    {}

    MidiMessage* operator()() const
    {
        if (iter.getNextEvent (message, frame))
        {
            message.setTimeStamp (static_cast<double> (frame));
            return &message;
        }

        return nullptr;
    }
    
private:
    mutable MidiBuffer::Iterator iter;
    mutable int frame { 0 };
    mutable MidiMessage message;
};

}}
