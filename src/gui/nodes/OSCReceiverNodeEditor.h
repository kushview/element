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

namespace Element {

class OSCReceiverLogListBox : public ListBox,
                              private ListBoxModel,
                              private AsyncUpdater
{
public:
    OSCReceiverLogListBox()
    {
        setModel (this);
    }

    ~OSCReceiverLogListBox() override = default;

    int getNumRows() override
    {
        return logList.size();
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        ignoreUnused (rowIsSelected);
        if (isPositiveAndBelow (row, logList.size()))
            ViewHelpers::drawBasicTextRow (logList[row], g, width, height, false);
    }

    void setMaxMessages (int newMax)
    {
        if (newMax <= 0 || newMax == maxMessages)
            return;
        triggerAsyncUpdate();
    }

    void addMessage (const String& message)
    {
        if (logList.size() > maxMessages)
            logList.remove (0);
        logList.add (message);
        triggerAsyncUpdate();
    }

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

        addMessage (getIndentationString (level)
                        + "- osc bundle, time tag = "
                        + timeTag.toTime().toString (true, true, true, true));

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

        addMessage (getIndentationString (level + 1) + "- " + typeAsString.paddedRight(' ', 12) + valueAsString);
    }

    void clear()
    {
        logList.clear();
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        updateContent();        
        scrollToEnsureRowIsOnscreen (logList.size() - 1);
        repaint();
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
    int maxMessages { 100 };
    StringArray logList;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCReceiverLogListBox)
};



class OSCReceiverNodeEditor : public NodeEditorComponent,
                              private OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>
{
public:
    OSCReceiverNodeEditor (const Node&);
    virtual ~OSCReceiverNodeEditor();

    void paint (Graphics&) override {};
    void resized() override;

    Node node;

private:
    OSCReceiverLogListBox oscReceiverLog;
    OSCReceiver oscReceiver;

    Label portNumberLabel    { {}, "OSC Receiver Port: " };
    Label portNumberField    { {}, "9001" };
    TextButton connectButton { "Connect" };
    TextButton clearButton   { "Clear" };
    Label connectionStatusLabel;

    int currentPortNumber = -1;

    //==============================================================================
    void connectButtonClicked()
    {
        if (! isConnected())
            connect();
        else
            disconnect();

        updateConnectionStatusLabel();
    }

    //==============================================================================
    void clearButtonClicked()
    {
        oscReceiverLog.clear();
    }

    //==============================================================================
    void oscMessageReceived (const OSCMessage& message) override
    {
        oscReceiverLog.addOSCMessage (message);
    }

    void oscBundleReceived (const OSCBundle& bundle) override
    {
        oscReceiverLog.addOSCBundle (bundle);
    }

    //==============================================================================
    void connect()
    {
        auto portToConnect = portNumberField.getText().getIntValue();

        if (! isValidOscPort (portToConnect))
        {
            handleInvalidPortNumberEntered();
            return;
        }

        if (oscReceiver.connect (portToConnect))
        {
            currentPortNumber = portToConnect;
            connectButton.setButtonText ("Disconnect");
        }
        else
        {
            handleConnectError (portToConnect);
        }
    }

    //==============================================================================
    void disconnect()
    {
        if (oscReceiver.disconnect())
        {
            currentPortNumber = -1;
            connectButton.setButtonText ("Connect");
        }
        else
        {
            handleDisconnectError();
        }
    }

    //==============================================================================
    void handleConnectError (int failedPort)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          "OSC Connection error",
                                          "Error: could not connect to port " + String (failedPort),
                                          "OK");
    }

    //==============================================================================
    void handleDisconnectError()
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          "Unknown error",
                                          "An unknown error occurred while trying to disconnect from UDP port.",
                                          "OK");
    }

    //==============================================================================
    void handleInvalidPortNumberEntered()
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          "Invalid port number",
                                          "Error: you have entered an invalid UDP port number.",
                                          "OK");
    }

    //==============================================================================
    bool isConnected() const
    {
        return currentPortNumber != -1;
    }

    //==============================================================================
    bool isValidOscPort (int port) const
    {
        return port > 0 && port < 65536;
    }

    //==============================================================================
    void updateConnectionStatusLabel()
    {
        String text; // = ""; // "Status: ";

        if (isConnected())
            text = "Connected"; //+= "Connected to UDP port " + String (currentPortNumber);
        else
            text = "Disconnected"; //+= "Disconnected";

        auto textColour = isConnected() ? Colours::green : Colours::red;

        connectionStatusLabel.setText (text, dontSendNotification);
        connectionStatusLabel.setFont (Font (15.00f, Font::bold));
        connectionStatusLabel.setColour (Label::textColourId, textColour);
        connectionStatusLabel.setJustificationType (Justification::centredRight);
    }

};
