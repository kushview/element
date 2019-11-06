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

#include "engine/nodes/OSCReceiverNode.h"
#include "gui/ViewHelpers.h"
#include "gui/nodes/NodeEditorComponent.h"
#include "gui/widgets/LogListBox.h"

namespace Element {

class OSCReceiverLogListBox : public LogListBox
{
public:
    OSCReceiverLogListBox() {}
    ~OSCReceiverLogListBox() {};

    void addOSCMessage (const OSCMessage& message, int level = 0)
    {
        addMessage (getIndentationString (level)
                        + "- osc message, address = '"
                        + message.getAddressPattern().toString()
                        + "', "
                        + String (message.size())
                        + " argument(s)");

        if (! message.isEmpty())
        {
            for (auto& arg : message)
                addOSCMessageArgument (arg, level + 1);
        }

        triggerAsyncUpdate();
    }

    //==============================================================================
    void addOSCBundle (const OSCBundle& bundle, int level = 0)
    {
        OSCTimeTag timeTag = bundle.getTimeTag();

        addMessage (String(getIndentationString (level)
                        + "- osc bundle, time tag = "
                        + timeTag.toTime().toString (true, true, true, true)));

        for (auto& element : bundle)
        {
            if (element.isMessage())
                addOSCMessage (element.getMessage(), level + 1);
            else if (element.isBundle())
                addOSCBundle (element.getBundle(), level + 1);
        }

        triggerAsyncUpdate();
    }

    //==============================================================================
    void addOSCMessageArgument (const OSCArgument& arg, int level)
    {
        String typeAsString;
        String valueAsString;

        if (arg.isFloat32())
        {
            typeAsString = "float32";
            valueAsString = String (arg.getFloat32());
        }
        else if (arg.isInt32())
        {
            typeAsString = "int32";
            valueAsString = String (arg.getInt32());
        }
        else if (arg.isString())
        {
            typeAsString = "string";
            valueAsString = arg.getString();
        }
        else if (arg.isBlob())
        {
            typeAsString = "blob";
            auto& blob = arg.getBlob();
            valueAsString = String::fromUTF8 ((const char*) blob.getData(), (int) blob.getSize());
        }
        else
        {
            typeAsString = "(unknown)";
        }

        addMessage (String(getIndentationString (level + 1) + "- " + typeAsString.paddedRight(' ', 12) + valueAsString));
    }

    void addInvalidOSCPacket (const char* /* data */, int dataSize)
    {
        addMessage ("- (" + String(dataSize) + "bytes with invalid format)");
    }

private:
    static String getIndentationString (int level)
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

    void paint (Graphics&) override {};
    void resized() override;

    void bindHandlers();
    void unbindHandlers();

private:
    OSCReceiverLogListBox oscReceiverLog;
    OSCReceiver oscReceiver;

    Label portNumberLabel    { {}, "OSC Receiver Port: " };
    Label portNumberField    { {}, "9001" };
    TextButton connectButton { "Connect" };
    TextButton clearButton   { "Clear" };
    Label connectionStatusLabel;

    int currentPortNumber = -1;

    void connectButtonClicked();
    void clearButtonClicked();
    void oscMessageReceived (const OSCMessage& message) override;
    void oscBundleReceived (const OSCBundle& bundle) override;
    void connect();
    void disconnect();
    void handleConnectError (int failedPort);
    void handleDisconnectError();
    void handleInvalidPortNumberEntered();
    bool isConnected() const;
    bool isValidOscPort (int port) const;

    void updateConnectionStatusLabel();
};

}
