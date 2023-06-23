/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "gui/Buttons.h"
#include <element/ui/style.hpp>
#include "gui/PatchMatrixComponent.h"

#include "matrixstate.hpp"
#include <element/signals.hpp>

namespace element {

class MidiChannelSelectComponent : public Component
{
public:
    MidiChannelSelectComponent()
        : Component (TRANS ("MIDI Channel")),
          matrix_2x8 (*this),
          matrix_1x16 (*this),
          layout (*this)
    {
        addAndMakeVisible (layout);
    }

    virtual ~MidiChannelSelectComponent() noexcept {}

    inline virtual void refresh()
    {
        resized();
    }

    inline int getSuggestedHeight (const int width) const
    {
        return jmax (10, layout.preferedHeight) * (width <= 600 ? 3 : 2);
    }

    /** @internal */
    inline void resized() override
    {
        layout.setBounds (getLocalBounds());
        layout.updateMatrix();
    }

    void enablementChanged() override
    {
        Component::enablementChanged();
        repaint();
    }

    Signal<void()> changed;
    std::function<void()> onChanged;

    void setChannels (const BigInteger& ch, const bool notify = true)
    {
        channels = ch;
        layout.updateMatrix();
        channelsValue.setValue (channels.toMemoryBlock());
        if (notify)
            sendChanged();
    }

    const BigInteger& getChannels() const { return channels; }
    Value& getChannelsValue() { return channelsValue; }

private:
    BigInteger channels;
    Value channelsValue;

    void sendChanged()
    {
        changed();
        if (onChanged)
            onChanged();
    }

    void updateMatrixState (MatrixState& state)
    {
        layout.omni.setToggleState (channels[0], dontSendNotification);
        for (int r = 0; r < state.getNumRows(); ++r)
            for (int c = 0; c < state.getNumColumns(); ++c)
                state.set (r, c, channels[1 + state.getIndexForCell (r, c)]);
    }

    void updateChannels (const MatrixState& state)
    {
        channels = BigInteger();
        channels.setBit (0, layout.omni.getToggleState());
        for (int i = 0; i < state.getNumRows() * state.getNumColumns(); ++i)
            channels.setBit (1 + i, state.connectedAtIndex (i));
        channelsValue.setValue (channels.toMemoryBlock());
        sendChanged();
    }

    class MatrixBase : public PatchMatrixComponent
    {
    public:
        MatrixBase (MidiChannelSelectComponent& o, const int r, const int c)
            : owner (o), state (r, c)
        {
            setMatrixCellSize (25, 25);
        }

        virtual ~MatrixBase() noexcept {}

        int getNumRows() override { return state.getNumRows(); }
        int getNumColumns() override { return state.getNumColumns(); }

        void paintMatrixCell (Graphics& g, const int width, const int height, const int row, const int col) override
        {
            if (state.connected (row, col))
            {
                if (owner.layout.omni.getToggleState() || ! isEnabled())
                    g.setColour (Colors::widgetBackgroundColor.darker (0.1f));
                else
                    g.setColour (Colors::toggleGreen);
            }
            else
            {
                g.setColour (Colors::widgetBackgroundColor.brighter());
            }

            g.fillRect (1, 1, width - 2, height - 2);
            g.setColour (isEnabled() ? Colours::black : Colours::darkgrey);
            g.setFont (12.f);
            g.drawText (var (state.getIndexForCell (row, col) + 1).toString(),
                        0,
                        0,
                        width,
                        height,
                        Justification::centred);
        }

        void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
        {
            if (owner.layout.omni.getToggleState() || ! isEnabled())
                return;
            state.toggleCell (row, col);
            owner.updateChannels (state);
            repaint();
        }

        MidiChannelSelectComponent& owner;
        MatrixState state;
    };

    class Matrix_2x8 : public MatrixBase
    {
    public:
        Matrix_2x8 (MidiChannelSelectComponent& o) : MatrixBase (o, 2, 8) {}
    } matrix_2x8;

    class Matrix_1x16 : public MatrixBase
    {
    public:
        Matrix_1x16 (MidiChannelSelectComponent& o) : MatrixBase (o, 1, 16) {}
    } matrix_1x16;

    class Layout : public Component,
                   public Button::Listener
    {
    public:
        Layout (MidiChannelSelectComponent& o)
            : owner (o), matrix116 (o), matrix28 (o)
        {
            addAndMakeVisible (omni);
            omni.addListener (this);
            omni.setButtonText ("Omni");
            omni.setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen.withAlpha (0.9f));
            addAndMakeVisible (matrix116);
            addAndMakeVisible (matrix28);
        }

        ~Layout() noexcept
        {
            omni.removeListener (this);
        }

        MidiChannelSelectComponent& owner;
        Matrix_1x16 matrix116;
        Matrix_2x8 matrix28;
        SettingButton omni;
        int preferedHeight = 16;

        void buttonClicked (Button*) override
        {
            omni.setToggleState (! omni.getToggleState(), dontSendNotification);
            owner.channels.setBit (0, omni.getToggleState());
            owner.channelsValue.setValue (owner.channels.toMemoryBlock());
            owner.sendChanged();

            if (matrix116.isVisible())
                matrix116.repaint();
            if (matrix28.isVisible())
                matrix28.repaint();
        }

        void updateMatrix()
        {
            matrix116.setVisible (false);
            matrix28.setVisible (false);
            owner.updateMatrixState (matrix116.state);
            owner.updateMatrixState (matrix28.state);

            if (owner.getWidth() <= 600)
            {
                matrix28.setVisible (true);
                matrix28.setMatrixCellSize (jmax (10, getWidth() / 8), preferedHeight);
            }
            else
            {
                matrix116.setVisible (true);
                matrix116.setMatrixCellSize (jmax (10, getWidth() / 16), preferedHeight);
            }

            resized();
            repaint();
        }

        void resized() override
        {
            auto r (getLocalBounds());
            omni.setBounds (r.removeFromTop (jmax (10, preferedHeight)).reduced (1));
            r.removeFromTop (2);
            if (matrix28.isVisible())
                matrix28.setBounds (r);
            else
                matrix116.setBounds (r);
        }
    } layout;
};

} // namespace element
