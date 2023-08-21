// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/standard.hpp>
#include <element/ui/style.hpp>
#include <element/ui/commands.hpp>

#include "ui/guicommon.hpp"
#include "ui/pluginwindow.hpp"
#include "ui/grapheditorview.hpp"

#include "plugineditor.hpp"
#include "pluginprocessor.hpp"
#include "ElementApp.h"

#define EL_PLUGIN_MIN_WIDTH 546
#define EL_PLUGIN_MIN_HEIGHT 266
#define EL_PLUGIN_DEFAULT_WIDTH 760
#define EL_PLUGIN_DEFAULT_HEIGHT 480

namespace element {
using element::LookAndFeel;

//==============================================================================
class PerformanceParameterSlider : public Slider,
                                   private AudioProcessorParameter::Listener,
                                   private Timer
{
public:
    PerformanceParameterSlider (PerformanceParameter& p)
        : param (p)
    {
        param.addListener (this);
        setSliderStyle (Slider::RotaryHorizontalDrag);
        setTextBoxStyle (Slider::NoTextBox, false, 10, 10);
        setScrollWheelEnabled (false);

        if (param.getNumSteps() != AudioProcessor::getDefaultNumParameterSteps())
            setRange (0.0, 1.0, 1.0 / (param.getNumSteps() - 1.0));
        else
            setRange (0.0, 1.0);

        handleNewParameterValue();

        onValueChange = [this]() { sliderValueChanged(); };
        onDragStart = [this]() { sliderStartedDragging(); };
        onDragEnd = [this]() { sliderStoppedDragging(); };

        updateEnablement();
        updateToolTip();

        param.onCleared = [this]() {
            updateEnablement();
            updateToolTip();
        };

        startTimerHz (100);
    }

    ~PerformanceParameterSlider()
    {
        param.onCleared = nullptr;
        param.removeListener (this);
    }

    void updateToolTip()
    {
        if (! param.haveNode())
        {
            setTooltip (param.getName (100));
            return;
        }

        String message = param.getNode().getName();
        String paramName = param.getBoundParameterName();
        if (paramName.isEmpty())
            paramName = param.getName (100);

        if (message.isNotEmpty() && paramName.isNotEmpty())
            message << " - " << paramName;
        else if (paramName.isNotEmpty())
            message = paramName;

        setTooltip (message);
    }

    void updateEnablement()
    {
        setEnabled (param.haveNode());
    }

    void mouseDown (const MouseEvent& ev) override;

private:
    PerformanceParameter& param;
    Atomic<int> parameterValueHasChanged { 0 };
    bool isDragging = false;

    //==============================================================================
    void parameterValueChanged (int, float) override
    {
        parameterValueHasChanged = 1;
    }

    void parameterGestureChanged (int, bool) override {}

    //==============================================================================
    void timerCallback() override
    {
        if (parameterValueHasChanged.compareAndSetBool (0, 1))
        {
            handleNewParameterValue();
            startTimerHz (50);
        }
        else
        {
            startTimer (jmin (250, getTimerInterval() + 10));
        }
    }

    void updateTextDisplay()
    {
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
        // valueLabel.setText (getParameter().getCurrentValueAsText(), dontSendNotification);
    }

    void handleNewParameterValue()
    {
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
        if (! isDragging)
        {
            setValue (param.getValue(), dontSendNotification);
            updateTextDisplay();
        }
    }

    void sliderValueChanged()
    {
        auto newVal = (float) getValue();

        if (param.getValue() != newVal)
        {
            if (! isDragging)
                param.beginChangeGesture();

            param.setValueNotifyingHost (newVal);
            updateTextDisplay();

            if (! isDragging)
                param.endChangeGesture();
        }
    }

    void sliderStartedDragging()
    {
        isDragging = true;
        param.beginChangeGesture();
    }

    void sliderStoppedDragging()
    {
        isDragging = false;
        param.endChangeGesture();
    }
};

//==============================================================================
class PerfSliders : public Component
{
public:
    PerfSliders (PluginProcessor& processor)
    {
        for (auto* param : processor.getParameters())
        {
            if (auto* perf = dynamic_cast<PerformanceParameter*> (param))
            {
                auto* slider = sliders.add (new PerformanceParameterSlider (*perf));
                addAndMakeVisible (slider);
            }
        }

        setSize (500, 44);
    }

