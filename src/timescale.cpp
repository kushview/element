/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "timescale.hpp"

namespace element {

void TimeScale::reset()
{
    mNodes.setScoped (true);
    mMarkers.setScoped (true);

    // Clear/reset location-markers...
    mMarkers.clear();
    mMarkerCursor.reset();

    // Clear/reset tempo-map...
    mNodes.clear();
    mCursor.reset();

    // There must always be one node, always.
    addNode (0);

    // Commit new scale...
    updateScale();
}

void TimeScale::clear()
{
    mSnapPerBeat = 4;
    mHorizontalZoom = 100;
    mVerticalZoom = 100;

    mDisplayFmt = Frames;

    mSampleRate = 44100;
    mTicksPerBeat = 960;
    mPixelsPerBeat = 32;

    // Clear/reset tempo-map...
    reset();
}

void TimeScale::sync (const TimeScale& ts)
{
    // Copy master parameters...
    mSampleRate = ts.mSampleRate;
    mTicksPerBeat = ts.mTicksPerBeat;
    mPixelsPerBeat = ts.mPixelsPerBeat;

    // Copy location markers...
    mMarkers.clear();
    Marker* other_marker = ts.mMarkers.first();
    while (other_marker)
    {
        mMarkers.append (new Marker (*other_marker));
        other_marker = other_marker->next();
    }

    mMarkerCursor.reset();

    // Copy tempo-map nodes...
    mNodes.clear();
    Node* other = ts.nodes().first();
    while (other)
    {
        mNodes.append (new Node (this, other->frame, other->tempo, other->beatType, other->beatsPerBar, other->beatDivisor));
        other = other->next();
    }

    mCursor.reset();
    updateScale();
}

TimeScale& TimeScale::copyFrom (const TimeScale& ts)
{
    if (&ts != this)
    {
        mNodes.setScoped (true);
        mMarkers.setScoped (true);

        mSampleRate = ts.mSampleRate;
        mSnapPerBeat = ts.mSnapPerBeat;
        mHorizontalZoom = ts.mHorizontalZoom;
        mVerticalZoom = ts.mVerticalZoom;
        mDisplayFmt = ts.mDisplayFmt;

        // Sync/copy tempo-map nodes...
        sync (ts);
    }

    return *this;
}

void TimeScale::Node::update()
{
    ticksPerBeat = ts->ticksPerBeat();
    tickRate = tempo * ticksPerBeat;
    beatRate = tempo;

    if (beatDivisor > beatType)
    {
        unsigned short n = (beatDivisor - beatType);
        ticksPerBeat >>= n;
        beatRate *= float (1 << n);
    }
    else if (beatDivisor < beatType)
    {
        unsigned short n = (beatType - beatDivisor);
        ticksPerBeat <<= n;
        beatRate /= float (1 << n);
    }
}

void TimeScale::Node::reset (TimeScale::Node* node)
{
    if (bar > node->bar)
        frame = node->frameFromBar (bar);
    else
        bar = node->barFromFrame (frame);

    beat = node->beatFromFrame (frame);
    tick = node->tickFromFrame (frame);
    pixel = ts->pixelFromFrame (frame);
}

void TimeScale::Node::setTempoEx (float extempo, unsigned short beattype_ex)
{
    if (beattype_ex > beatType)
        extempo /= float (1 << (beattype_ex - beatType));
    else if (beatType > beattype_ex)
        extempo *= float (1 << (beatType - beattype_ex));

    tempo = extempo;
}

float TimeScale::Node::tempoEx (unsigned short beatType_) const
{
    float extempo = tempo;

    if (beatType > beatType_)
        extempo /= float (1 << (beatType - beatType_));
    else if (beatType_ > beatType)
        extempo *= float (1 << (beatType_ - beatType));

    return extempo;
}

uint64 TimeScale::Node::tickSnap (uint64 tick_, unsigned short p) const
{
    uint64 ticksnap = tick_ - tick;
    if (ts->snapPerBeat() > 0)
    {
        uint64 q = ticksPerBeat / ts->snapPerBeat();
        ticksnap = q * ((ticksnap + (q >> p)) / q);
    }
    return tick + ticksnap;
}

void TimeScale::Cursor::reset (TimeScale::Node* n)
{
    node = (n ? n : ts->nodes().first());
}

TimeScale::Node* TimeScale::Cursor::seekFrame (uint64 iFrame) const
{
    if (node == 0)
    {
        node = ts->nodes().first();
        if (node == 0)
            return 0;
    }

    if (iFrame > node->frame)
    {
        // Seek frame forward...
        while (node && node->next() && iFrame >= (node->next())->frame)
            node = node->next();
    }
    else if (iFrame < node->frame)
    {
        // Seek frame backward...
        while (node && node->frame > iFrame)
            node = node->prev();
        if (node == 0)
            node = ts->nodes().first();
    }

    return node;
}

TimeScale::Node* TimeScale::Cursor::seekBar (unsigned short sbar) const
{
    if (node == 0)
    {
        node = ts->nodes().first();
        if (node == 0)
            return 0;
    }

    if (sbar > node->bar)
    {
        // Seek bar forward...
        while (node && node->next() && sbar >= (node->next())->bar)
            node = node->next();
    }
    else if (sbar < node->bar)
    {
        // Seek bar backward...
        while (node && node->bar > sbar)
            node = node->prev();
        if (node == 0)
            node = ts->nodes().first();
    }

    return node;
}

TimeScale::Node* TimeScale::Cursor::seekBeat (unsigned int sbeat) const
{
    if (node == 0)
    {
        node = ts->nodes().first();
        if (node == 0)
            return 0;
    }

    if (sbeat > node->beat)
    {
        // Seek beat forward...
        while (node && node->next() && sbeat >= (node->next())->beat)
            node = node->next();
    }
    else if (sbeat < node->beat)
    {
        // Seek beat backward...
        while (node && node->beat > sbeat)
            node = node->prev();
        if (node == 0)
            node = ts->nodes().first();
    }

    return node;
}

TimeScale::Node* TimeScale::Cursor::seekTick (uint64 stick) const
{
    if (node == 0)
    {
        node = ts->nodes().first();
        if (node == 0)
            return 0;
    }

    if (stick > node->tick)
    {
        // Seek tick forward...
        while (node && node->next() && stick >= (node->next())->tick)
            node = node->next();
    }
    else if (stick < node->tick)
    {
        // Seek tick backward...
        while (node && node->tick > stick)
            node = node->prev();
        if (node == 0)
            node = ts->nodes().first();
    }

    return node;
}

TimeScale::Node* TimeScale::Cursor::seekPixel (int px) const
{
    if (node == 0)
    {
        node = ts->nodes().first();
        if (node == 0)
            return 0;
    }

    if (px > node->pixel)
    {
        // Seek pixel forward...
        while (node && node->next() && px >= (node->next())->pixel)
            node = node->next();
    }
    else if (px < node->pixel)
    {
        // Seek tick backward...
        while (node && node->pixel > px)
            node = node->prev();
        if (node == 0)
            node = ts->nodes().first();
    }

    return node;
}

TimeScale::Node* TimeScale::addNode (uint64 frame_, float tempo_, unsigned short beat_type_, unsigned short beats_per_bar_, unsigned short beat_divisor_)
{
    Node* node = 0;

    // Seek for the nearest preceding node...
    Node* prev = mCursor.seekFrame (frame_);
    // Snap frame to nearest bar...
    if (prev)
    {
        frame_ = prev->frameSnapToBar (frame_);
        prev = mCursor.seekFrame (frame_);
    }

    // Either update existing node or add new one...
    Node* next = (prev ? prev->next() : 0);
    if (prev && prev->frame == frame_)
    {
        // Update exact matching node...
        node = prev;
        node->tempo = tempo_;
        node->beatType = beat_type_;
        node->beatsPerBar = beats_per_bar_;
        node->beatDivisor = beat_divisor_;
    }
    else if (prev && prev->tempo == tempo_
             && prev->beatType == beat_type_
             && prev->beatsPerBar == beats_per_bar_
             && prev->beatDivisor == beat_divisor_)
    {
        // No need for a new node...
        return prev;
    }
    else if (next && next->tempo == tempo_
             && next->beatType == beat_type_
             && next->beatsPerBar == beats_per_bar_
             && next->beatDivisor == beat_divisor_)
    {
        // Update next exact matching node...
        node = next;
        node->frame = frame_;
        node->bar = 0;
    }
    else
    {
        // Add/insert a new node...
        node = new Node (this, frame_, tempo_, beat_type_, beats_per_bar_, beat_divisor_);
        if (prev)
            mNodes.insertAfter (node, prev);
        else
            mNodes.append (node);
    }

    // Update coefficients and positioning thereafter...
    updateNode (node);

    return node;
}

void TimeScale::updateNode (TimeScale::Node* node)
{
    // Update coefficients...
    node->update();

    // Relocate internal cursor...
    mCursor.reset (node);

    // Update positioning on all nodes thereafter...
    Node* next = node;
    Node* prev = next->prev();
    while (next)
    {
        if (prev)
            next->reset (prev);

        prev = next;
        next = next->next();
    }

    // And update marker/bar positions too...
    updateMarkers (node->prev());
}

void TimeScale::removeNode (TimeScale::Node* node)
{
    // Don't ever remove the very first node...
    Node* node_prev = node->prev();
    if (node_prev == 0)
        return;

    // Relocate internal cursor...
    mCursor.reset (node_prev);

    // Update positioning on all nodes thereafter...
    Node* prev = node_prev;
    Node* next = node->next();
    while (next)
    {
        if (prev)
            next->reset (prev);

        prev = next;
        next = next->next();
    }

    // Actually remove/unlink the node...
    mNodes.remove (node);

    // Then update marker/bar positions too...
    updateMarkers (prev);
}

void TimeScale::updateScale()
{
    // Update time-map independent coefficients...
    mPixelRate = 1.20f * float (mHorizontalZoom * mPixelsPerBeat);
    mFrameRate = 60.0f * float (mSampleRate);

    // Update all nodes thereafter...
    Node* prev = 0;
    Node* next = mNodes.first();
    while (next)
    {
        next->update();
        if (prev)
            next->reset (prev);

        prev = next;
        next = next->next();
    }

    // Also update all marker/bar positions too...
    updateMarkers (mNodes.first());
}

// Beat divisor (snap index) map.
static unsigned short s_snap_per_beat[] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    12,
    14,
    16,
    21,
    24,
    28,
    32,
    48,
    64,
    96
};

