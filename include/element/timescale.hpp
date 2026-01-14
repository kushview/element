// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

#include <element/linkedlist.hpp>

#if _MSC_VER
    #pragma warning(disable : 4355)
#endif

#define beatToFrame (beat, bpm, rate)

namespace element {

/** Time scaling helper class */
class TimeScale {
public:
    enum DisplayFormat {
        Frames = 0,
        Time,
        BBT
    };

    TimeScale() : mDisplayFmt (Frames), mCursor (this), mMarkerCursor (this) { clear(); }
    TimeScale (const TimeScale& ts) : mCursor (this), mMarkerCursor (this) { copyFrom (ts); }
    TimeScale& operator= (const TimeScale& ts) { return copyFrom (ts); }

    /** Reset the node list */
    void reset();

    /** Clear/sync/initialize the list */
    void clear();

    /** Sync timing values from another timescale */
    void sync (const TimeScale& ts);

    /** Copy the timescale.
        @param ts The timescale to copy from
        @note Also calls sync */
    TimeScale& copyFrom (const TimeScale& ts);

    /** Sample rate (frames per second) */
    void setSampleRate (unsigned int rate) { mSampleRate = rate; }
    unsigned int getSampleRate() const { return mSampleRate; }

    /** Resolution (ticks per quarter note; PPQN) */
    void setTicksPerBeat (unsigned short ticks) { mTicksPerBeat = ticks; }
    unsigned short ticksPerBeat() const { return mTicksPerBeat; }
    unsigned short ppq() const { return mTicksPerBeat; }

    /** Pixels Per Beat */
    void setPixelsPerBeat (unsigned short ppb) { mPixelsPerBeat = ppb; }
    unsigned short pixelsPerBeat() const { return mPixelsPerBeat; }

    /** Set snaps per beat */
    void setSnapPerBeat (unsigned short snap) { mSnapPerBeat = snap; }
    unsigned short snapPerBeat() const { return mSnapPerBeat; }

    /** Horizontal zoom */
    void setHorizontalZoom (unsigned short hZoom) { mHorizontalZoom = hZoom; }
    unsigned short horizontalZoom() const { return mHorizontalZoom; }

    /** Vertical zoom */
    void setVerticalZoom (unsigned short vzoom) { mVerticalZoom = vzoom; }
    unsigned short verticalZoom() const { return mVerticalZoom; }

    // Fastest rounding-from-float helper.
    static uint64_t uroundf (float x) { return static_cast<uint64_t> (x >= 0.0f ? x + 0.5f : x - 0.5f); }
    static int64_t roundf (float x) { return static_cast<int64_t> (x >= 0.0f ? x + 0.5f : x - 0.5f); }

    // Beat divisor (snap index) accessors.
    static unsigned short snapFromIndex (int index);
    static int indexFromSnap (unsigned short snap);

    // Time scale node declaration.
    class Node : public LinkedList<Node>::Link {
    public:
        // Constructor.
        Node (TimeScale* timescale, uint64_t frame_ = 0, float tempo_ = 120.0f, unsigned short beattype_ = 2, unsigned short beats_per_bar_ = 4, unsigned short beat_divisor_ = 2)
            : frame (frame_), bar (0), beat (0), tick (0), pixel (0), tempo (tempo_), beatType (beattype_), beatsPerBar (beats_per_bar_), beatDivisor (beat_divisor_), ticksPerBeat (0), ts (timescale), tickRate (1.0f), beatRate (1.0f)
        {
        }

        // Update node scale coefficients.
        void update();

        // Update node position metrics.
        void reset (Node* node);

        // Tempo accessor/convertors.
        void setTempoEx (float tempo, unsigned short beatType = 2);
        float tempoEx (unsigned short beatType = 2) const;

        // Frame/bar convertors.
        unsigned short barFromFrame (uint64_t iFrame) const
        {
            return bar + (unsigned short) uroundf ((beatRate * (iFrame - frame)) / (ts->frameRate() * beatsPerBar));
        }

        uint64_t frameFromBar (unsigned short iBar) const
        {
            return frame + (uint64_t) uroundf ((ts->frameRate() * beatsPerBar * (iBar - bar)) / beatRate);
        }

        // Frame/beat convertors.
        unsigned int beatFromFrame (uint64_t iFrame) const
        {
            return beat + (unsigned int) uroundf ((beatRate * (iFrame - frame)) / ts->frameRate());
        }

        uint64_t frameFromBeat (unsigned int iBeat) const { return frame + (uint64_t) uroundf ((ts->frameRate() * (iBeat - beat)) / beatRate); }