    void update()
    {
        for (auto* const slider : sliders)
        {
            slider->updateEnablement();
            slider->updateToolTip();
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (element::Colors::widgetBackgroundColor.darker (0.29f));
    }

    void resized() override
    {
        int space = 12;
        Rectangle<int> sb;
        sb = getLocalBounds();

#if 0
        // right to left
        sb.removeFromRight (space);
        for (int i = sliders.size(); --i >= 0;)
        {
            auto* const slider = sliders [i];
            slider->setBounds (sb.removeFromRight (44).reduced(4));
            sb.removeFromRight (space);
        }
#else
        // left to right
        sb.removeFromLeft (space);
        for (int i = 0; i < sliders.size(); ++i)
        {
            auto* const slider = sliders[i];
            slider->setBounds (sb.removeFromLeft (44).reduced (4));
            sb.removeFromLeft (space);
        }
#endif
    }

private:
    OwnedArray<PerformanceParameterSlider> sliders;
};

//==============================================================================
class PluginEditor::ParamTable : public TableListBox,
                                 public TableListBoxModel
{
public:
    ParamTable()
    {
        setModel (this);
    }

    ~ParamTable()
    {
        setModel (nullptr);
    }

    int getNumRows() override { return 8; }

    void paintRowBackground (Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override {}

    void paintCell (Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        g.setColour (Colours::black);
        g.fillRect (0, 0, width, height);
    }

#if 0
    virtual Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                Component* existingComponentToUpdate);
    virtual void cellClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&);
    virtual void backgroundClicked (const MouseEvent&);
    virtual void sortOrderChanged (int newSortColumnId, bool isForwards);
    virtual int getColumnAutoSizeWidth (int columnId);
    virtual String getCellTooltip (int rowNumber, int columnId);
    virtual void selectedRowsChanged (int lastRowSelected);
    virtual void deleteKeyPressed (int lastRowSelected);
    virtual void returnKeyPressed (int lastRowSelected);
    virtual void listWasScrolled();
    virtual var getDragSourceDescription (const SparseSet<int>& currentlySelectedRows);
#endif
};

//==============================================================================
void PerformanceParameterSlider::mouseDown (const MouseEvent& ev)
{
    auto* const editor = findParentComponentOfClass<PluginEditor>();
    if (editor != nullptr && ev.mods.isPopupMenu())
    {
        auto& processor = editor->getProcessor();
        PopupMenu menu = processor.getPerformanceParameterMenu (param.getParameterIndex());
        const int paramIndex = param.getParameterIndex();
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (this),
                            std::bind (&PluginProcessor::handlePerformanceParameterResult, &processor, std::placeholders::_1, paramIndex));
        return;
    }

    return Slider::mouseDown (ev);
}

//==============================================================================
class PluginEditor::ParamTableToggle : public Button
{
public:
    ParamTableToggle() : Button ("ParamToggle") {}
    ~ParamTableToggle() {}

protected:
    void paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        ignoreUnused (shouldDrawButtonAsDown);
        getLookAndFeel().drawTreeviewPlusMinusBox (g, getLocalBounds().toFloat().reduced (2), Colors::widgetBackgroundColor.darker(), getToggleState(), shouldDrawButtonAsHighlighted);
    }
};

//==============================================================================
PluginEditor::PluginEditor (PluginProcessor& plugin)
    : AudioProcessorEditor (&plugin), processor (plugin)
{
    Logger::writeToLog ("PluginEditor::PluginEditor");
    setOpaque (true);
    paramTable.reset (new ParamTable());
    addAndMakeVisible (paramTable.get());
    paramToggle.reset (new ParamTableToggle());
    // addAndMakeVisible (paramToggle.get());
    paramToggle->setClickingTogglesState (true);
    paramToggle->onClick = [this] {
        paramTableVisible = paramToggle->getToggleState();
        if (paramTableVisible)
        {
            paramTable->setVisible (true);
            setSize (getWidth(), getHeight() + paramTableSize);
        }
        else
        {
            paramTable->setVisible (false);
            setSize (getWidth(), getHeight() - paramTableSize);
        }
    };

    const auto bounds (processor.getEditorBounds());
    if (bounds.getWidth() > 0 && bounds.getHeight() > 0)
    {
        setSize (jmax (EL_PLUGIN_MIN_WIDTH, bounds.getWidth()),
                 jmax (EL_PLUGIN_MIN_HEIGHT, bounds.getHeight()));
    }
    else
    {
        setSize (EL_PLUGIN_DEFAULT_WIDTH, EL_PLUGIN_DEFAULT_HEIGHT);
    }

    triggerAsyncUpdate();

    perfParamChangedConnection = processor.onPerfParamsChanged.connect (std::bind (
        &PluginEditor::updatePerformanceParamEnablements, this));
}