const int c_snap_item_count = sizeof (s_snap_per_beat) / sizeof (unsigned short);

// Beat divisor (snap index) accessors.
unsigned short TimeScale::snapFromIndex (int index)
{
    return s_snap_per_beat[index];
}

// Beat divisor (snap index) accessors.
int TimeScale::indexFromSnap (unsigned short snap_per_beat)
{
    for (int snap = 0; snap < c_snap_item_count; ++snap)
    {
        if (s_snap_per_beat[snap] == snap_per_beat)
            return snap;
    }

    return 0;
}

// Tick/Frame range conversion (delta conversion).
uint64 TimeScale::frameFromTickRange (uint64 iTickStart, uint64 iTickEnd)
{
    Node* pNode = mCursor.seekTick (iTickStart);
    uint64 iFrameStart = (pNode ? pNode->frameFromTick (iTickStart) : 0);
    pNode = mCursor.seekTick (iTickEnd);
    uint64 iFrameEnd = (pNode ? pNode->frameFromTick (iTickEnd) : 0);
    return (iFrameEnd > iFrameStart ? iFrameEnd - iFrameStart : 0);
}

uint64 TimeScale::tickFromFrameRange (uint64 iFrameStart, uint64 iFrameEnd)
{
    Node* pNode = mCursor.seekFrame (iFrameStart);
    uint64 iTickStart = (pNode ? pNode->tickFromFrame (iFrameStart) : 0);
    pNode = mCursor.seekFrame (iFrameEnd);
    uint64 iTickEnd = (pNode ? pNode->tickFromFrame (iFrameEnd) : 0);
    return (iTickEnd > iTickStart ? iTickEnd - iTickStart : 0);
}

