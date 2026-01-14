// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later
// Author: Eliot Akira <me@eliotakira.com>

#pragma once

#include <element/ui/nodeeditor.hpp>
#include <element/ui/style.hpp>

#include "nodes/oscsender.hpp"
#include "nodes/oscsendereditor.hpp"
#include "ui/viewhelpers.hpp"
#include "ui/loglistbox.hpp"
#include "utils.hpp"

namespace element {

class OSCSenderLogListBox : public LogListBox
{
public:
    OSCSenderLogListBox()
    {
        setOpaque (true);
    }
    ~OSCSenderLogListBox() {};

    void paint (Graphics& g)
    {
        // In case parent background is different
        g.fillAll (element::Colors::backgroundColor);
    }

    void addOSCMessage (const OSCMessage& message, int level = 0)
    {
        addMessage (indent (level) + Util::getOscMessageAsString (message));
    }

    void addOSCBundle (const OSCBundle& bundle, int level = 0)
    {
        addMessage (String (indent (level)
                            + "Bundle"));

        for (auto& element : bundle)
        {
            if (element.isMessage())
                addOSCMessage (element.getMessage(), level + 1);
            else if (element.isBundle())
                addOSCBundle (element.getBundle(), level + 1);
        }
    }

    void addInvalidOSCPacket (const char* /* data */, int dataSize)
    {
        addMessage ("Invalid format of " + String (dataSize) + " bytes");
    }

private:
    static String indent (int level)
    {
        return String().paddedRight (' ', 2 * level);
    }
};

class OSCSenderNodeEditor : public NodeEditor,
                            public ChangeListener,
                            private Timer
{
public:
    OSCSenderNodeEditor (const Node&);
    virtual ~OSCSenderNodeEditor();

    void paint (Graphics&) override;
    void resized() override;
    void resetBounds (int width, int height);
    void timerCallback() override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void syncUIFromNodeState();

private:
    OSCSenderLogListBox oscSenderLog;

    ReferenceCountedObjectPtr<OSCSenderNode> oscSenderNodePtr;

    Label hostNameLabel { {}, "Host" };
    Label hostNameField { {}, "127.0.0.1" };
    Label portNumberLabel { {}, "Port" };
    Slider portNumberSlider;

    TextButton connectButton { "Connect" };
    TextButton pauseButton { "Pause" };
    TextButton clearButton { "Clear" };
    Label connectionStatusLabel;

    bool paused = false;
    bool connected = false;

    int currentPortNumber = -1;
    String currentHostName = "";

    void connectButtonClicked();
    void pauseButtonClicked();
    void clearButtonClicked();
    void hostNameFieldChanged();

    void updateConnectButton();
    void updateConnectionStatusLabel();
    void updatePauseButton();
    void updateHostNameField();
    void updatePortNumberSlider();

    void connect();
    void disconnect();

    void handleConnectError (int failedPort);
    void handleDisconnectError();
    void handleInvalidPortNumberEntered();
};

} // namespace element