        // Frame/tick convertors.
        uint64_t tickFromFrame (uint64_t iFrame) const
        {
            return tick + (uint64_t) uroundf ((tickRate * (iFrame - frame)) / ts->frameRate());
        }

        uint64_t frameFromTick (uint64_t iTick) const { return frame + (uint64_t) uroundf ((ts->frameRate() * (iTick - tick)) / tickRate); }

        // Tick/beat convertors.
        unsigned int beatFromTick (uint64_t iTick) const { return beat + (unsigned int) ((iTick - tick) / ticksPerBeat); }
        uint64_t tickFromBeat (unsigned int iBeat) const { return tick + (uint64_t) (ticksPerBeat * (iBeat - beat)); }

        // Tick/bar convertors.
        unsigned short barFromTick (uint64_t iTick) const { return bar + (unsigned short) ((iTick - tick) / (ticksPerBeat * beatsPerBar)); }
        uint64_t tickFromBar (unsigned short iBar) const { return tick + (uint64_t) (ticksPerBeat * beatsPerBar * (iBar - bar)); }

        // Tick/pixel convertors.
        uint64_t tickFromPixel (int x) const { return tick + (uint64_t) uroundf ((tickRate * (x - pixel)) / ts->pixelRate()); }
        int pixelFromTick (uint64_t iTick) const { return pixel + (int) uroundf ((ts->pixelRate() * (iTick - tick)) / tickRate); }

        // Beat/pixel convertors.
        unsigned int beatFromPixel (int x) const { return beat + (unsigned int) uroundf ((beatRate * (x - pixel)) / ts->pixelRate()); }
        int pixelFromBeat (unsigned int b) const { return pixel + (int) uroundf ((ts->pixelRate() * (b - beat)) / beatRate); }

        // Pixel/beat rate convertor.
        unsigned short pixelsPerBeat() const { return (unsigned short) uroundf (ts->pixelRate() / beatRate); }

        // Bar/pixel convertors.
        unsigned short barFromPixel (int x) const { return bar + (unsigned short) uroundf ((beatRate * (x - pixel)) / (ts->pixelRate() * beatsPerBar)); }
        int pixelFromBar (unsigned short b) const { return pixel + (int) uroundf ((ts->pixelRate() * beatsPerBar * (b - bar)) / beatRate); }

        // Bar/beat convertors.
        unsigned short barFromBeat (unsigned int iBeat) const { return bar + (unsigned short) ((iBeat - beat) / beatsPerBar); }
        unsigned int beatFromBar (unsigned short iBar) const { return beat + (int) (beatsPerBar * (iBar - bar)); }

        bool beatIsBar (unsigned int iBeat) const { return ((iBeat - beat) % beatsPerBar) == 0; }

        // Frame/bar quantizer.
        uint64_t frameSnapToBar (uint64_t frame_) const { return frameFromBar (barFromFrame (frame_)); }

        // Beat snap filters.
        uint64_t tickSnap (uint64_t tick_, unsigned short p = 1) const;
        uint64_t frameSnap (uint64_t frame_) const { return frameFromTick (tickSnap (tickFromFrame (frame_))); }
        int pixelSnap (int x) const { return pixelFromTick (tickSnap (tickFromPixel (x))); }

        // Node keys.
        uint64_t frame;
        unsigned short bar;
        unsigned int beat;
        uint64_t tick;
        int pixel;

        // Node payload.
        float tempo;
        unsigned short beatType;
        unsigned short beatsPerBar;
        unsigned short beatDivisor;
        unsigned short ticksPerBeat;

    protected:
        friend class TimeScale;
        // Node owner.
        TimeScale* ts;

        // Node cached coefficients.
        float tickRate;
        float beatRate;
    };

    // Node list accessor.
    const LinkedList<Node>& nodes() const { return mNodes; }

    // To optimize and keep track of current frame
    // position, mostly like an sequence cursor/iterator.
    class Cursor {
    public:
        Cursor (TimeScale* scale) : ts (scale), node (0) {}
        TimeScale* timescale() const { return ts; }

        void reset (Node* node = 0);

        Node* seekFrame (uint64_t frame) const;
        Node* seekBar (unsigned short bar) const;
        Node* seekBeat (unsigned int beat) const;
        Node* seekTick (uint64_t tick) const;
        Node* seekPixel (int x) const;

    protected:
        TimeScale* ts;
        mutable Node* node;
    };

    Cursor& cursor() { return mCursor; }

    // Node list specifics.
    Node* addNode (uint64_t iFrame = 0, float fTempo = 120.0f, unsigned short iBeatType = 2, unsigned short iBeatsPerBar = 4, unsigned short iBeatDivisor = 2);