// Location marker reset method.
void TimeScale::MarkerCursor::reset (TimeScale::Marker* m)
{
    marker = (m ? m : ts->markers().first());
}

// Location marker seek methods.
TimeScale::Marker* TimeScale::MarkerCursor::seekFrame (uint64 iFrame)
{
    if (marker == 0)
    {
        marker = ts->markers().first();
        if (marker == 0)
            return 0;
    }

    if (iFrame > marker->frame)
    {
        // Seek frame forward...
        while (marker && marker->next() && iFrame >= (marker->next())->frame)
            marker = marker->next();
    }
    else if (iFrame < marker->frame)
    {
        // Seek frame backward...
        while (marker && marker->frame > iFrame)
            marker = marker->prev();
        if (marker == 0)
            marker = ts->markers().first();
    }

    return marker;
}

TimeScale::Marker* TimeScale::MarkerCursor::seekBar (unsigned short iBar)
{
    return seekFrame (ts->frameFromBar (iBar));
}

TimeScale::Marker* TimeScale::MarkerCursor::seekBeat (unsigned int iBeat)
{
    return seekFrame (ts->frameFromBeat (iBeat));
}

TimeScale::Marker* TimeScale::MarkerCursor::seekTick (uint64 iTick)
{
    return seekFrame (ts->frameFromTick (iTick));
}

