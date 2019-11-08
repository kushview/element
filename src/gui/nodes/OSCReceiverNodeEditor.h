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

#include "engine/nodes/OSCProcessor.h"
#include "engine/nodes/OSCReceiverNode.h"
#include "gui/ViewHelpers.h"
#include "gui/nodes/NodeEditorComponent.h"
#include "gui/widgets/LogListBox.h"

namespace Element {

class OSCProcessor;

class OSCReceiverLogListBox : public LogListBox
{
public:
    OSCReceiverLogListBox()
    {
        setOpaque (true);
    }
    ~OSCReceiverLogListBox() {};

    void paint (Graphics& g)
    {
        // In case parent background is different
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void addOSCMessage (const OSCMessage& message, int level = 0)
    {
        OSCAddressPattern addressPattern = message.getAddressPattern();
        String log;

        log = indent (level)
                        + addressPattern.toString()
                        + " ";

        if (! message.isEmpty())
        {
            int i = 0;
            for (auto& arg : message)
            {
                if (i > 0) log += ", ";
                log += OscProcessor::getOSCArgumentAsString (arg);
                i++;
            }
        }

        addMessage (log);
    }

    void addOSCBundle (const OSCBundle& bundle, int level = 0)
    {
        addMessage (String(indent (level)
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
        addMessage ("Invalid format of " + String(dataSize) + " bytes");
    }

private:
    static String indent (int level)
    {
        return String().paddedRight (' ', 2 * level);
    }
};

class OSCReceiverNodeEditor : public NodeEditorComponent,
                              private OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>
{
public:
    OSCReceiverNodeEditor (const Node&);
    virtual ~OSCReceiverNodeEditor();

    void paint (Graphics&) override;
    void resized() override;
    void resetBounds (int width, int height);

private:
    OSCReceiverLogListBox oscReceiverLog;
    ReferenceCountedObjectPtr<OSCReceiverNode> oscReceiverNodePtr;

    Label hostNameLabel      { {}, "Host" };
    Label hostNameField      { {}, "127.0.0.1" };
    Label portNumberLabel    { {}, "Port" };
    Label portNumberField    { {}, "9000" };

    TextButton connectButton { "Connect" };
    TextButton pauseButton { "Pause" };
    TextButton clearButton   { "Clear" };
    Label connectionStatusLabel;

    bool paused = false;
    bool connected = false;

    int currentPortNumber = -1;
    String currentHostName = "";

    void connectButtonClicked();
    void pauseButtonClicked();
    void clearButtonClicked();

    void updateConnectButton();
    void updateConnectionStatusLabel();
    void updatePauseButton();

    void connect();
    void disconnect();

    void oscMessageReceived (const OSCMessage& message) override;
    void oscBundleReceived (const OSCBundle& bundle) override;

    void handleConnectError (int failedPort);
    void handleDisconnectError();
    void handleInvalidPortNumberEntered();
};

}
