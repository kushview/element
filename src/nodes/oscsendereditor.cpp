// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later
// Author: Eliot Akira <me@eliotakira.com>

#include <element/ui/style.hpp>
#include "nodes/oscsender.hpp"
#include "nodes/oscsendereditor.hpp"

namespace element {

OSCSenderNodeEditor::OSCSenderNodeEditor (const Node& node)
    : NodeEditor (node)
{
    oscSenderNodePtr = getNodeObjectOfType<OSCSenderNode>();

    int width = 540;
    int height = 320;

    resetBounds (width, height);

    hostNameField.setEditable (true, true, true);

    portNumberSlider.setRange (1.0, 65535.0, 1.0);
    portNumberSlider.setSliderStyle (Slider::IncDecButtons);
    portNumberSlider.setTextBoxStyle (Slider::TextBoxLeft, false, 60, 22);

    syncUIFromNodeState();

    addAndMakeVisible (hostNameLabel);
    addAndMakeVisible (hostNameField);
    addAndMakeVisible (portNumberLabel);
    addAndMakeVisible (portNumberSlider);
    addAndMakeVisible (connectButton);
    addAndMakeVisible (pauseButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (connectionStatusLabel);
    addAndMakeVisible (oscSenderLog);

    setSize (width, height);

    /* Bind handlers */
    connectButton.onClick = std::bind (&OSCSenderNodeEditor::connectButtonClicked, this);
    pauseButton.onClick = std::bind (&OSCSenderNodeEditor::pauseButtonClicked, this);
    clearButton.onClick = std::bind (&OSCSenderNodeEditor::clearButtonClicked, this);
    hostNameField.onTextChange = std::bind (&OSCSenderNodeEditor::hostNameFieldChanged, this);
    hostNameField.onTextChange = [this]() {
        String newHostName = hostNameField.getText();
        if (currentHostName == newHostName)
            return;
        if (connected)
            disconnect();
        currentHostName = newHostName;
        oscSenderNodePtr->setHostName (currentHostName);
    };
    portNumberSlider.onValueChange = [this]() {
        int newPortNumber = roundToInt (portNumberSlider.getValue());
        if (newPortNumber == currentPortNumber)
            return;
        if (connected)
            disconnect();
        currentPortNumber = newPortNumber;
        oscSenderNodePtr->setPortNumber (currentPortNumber);
    };

    oscSenderNodePtr->addChangeListener (this);
    startTimerHz (60);
}

OSCSenderNodeEditor::~OSCSenderNodeEditor()
{
    /* Unbind handlers */
    stopTimer();
    connectButton.onClick = nullptr;
    pauseButton.onClick = nullptr;
    clearButton.onClick = nullptr;
    hostNameField.onTextChange = nullptr;
    portNumberSlider.onValueChange = nullptr;
    oscSenderNodePtr->removeChangeListener (this);
}

void OSCSenderNodeEditor::timerCallback()
{
    const std::vector<OSCMessage> oscMessages = oscSenderNodePtr->getOscMessages();

    for (auto msg : oscMessages)
    {
        oscSenderLog.addOSCMessage (msg);
    }
};

void OSCSenderNodeEditor::resized()
{
    resetBounds (getWidth(), getHeight());
}

void OSCSenderNodeEditor::resetBounds (int fullWidth, int fullHeight)
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
    oscSenderLog.setBounds (x, y, fullWidth, fullHeight - y);
}

void OSCSenderNodeEditor::paint (Graphics& g)
{
    g.fillAll (Colors::backgroundColor.brighter (0.1f));
}

void OSCSenderNodeEditor::connectButtonClicked()
{
    if (! connected)
        connect();
    else
        disconnect();

    updateConnectionStatusLabel();
}

void OSCSenderNodeEditor::pauseButtonClicked()
{
    paused = oscSenderNodePtr->togglePause();
    updatePauseButton();
}

void OSCSenderNodeEditor::clearButtonClicked()
{
    oscSenderLog.clear();
}

void OSCSenderNodeEditor::hostNameFieldChanged()
{
    disconnect();
    currentHostName = hostNameField.getText();
}

void OSCSenderNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    syncUIFromNodeState();
}

void OSCSenderNodeEditor::syncUIFromNodeState()
{
    currentHostName = oscSenderNodePtr->getCurrentHostName();
    currentPortNumber = oscSenderNodePtr->getCurrentPortNumber();
    connected = oscSenderNodePtr->isConnected();
    paused = oscSenderNodePtr->isPaused();

    updateHostNameField();
    updatePortNumberSlider();
    updateConnectionStatusLabel();
    updateConnectButton();
    updatePauseButton();
}

void OSCSenderNodeEditor::updateHostNameField()
{
    hostNameField.setText (currentHostName, NotificationType::dontSendNotification);
}

void OSCSenderNodeEditor::updatePortNumberSlider()
{
    portNumberSlider.setValue ((double) currentPortNumber);
}

void OSCSenderNodeEditor::connect()
{
    if (! Util::isValidOscPort (currentPortNumber))
    {
        handleInvalidPortNumberEntered();
        return;
    }

    if (oscSenderNodePtr->connect (currentHostName, currentPortNumber))
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

void OSCSenderNodeEditor::disconnect()
{
    if (oscSenderNodePtr->disconnect())
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

void OSCSenderNodeEditor::handleConnectError (int failedPort)
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                      "OSC Connection error",
                                      "Could not connect to port " + String (failedPort) + ".",
                                      "OK");
}

void OSCSenderNodeEditor::handleDisconnectError()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                      "Unknown error",
                                      "An unknown error occurred while trying to disconnect from UDP port.",
                                      "OK");
}

void OSCSenderNodeEditor::handleInvalidPortNumberEntered()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                      "Invalid port number",
                                      "You have entered an invalid UDP port number.",
                                      "OK");
}

void OSCSenderNodeEditor::updateConnectButton()
{
    connectButton.setButtonText (connected ? "Disconnect" : "Connect");
}

void OSCSenderNodeEditor::updateConnectionStatusLabel()
{
    String text = connected ? "On" : "Off";
    auto textColour = connected ? Colours::green.brighter (0.3f) : Colours::red.brighter (0.3f);

    connectionStatusLabel.setText (text, dontSendNotification);
    connectionStatusLabel.setColour (Label::textColourId, textColour);
}

void OSCSenderNodeEditor::updatePauseButton()
{
    pauseButton.setButtonText (paused ? "Resume" : "Pause");
};

}; // namespace element
