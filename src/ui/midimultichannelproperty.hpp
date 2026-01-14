// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/signals.hpp>
#include <element/ui/style.hpp>

#include "ui/buttons.hpp"
#include "ui/patchmatrix.hpp"
#include "matrixstate.hpp"

namespace element {

class MidiMultiChannelPropertyComponent : public PropertyComponent
{
public:
    MidiMultiChannelPropertyComponent()
        : PropertyComponent (TRANS ("MIDI Channel")),
          matrix_2x8 (*this),
          matrix_1x16 (*this),
          layout (*this)
    {
        addAndMakeVisible (layout);
        refresh();
    }

    virtual ~MidiMultiChannelPropertyComponent() noexcept {}

    inline virtual void refresh() override
    {
        resized();
    }

    /** @internal */
    inline void resized() override
    {
        setPreferredHeight (getWidth() <= 600 ? 18 * 3 : 18 * 2);
        PropertyComponent::resized();
        layout.updateMatrix();
    }

    Signal<void()> changed;

    void setChannels (const BigInteger& ch)
    {
        channels = ch;
        layout.updateMatrix();
        channelsValue.setValue (channels.toMemoryBlock());
        changed();
    }

    const BigInteger& getChannels() const { return channels; }
    Value& getChannelsValue() { return channelsValue; }

private:
    BigInteger channels;
    Value channelsValue;

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
        changed();
    }

    class MatrixBase : public PatchMatrixComponent
    {
    public:
        MatrixBase (MidiMultiChannelPropertyComponent& o, const int r, const int c)
            : owner (o), state (r, c)
        {
            setMatrixCellSize (18, 18);
        }

        virtual ~MatrixBase() noexcept {}

        int getNumRows() override { return state.getNumRows(); }
        int getNumColumns() override { return state.getNumColumns(); }

        void paintMatrixCell (Graphics& g, const int width, const int height, const int row, const int col) override
        {
            if (state.connected (row, col))
            {
                if (owner.layout.omni.getToggleState())
                    g.setColour (Colors::widgetBackgroundColor.darker (0.1f));
                else
                    g.setColour (Colors::toggleGreen);
            }
            else
            {
                g.setColour (Colors::widgetBackgroundColor.brighter());
            }

            g.fillRect (1, 1, width - 2, height - 2);
            g.setFont (12.f);
            g.setColour (Colours::black);
            g.drawText (var (state.getIndexForCell (row, col) + 1).toString(), 0, 0, width, height, Justification::centred);
        }

        void matrixCellClicked (const int row, const int col, const MouseEvent& ev) override
        {
            if (owner.layout.omni.getToggleState())
                return;
            state.toggleCell (row, col);
            owner.updateChannels (state);
            repaint();
        }

        MidiMultiChannelPropertyComponent& owner;
        MatrixState state;
    };

    class Matrix_2x8 : public MatrixBase
    {
    public:
        Matrix_2x8 (MidiMultiChannelPropertyComponent& o) : MatrixBase (o, 2, 8) {}
    } matrix_2x8;

    class Matrix_1x16 : public MatrixBase
    {
    public:
        Matrix_1x16 (MidiMultiChannelPropertyComponent& o) : MatrixBase (o, 1, 16) {}
    } matrix_1x16;

    class Layout : public Component,
                   public Button::Listener
    {
    public:
        Layout (MidiMultiChannelPropertyComponent& o)
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

        MidiMultiChannelPropertyComponent& owner;
        Matrix_1x16 matrix116;
        Matrix_2x8 matrix28;
        SettingButton omni;

        void buttonClicked (Button*) override
        {
            omni.setToggleState (! omni.getToggleState(), dontSendNotification);
            owner.channels.setBit (0, omni.getToggleState());
            owner.channelsValue.setValue (owner.channels.toMemoryBlock());
            owner.changed();

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
                matrix28.setMatrixCellSize (jmax (10, getWidth() / 8), 18);
            }
            else
            {
                matrix116.setVisible (true);
                matrix116.setMatrixCellSize (jmax (10, getWidth() / 16), 18);
            }

            resized();
        }

        void resized() override
        {
            auto r (getLocalBounds());
            omni.setBounds (r.removeFromTop (18).reduced (1));
            r.removeFromTop (2);
            if (matrix28.isVisible())
                matrix28.setBounds (r);
            else
                matrix116.setBounds (r);
        }
    } layout;
};

} // namespace element