    void updateNode (Node* node);
    void removeNode (Node* node);

    /** Update the timescale as a whole */
    void updateScale();

    // Frame/pixel convertors.
    int pixelFromFrame (int64_t frame) const { return (int) roundf ((mPixelRate * frame) / mFrameRate); }
    int64_t frameFromPixel (int x) const { return roundf ((mFrameRate * x) / mPixelRate); }

    // Frame/bar general converters.
    unsigned short barFromFrame (uint64_t frame)
    {
        Node* node = mCursor.seekFrame (frame);
        return (node ? node->barFromFrame (frame) : 0);
    }

    uint64_t frameFromBar (unsigned short bar)
    {
        Node* node = mCursor.seekBar (bar);
        return (node ? node->frameFromBar (bar) : 0);
    }

    // Frame/beat general converters.
    unsigned int beatFromFrame (uint64_t frame)
    {
        Node* node = mCursor.seekFrame (frame);
        return (node ? node->beatFromFrame (frame) : 0);
    }

    uint64_t frameFromBeat (unsigned int beat)
    {
        Node* node = mCursor.seekBeat (beat);
        return (node ? node->frameFromBeat (beat) : 0);
    }

    // Frame/tick general converters.
    uint64_t tickFromFrame (uint64_t frame) const
    {
        Node* node = mCursor.seekFrame (frame);
        return (node ? node->tickFromFrame (frame) : 0);
    }

    uint64_t frameFromTick (uint64_t tick) const
    {
        Node* node = mCursor.seekTick (tick);
        return (node ? node->frameFromTick (tick) : 0);
    }

    // Tick/pixel general converters.
    uint64_t tickFromPixel (int x) const
    {
        Node* node = mCursor.seekPixel (x);
        return (node ? node->tickFromPixel (x) : 0);
    }

    int pixelFromTick (uint64_t tick) const
    {
        Node* node = mCursor.seekTick (tick);
        return (node ? node->pixelFromTick (tick) : 0);
    }

    // Beat/pixel composite converters.
    unsigned int beatFromPixel (int x)
    {
        Node* node = mCursor.seekPixel (x);
        return (node ? node->beatFromPixel (x) : 0);
    }

    int pixelFromBeat (unsigned int iBeat)
    {
        Node* node = mCursor.seekBeat (iBeat);
        return (node ? node->pixelFromBeat (iBeat) : 0);
    }

    // Bar/beat predicate.
    bool beatIsBar (unsigned int beat)
    {
        Node* node = mCursor.seekBeat (beat);
        return (node ? node->beatIsBar (beat) : false);
    }

    // Snap functions.
    uint64_t tickSnap (uint64_t iTick)
    {
        Node* node = mCursor.seekTick (iTick);
        return (node ? node->tickSnap (iTick) : iTick);
    }

    uint64_t frameSnap (uint64_t frame)
    {
        Node* node = mCursor.seekFrame (frame);
        return (node ? node->frameSnap (frame) : frame);
    }

    int pixelSnap (int x) const
    {
        Node* node = mCursor.seekPixel (x);
        return (node ? node->pixelSnap (x) : x);
    }

    // Display-format accessors.
    void setDisplayFormat (DisplayFormat dfmt) { mDisplayFmt = dfmt; }
    DisplayFormat displayFormat() const { return mDisplayFmt; }

#if 0
	// Convert frames to time string and vice-versa.
    QString textFromFrame (uint64_t iFrame, bool bDelta = false, uint64_t iDelta = 0);
    uint64_t frameFromText (const QString& sText, bool bDelta = false, uint64_t iFrame = 0);

	// Convert ticks to time string and vice-versa.
    QString textFromTick (uint64_t iTick, bool bDelta = false, uint64_t iDelta = 0);
    uint64_t tickFromText (const QString& sText, bool bDelta = false, uint64_t iTick = 0);
#endif

    /** Set the tempo
        @param tempo The tempo to use (BPM) */
    void setTempo (float tempo)
    {
        if (Node* node = mNodes.first())
            node->tempo = tempo;
    }

    /** Return the timescale's current tempo (at the first node) */
    float getTempo() const
    {
        Node* node = mNodes.first();
        return (node ? node->tempo : 120.0f);
    }

    // Tempo convertors (default's quarter notes per minute)
    void setTempoEx (float tempo, unsigned short beat_type = 2)
    {
        Node* node = mNodes.first();
        if (node)
            node->setTempoEx (tempo, beat_type);
    }