PluginEditor::~PluginEditor()
{
    perfParamChangedConnection.disconnect();
    removeChildComponent (content.getComponent());
    content = nullptr;
    if (auto* const app = processor.getServices())
    {
        if (auto* gui = app->find<GuiService>())
        {
            gui->closeAllPluginWindows();
            gui->clearContentComponent();
        }
    }
}

//==============================================================================
element::Content* PluginEditor::getContentComponent()
{
    if (nullptr == content)
        if (auto* app = processor.getServices())
            if (auto* gui = app->find<GuiService>())
                content = gui->content();
    return nullptr != content ? dynamic_cast<element::Content*> (content.getComponent())
                              : nullptr;
}

//==============================================================================
void PluginEditor::setWantsPluginKeyboardFocus (bool focus)
{
    if (focus == getWantsKeyboardFocus())
        return;

    setWantsKeyboardFocus (focus);
    processor.setEditorWantsKeyboard (focus);
}

bool PluginEditor::getWantsPluginKeyboardFocus() const
{
    return getWantsKeyboardFocus();
}

//==============================================================================
int PluginEditor::getLatencySamples() const { return processor.getLatencySamples(); }
void PluginEditor::setReportZeroLatency (bool force) { processor.setForceZeroLatency (force); }
bool PluginEditor::isReportingZeroLatency() const { return processor.isForcingZeroLatency(); }

//==============================================================================
void PluginEditor::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor.darker (0.29f));
    if (! content)
    {
        g.setColour (Colors::textColor);
        g.drawFittedText (TRANS ("Content is empty....."),
                          0,
                          0,
                          getWidth(),
                          getHeight(),
                          Justification::centred,
                          2);
    }
}

void PluginEditor::resized()
{
    auto bounds (getLocalBounds());
    processor.setEditorBounds (bounds);
    if (content)
        content->setBounds (bounds);
}

bool PluginEditor::keyPressed (const KeyPress& key)
{
    auto* app = processor.getServices();
    auto& cmd (app->find<UI>()->commands());

    for (int i = 0; i < cmd.getNumCommands(); ++i)
    {
        const auto* info = cmd.getCommandForIndex (i);
        for (const auto& k : info->defaultKeypresses)
            if (k == key)
                return cmd.invokeDirectly (info->commandID, true);
    }

    return false;
}

//==============================================================================
void PluginEditor::updatePerformanceParamEnablements()
{
    if (auto* cc = dynamic_cast<StandardContent*> (content.getComponent()))
    {
        if (auto* ps = dynamic_cast<PerfSliders*> (cc->extraView()))
            ps->update();
        cc->stabilizeViews();
    }
}

void PluginEditor::handleAsyncUpdate()
{
    if (asyncInitDone)
        return;

    asyncInitDone = true;
    auto* const app = processor.getServices();
    auto* const gui = app->find<GuiService>();

    content = gui->content();
    jassert (content);

    if (auto* cc = dynamic_cast<StandardContent*> (content.getComponent()))
        cc->setExtraView (new PerfSliders (processor));

    setResizable (true, true);
    getConstrainer()->setMinimumSize (EL_PLUGIN_MIN_WIDTH, EL_PLUGIN_MIN_HEIGHT);

    const auto bounds (processor.getEditorBounds());
    if (bounds.getWidth() > 0 && bounds.getHeight() > 0)
    {
        setSize (jmax (EL_PLUGIN_MIN_WIDTH, bounds.getWidth()),
                 jmax (EL_PLUGIN_MIN_HEIGHT, bounds.getHeight()));
    }
    else
    {
        setSize (EL_PLUGIN_DEFAULT_WIDTH, EL_PLUGIN_DEFAULT_HEIGHT);
    }

    content->setBounds (bounds);
    addAndMakeVisible (content.getComponent());
    resized();

    setWantsPluginKeyboardFocus (processor.getEditorWantsKeyboard());

    gui->stabilizeContent();
    gui->stabilizeViews();

    if (auto session = app->context().session())
    {
        const auto graph (session->getActiveGraph());
        if (graph.isGraph())
        {
            gui->showPluginWindowsFor (graph);
            for (int w = 0; w < gui->getNumPluginWindows(); ++w)
                if (auto* window = gui->getPluginWindow (w))
                    window->toFront (false);
            // if (auto standard = dynamic_cast<StandardContent*> (content.getComponent()))
            //     standard->setCurrentNode (graph);
            if (auto* cc = dynamic_cast<StandardContent*> (content.getComponent()))
                cc->setMainView (new GraphEditorView (graph));
        }
    }
}

} // namespace element
