// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/settings.hpp>
#include <element/tags.hpp>

#include "engine/midiengine.hpp"

using namespace element;

namespace {

// Identifiers that never match a real device, so nothing is opened.
const juce::String ghostInputID { "el-test-ghost-in" };
const juce::String ghostInputName { "Ghost In" };
const juce::String ghostOutputID { "el-test-ghost-out" };
const juce::String ghostOutputName { "Ghost Out" };

static juce::ValueTree makeInput (const juce::String& name, const juce::String& identifier, bool active)
{
    juce::ValueTree input (tags::input);
    input.setProperty (tags::name, name, nullptr)
        .setProperty (tags::identifier, identifier, nullptr)
        .setProperty (tags::active, active, nullptr);
    return input;
}

static void store (juce::PropertySet& props, const juce::ValueTree& data)
{
    auto xml = data.createXml();
    BOOST_REQUIRE (xml != nullptr);
    props.setValue (Settings::midiEngineKey, xml.get());
}

static juce::ValueTree load (juce::PropertySet& props)
{
    auto xml = props.getXmlValue (Settings::midiEngineKey);
    BOOST_REQUIRE (xml != nullptr);
    return juce::ValueTree::fromXml (*xml);
}

static juce::ValueTree findInput (const juce::ValueTree& data, const juce::String& identifier)
{
    for (const auto& child : data)
        if (child.hasType (tags::input) && child[tags::identifier].toString() == identifier)
            return child;
    return {};
}

} // namespace

BOOST_AUTO_TEST_SUITE (MidiEngineTests)

BOOST_AUTO_TEST_CASE (RemembersEnabledInputThatCannotBeOpened)
{
    juce::PropertySet props;
    juce::ValueTree data ("MidiSettings");
    data.appendChild (makeInput (ghostInputName, ghostInputID, true), nullptr);
    store (props, data);

    MidiEngine engine;
    engine.applySettings (props);
    engine.writeSettings (props);

    const auto input = findInput (load (props), ghostInputID);
    BOOST_REQUIRE (input.isValid());
    BOOST_CHECK ((bool) input[tags::active]);
    BOOST_CHECK_EQUAL (input[tags::name].toString().toStdString(), ghostInputName.toStdString());
}

BOOST_AUTO_TEST_CASE (InactiveInputIsNotResurrected)
{
    juce::PropertySet props;
    juce::ValueTree data ("MidiSettings");
    data.appendChild (makeInput (ghostInputName, ghostInputID, false), nullptr);
    store (props, data);

    MidiEngine engine;
    engine.applySettings (props);
    engine.writeSettings (props);

    const auto input = findInput (load (props), ghostInputID);
    BOOST_CHECK (! input.isValid() || ! (bool) input[tags::active]);
}

BOOST_AUTO_TEST_CASE (DefaultOutputRoundTrip)
{
    juce::PropertySet props;
    juce::ValueTree data ("MidiSettings");
    data.setProperty ("defaultMidiOutput", ghostOutputName, nullptr);
    data.setProperty ("defaultMidiOutputID", ghostOutputID, nullptr);
    store (props, data);

    MidiEngine engine;
    engine.applySettings (props);
    BOOST_CHECK_EQUAL (engine.getDefaultMidiOutputID().toStdString(), ghostOutputID.toStdString());
    BOOST_CHECK_EQUAL (engine.getDefaultMidiOutputName().toStdString(), ghostOutputName.toStdString());

    engine.writeSettings (props);
    const auto written = load (props);
    BOOST_CHECK_EQUAL (written["defaultMidiOutputID"].toString().toStdString(), ghostOutputID.toStdString());
    BOOST_CHECK_EQUAL (written["defaultMidiOutput"].toString().toStdString(), ghostOutputName.toStdString());
}

BOOST_AUTO_TEST_CASE (SetMidiInputEnabledRemembersUnavailableDevice)
{
    MidiEngine engine;
    juce::MidiDeviceInfo device;
    device.name = ghostInputName;
    device.identifier = ghostInputID;

    engine.setMidiInputEnabled (device, true);

    juce::PropertySet props;
    engine.writeSettings (props);
    auto input = findInput (load (props), ghostInputID);
    BOOST_REQUIRE (input.isValid());
    BOOST_CHECK ((bool) input[tags::active]);
    BOOST_CHECK_EQUAL (input[tags::name].toString().toStdString(), ghostInputName.toStdString());

    engine.setMidiInputEnabled (device, false);
    engine.writeSettings (props);
    input = findInput (load (props), ghostInputID);
    BOOST_CHECK (! input.isValid());
}

BOOST_AUTO_TEST_CASE (WriteIsIdempotent)
{
    juce::PropertySet props;
    juce::ValueTree data ("MidiSettings");
    data.appendChild (makeInput (ghostInputName, ghostInputID, true), nullptr);
    data.setProperty ("defaultMidiOutput", ghostOutputName, nullptr);
    data.setProperty ("defaultMidiOutputID", ghostOutputID, nullptr);
    store (props, data);

    MidiEngine engine;
    engine.applySettings (props);
    engine.writeSettings (props);
    const auto first = props.getValue (Settings::midiEngineKey);
    engine.writeSettings (props);
    const auto second = props.getValue (Settings::midiEngineKey);
    BOOST_CHECK_EQUAL (first.toStdString(), second.toStdString());
}

BOOST_AUTO_TEST_SUITE_END()