    float tempoEx (unsigned short iBeatType = 2) const
    {
        Node* node = mNodes.first();
        return (node ? node->tempoEx (iBeatType) : 120.0f);
    }

    // Tempo beat type (if not standard 2 = quarter note)
    void setBeatType (unsigned short beatType)
    {
        Node* node = mNodes.first();
        if (node)
            node->beatType = beatType;
    }

    unsigned short beatType() const
    {
        Node* node = mNodes.first();
        return (node ? node->beatType : 2);
    }

    /** Set beats per bar (time signature numerator)
        @param bpb The time signature's numerator */
    void setBeatsPerBar (unsigned short bpb)
    {
        if (Node* node = mNodes.first())
            node->beatsPerBar = bpb;
    }

    unsigned short beatsPerBar() const
    {
        Node* node = mNodes.first();
        return (node ? node->beatsPerBar : 4);
    }

    // Time signature (denominator)
    void setBeatDivisor (unsigned short divisor)
    {
        if (Node* node = mNodes.first())
            node->beatDivisor = divisor;
    }

    unsigned short beatDivisor() const
    {
        Node* node = mNodes.first();
        return (node ? node->beatDivisor : 2);
    }

    float beatRate() const
    {
        Node* node = mNodes.first();
        return (node ? node->beatRate : 60.f);
    }

    // Tick/Frame range conversion (delta conversion).
    uint64_t frameFromTickRange (uint64_t tickStart, uint64_t tickEnd);
    uint64_t tickFromFrameRange (uint64_t frameStart, uint64_t frameEnd);

#if 1
    // Location marker declaration.
    class Marker : public LinkedList<Marker>::Link {
    public:
        // Constructor.
        Marker (uint64_t iFrame, unsigned short iBar, const std::string& sText, const std::string& rgbColor = std::string ("#545454"))
            : frame (iFrame), bar (iBar), text (sText), color (rgbColor) {}

        // Copy constructor.
        Marker (const Marker& marker) : frame (marker.frame),
                                        bar (marker.bar),
                                        text (marker.text),
                                        color (marker.color) {}

        // Marker keys.
        uint64_t frame;
        unsigned short bar;

        // Marker payload.
        std::string text;
        std::string color;
    };

    // To optimize and keep track of current frame
    // position, mostly like an sequence cursor/iterator.
    class MarkerCursor {
    public:
        // Constructor.
        MarkerCursor (TimeScale* tscale) : ts (tscale), marker (0) {}

        // Time scale accessor.
        TimeScale* timeScale() const { return ts; }

        // Reset method.
        void reset (Marker* marker = 0);

        // Seek methods.
        Marker* seekFrame (uint64_t iFrame);
        Marker* seekBar (unsigned short iBar);
        Marker* seekBeat (unsigned int iBeat);
        Marker* seekTick (uint64_t iTick);
        Marker* seekPixel (int x);

        // Notable markers accessors
        Marker* first() const { return ts->mMarkers.first(); }
        Marker* last() const { return ts->mMarkers.last(); }

    protected:
        // Member variables.
        TimeScale* ts;
        Marker* marker;
    };

    // Markers list accessor.
    MarkerCursor& markers() { return mMarkerCursor; }

    // Marker list specifics.
    Marker* addMarker (uint64_t iFrame, const std::string& text, const std::string& color = std::string ("#000000"));
    void updateMarker (Marker* marker);
    void removeMarker (Marker* marker);

    // Update markers from given node position.
    void updateMarkers (Node* n);

#endif

protected:
    // Tempo-map independent coefficients.
    float pixelRate() const { return mPixelRate; }
    float frameRate() const { return mFrameRate; }

private:
    unsigned short mSnapPerBeat;    ///< Snap per beat (divisor).
    unsigned short mHorizontalZoom; ///< Horizontal zoom factor.
    unsigned short mVerticalZoom;   ///< Vertical zoom factor.

    DisplayFormat mDisplayFmt; ///< Textual display format.

    unsigned int mSampleRate;      ///< Sample rate (frames per second)
    unsigned short mTicksPerBeat;  ///< Tticks per quarter note (PPQN)
    unsigned short mPixelsPerBeat; ///< Pixels per beat (width).

    // Tempo-map node list.
    LinkedList<Node> mNodes;

    // Internal node cursor.
    Cursor mCursor;

    // Tempo-map independent coefficients.
    float mPixelRate;
    float mFrameRate;

    // Location marker list.
    LinkedList<Marker> mMarkers;

    // Internal node cursor.
    MarkerCursor mMarkerCursor;
};

} // namespace element