TimeScale::Marker* TimeScale::MarkerCursor::seekPixel (int x)
{
    return seekFrame (static_cast<uint64> (ts->frameFromPixel (x)));
}

// Location markers list specifics.
TimeScale::Marker* TimeScale::addMarker (uint64 target_frame, const std::string& txt, const std::string& rgb)
{
    Marker* marker = 0;

    // Snap to nearest bar...
    unsigned short nearest_bar = 0;

    Node* prev = mCursor.seekFrame (target_frame);
    if (prev)
    {
        nearest_bar = prev->barFromFrame (target_frame);
        target_frame = prev->frameFromBar (nearest_bar);
    }

    // Seek for the nearest marker...
    Marker* nearest_marker = mMarkerCursor.seekFrame (target_frame);

    // Either update existing marker or add new one...
    if (nearest_marker && nearest_marker->frame == target_frame)
    {
        // Update exact matching marker...
        marker = nearest_marker;
        marker->bar = nearest_bar;
        marker->text = txt;
        marker->color = rgb;
    }
    else
    {
        // Add/insert a new marker...
        marker = new Marker (target_frame, nearest_bar, txt, rgb);

        if (nearest_marker && nearest_marker->frame > target_frame)
            mMarkers.insertBefore (marker, nearest_marker);
        else if (nearest_marker && nearest_marker->frame < target_frame)
            mMarkers.insertAfter (marker, nearest_marker);
        else
            mMarkers.append (marker);
    }

    // Update positioning...
    updateMarker (marker);

    return marker;
}

void TimeScale::updateMarker (TimeScale::Marker* pMarker)
{
    // Relocate internal cursor...
    mMarkerCursor.reset (pMarker);
}

void TimeScale::removeMarker (TimeScale::Marker* pMarker)
{
    // Actually remove/unlink the marker
    // and relocate internal cursor...
    Marker* pMarkerPrev = pMarker->prev();
    mMarkers.remove (pMarker);
    mMarkerCursor.reset (pMarkerPrev);
}

// Update markers from given node position.
void TimeScale::updateMarkers (TimeScale::Node* pNode)
{
    if (pNode == 0)
        pNode = mNodes.first();
    if (pNode == 0)
        return;

    Marker* pMarker = mMarkerCursor.seekFrame (pNode->frame);
    while (pMarker)
    {
        while (pNode->next() && pMarker->frame > pNode->next()->frame)
            pNode = pNode->next();
        if (pMarker->frame >= pNode->frame)
            pMarker->frame = pNode->frameFromBar (pMarker->bar);
        pMarker = pMarker->next();
    }
}

} // namespace element
