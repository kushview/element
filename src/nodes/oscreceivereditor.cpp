// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later
// Author: Eliot Akira <me@eliotakira.com>

#include <element/ui/style.hpp>
#include "nodes/oscreceivereditor.hpp"

namespace element {

OSCReceiverNodeEditor::OSCReceiverNodeEditor (const Node& node)
    : NodeEditor (node)
{
    oscReceiverNodePtr = getNodeObjectOfType<OSCReceiverNode>();

    int width = 540;
    int height = 320;

    portNumberSlider.setRange (1.0, 65535.0, 1.0);
    portNumberSlider.setSliderStyle (Slider::IncDecButtons);
    portNumberSlider.setTextBoxStyle (Slider::TextBoxLeft, false, 60, 22);

    syncUIFromNodeState();

    resetBounds (width, height);
    addAndMakeVisible (hostNameLabel);
    addAndMakeVisible (hostNameField);
    addAndMakeVisible (portNumberLabel);
    addAndMakeVisible (portNumberSlider);
    addAndMakeVisible (connectButton);
    addAndMakeVisible (pauseButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (connectionStatusLabel);
    addAndMakeVisible (oscReceiverLog);

    setSize (width, height);

    /* Bind handlers */
    connectButton.onClick = std::bind (&OSCReceiverNodeEditor::connectButtonClicked, this);
    pauseButton.onClick = std::bind (&OSCReceiverNodeEditor::pauseButtonClicked, this);
    clearButton.onClick = std::bind (&OSCReceiverNodeEditor::clearButtonClicked, this);
    hostNameField.onTextChange = [this]() {
        String newHostName = hostNameField.getText();
        if (currentHostName == newHostName)
            return;
        if (connected)
            disconnect();
        currentHostName = newHostName;
        oscReceiverNodePtr->setHostName (currentHostName);
    };
    portNumberSlider.onValueChange = [this]() {
        int newPortNumber = roundToInt (portNumberSlider.getValue());
        if (newPortNumber == currentPortNumber)
            return;
        if (connected)
            disconnect();
        currentPortNumber = newPortNumber;
        oscReceiverNodePtr->setPortNumber (currentPortNumber);
    };

    oscReceiverNodePtr->addChangeListener (this);
    oscReceiverNodePtr->addMessageLoopListener (this);
}

OSCReceiverNodeEditor::~OSCReceiverNodeEditor()
{
    /* Unbind handlers */
    connectButton.onClick = nullptr;
    pauseButton.onClick = nullptr;
    clearButton.onClick = nullptr;
    hostNameField.onTextChange = nullptr;
    portNumberSlider.onValueChange = nullptr;
    oscReceiverNodePtr->removeChangeListener (this);
    oscReceiverNodePtr->removeMessageLoopListener (this);
}

void OSCReceiverNodeEditor::resized()
{
    resetBounds (getWidth(), getHeight());
}

void OSCReceiverNodeEditor::resetBounds (int fullWidth, int fullHeight)
{
    int margin = 5;

    int x = margin;
    int y = margin;
    int w;
    int h;

    h = 20;

    w = 40;
    hostNameLabel.setBounds (x, y, w, h);
    x += w + margin;

    w = 80;
    hostNameField.setBounds (x, y, w, h);
    x += w + margin;

    w = 40;
    portNumberLabel.setBounds (x, y, w, h);
    x += w;

    w = 100;
    portNumberSlider.setBounds (x, y, w, h);
    x += w + margin;

    // From right side

    w = 60;
    x = fullWidth - margin - w;
    clearButton.setBounds (x, y, w, h);

    w = 60;
    x -= margin + w;
    pauseButton.setBounds (x, y, w, h);

    w = 80;
    x -= margin + w;
    connectButton.setBounds (x, y, w, h);

    w = 30;
    x -= margin + w;
    connectionStatusLabel.setBounds (x, y, w, h);

    // Row
    x = 0;
    y += h + margin;
    oscReceiverLog.setBounds (x, y, fullWidth, fullHeight - y);
}

void OSCReceiverNodeEditor::paint (Graphics& g)
{
    g.fillAll (Colors::backgroundColor.brighter (0.1f));
}

void OSCReceiverNodeEditor::connectButtonClicked()
{
    if (! connected)
        connect();
    else
        disconnect();

    updateConnectionStatusLabel();
}

void OSCReceiverNodeEditor::pauseButtonClicked()
{
    paused = oscReceiverNodePtr->togglePause();
    updatePauseButton();
}

void OSCReceiverNodeEditor::clearButtonClicked()
{
    oscReceiverLog.clear();
}

void OSCReceiverNodeEditor::oscMessageReceived (const OSCMessage& message)
{
    if (paused)
        return;

    oscReceiverLog.addOSCMessage (message);
}

void OSCReceiverNodeEditor::oscBundleReceived (const OSCBundle& bundle)
{
    if (paused)
        return;

    oscReceiverLog.addOSCBundle (bundle);
}

void OSCReceiverNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    syncUIFromNodeState();
}

void OSCReceiverNodeEditor::syncUIFromNodeState()
{
    currentHostName = oscReceiverNodePtr->getCurrentHostName();
    currentPortNumber = oscReceiverNodePtr->getCurrentPortNumber();
    connected = oscReceiverNodePtr->isConnected();
    paused = oscReceiverNodePtr->isPaused();

    updateHostNameField();
    updatePortNumberSlider();
    updateConnectionStatusLabel();
    updateConnectButton();
    updatePauseButton();
}

void OSCReceiverNodeEditor::updateHostNameField()
{
    hostNameField.setText (currentHostName, NotificationType::dontSendNotification);
}

void OSCReceiverNodeEditor::updatePortNumberSlider()
{
    portNumberSlider.setValue ((double) currentPortNumber);
}

void OSCReceiverNodeEditor::connect()
{
    if (! Util::isValidOscPort (currentPortNumber))
    {
        handleInvalidPortNumberEntered();
        return;
    }

    if (oscReceiverNodePtr->connect (currentPortNumber))
    {
        connected = true;
        connectButton.setButtonText ("Disconnect");
        updateConnectionStatusLabel();
    }
    else
    {
        handleConnectError (currentPortNumber);
    }
}

void OSCReceiverNodeEditor::disconnect()
{
    if (oscReceiverNodePtr->disconnect())
    {
        connected = false;
        connectButton.setButtonText ("Connect");
        updateConnectionStatusLabel();
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
                                      "Could not connect to port " + String (failedPort) + ".",
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
                                      "You have entered an invalid UDP port number.",
                                      "OK");
}

void OSCReceiverNodeEditor::updateConnectButton()
{
    connectButton.setButtonText (connected ? "Disconnect" : "Connect");
}

void OSCReceiverNodeEditor::updateConnectionStatusLabel()
{
    String text = connected ? "On" : "Off";
    auto textColour = connected ? Colours::green.brighter (0.3f) : Colours::red.brighter (0.3f);

    connectionStatusLabel.setText (text, dontSendNotification);
    connectionStatusLabel.setColour (Label::textColourId, textColour);
}

void OSCReceiverNodeEditor::updatePauseButton()
{
    pauseButton.setButtonText (paused ? "Resume" : "Pause");
};

}; // namespace element
