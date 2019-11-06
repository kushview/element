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
    OSCReceiverLogListBox() {

        setOpaque (true);

    }
    ~OSCReceiverLogListBox() {};

    void paint (Graphics& g)
    {
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void addOSCMessage (const OSCMessage& message, int level = 0)
    {
        String log;
        // message.size()

        log = indent (level)
                        + message.getAddressPattern().toString()
                        + " ";

        if (! message.isEmpty())
        {
            int i = 0;
            for (auto& arg : message) {
                if (i > 0) log += ", ";
                log += getOSCArgumentAsString (arg);
                i++;
            }
        }

        addMessage (log);
    }

    //==============================================================================
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

    //==============================================================================
    String getOSCArgumentAsString (const OSCArgument& arg)
    {
        String type;
        String value;

        if (arg.isFloat32())
        {
            type = "float32";
            value = String (arg.getFloat32());
        }
        else if (arg.isInt32())
        {
            type = "int32";
            value = String (arg.getInt32());
        }
        else if (arg.isString())
        {
            type = "string";
            value = arg.getString();
        }
        else if (arg.isBlob())
        {
            type = "blob";
            auto& blob = arg.getBlob();
            value = String::fromUTF8 ((const char*) blob.getData(), (int) blob.getSize());
        }
        else
        {
            type = "unknown";
            //addMessage (type);
            return type;
        }
        return type + " " + value;
        //addMessage (type + " " + value);
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
                              private OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>,
                              private ComponentBoundsConstrainer
{
public:
    OSCReceiverNodeEditor (const Node&);
    virtual ~OSCReceiverNodeEditor();

    void paint (Graphics&) override;
    void resized() override;
    void resetBounds (int width, int height);

    void bindHandlers();
    void unbindHandlers();

private:
    OSCReceiverLogListBox oscReceiverLog;

    OSCReceiver oscReceiver;

    Label portNumberLabel    { {}, "Port" };
    Label portNumberField    { {}, "9000" };
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
