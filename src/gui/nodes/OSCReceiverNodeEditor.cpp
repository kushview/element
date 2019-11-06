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

#include "gui/nodes/OSCReceiverNodeEditor.h"

namespace Element {

typedef ReferenceCountedObjectPtr<OSCReceiverNode> OSCReceiverNodePtr;

OSCReceiverNodeEditor::OSCReceiverNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{
    portNumberLabel.setBounds (10, 18, 130, 25);
    addAndMakeVisible (portNumberLabel);

    portNumberField.setEditable (true, true, true);
    portNumberField.setBounds (140, 18, 50, 25);
    addAndMakeVisible (portNumberField);

    connectButton.setBounds (200, 18, 100, 25);
    addAndMakeVisible (connectButton);

    clearButton.setBounds (310, 18, 60, 25);
    addAndMakeVisible (clearButton);

    connectionStatusLabel.setBounds (390, 18, 100, 25);
    updateConnectionStatusLabel();
    addAndMakeVisible (connectionStatusLabel);

    oscReceiverLog.setBounds (0, 60, 500, 100);
    addAndMakeVisible (oscReceiverLog);

    bindHandlers();

    setSize (640, 320);
}

OSCReceiverNodeEditor::~OSCReceiverNodeEditor()
{
    unbindHandlers();
}

void OSCReceiverNodeEditor::bindHandlers()
{
    connectButton.onClick = std::bind (&OSCReceiverNodeEditor::connectButtonClicked, this);
    clearButton.onClick = std::bind (&OSCReceiverNodeEditor::clearButtonClicked, this);

    oscReceiver.addListener (this);
}

void OSCReceiverNodeEditor::unbindHandlers()
{
    connectButton.onClick = nullptr;
    clearButton.onClick = nullptr;

    oscReceiver.removeListener (this);
}

void OSCReceiverNodeEditor::resized ()
{
    //oscReceiverLog.setBounds (getLocalBounds().reduced (4));
}

void OSCReceiverNodeEditor::connectButtonClicked()
{
    if (! isConnected())
        connect();
    else
        disconnect();

    updateConnectionStatusLabel();
}

void OSCReceiverNodeEditor::clearButtonClicked()
{
    oscReceiverLog.clear();
}

void OSCReceiverNodeEditor::oscMessageReceived (const OSCMessage& message)
{
    oscReceiverLog.addOSCMessage (message);
}

void OSCReceiverNodeEditor::oscBundleReceived (const OSCBundle& bundle)
{
    oscReceiverLog.addOSCBundle (bundle);
}

void OSCReceiverNodeEditor::connect()
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

void OSCReceiverNodeEditor::disconnect()
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

void OSCReceiverNodeEditor::handleConnectError (int failedPort)
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                        "OSC Connection error",
                                        "Error: could not connect to port " + String (failedPort),
                                        "OK");
}

void OSCReceiverNodeEditor::handleDisconnectError()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                        "Unknown error",
                                        "An unknown error occurred while trying to disconnect from UDP port.",
                                        "OK");
}

void OSCReceiverNodeEditor::handleInvalidPortNumberEntered()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                        "Invalid port number",
                                        "Error: you have entered an invalid UDP port number.",
                                        "OK");
}

bool OSCReceiverNodeEditor::isConnected() const
{
    return currentPortNumber != -1;
}

bool OSCReceiverNodeEditor::isValidOscPort (int port) const
{
    return port > 0 && port < 65536;
}

void OSCReceiverNodeEditor::updateConnectionStatusLabel()
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
