
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/LookAndFeel.h"
#include "gui/NodeUIContentView.h"
#include "gui/ViewHelpers.h"

namespace Element {

    static String noteValueToString (double value)
    {
        return MidiMessage::getMidiNoteName (roundToInt (value), true, true, 3);
    }

    NodeUIContentView::NodeUIContentView()
    {
        // const Font font (12.f);
        // setWantsKeyboardFocus (false);
        // setMouseClickGrabsKeyboardFocus (false);
        // setInterceptsMouseClicks (true, true);
    }

    NodeUIContentView::~NodeUIContentView()
    {
        selectedNodeConnection.disconnect();
    }

    void NodeUIContentView::paint (Graphics& g)
    {
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void NodeUIContentView::resized()
    {
        if (editor)
            editor->setBounds (getLocalBounds().reduced (2));
    }

    void NodeUIContentView::stabilizeContent()
    {
        auto *cc = ViewHelpers::findContentComponent(this);
        jassert(cc);
        auto& gui = *cc->getAppController().findChild<GuiController>();
        if (! selectedNodeConnection.connected())
            selectedNodeConnection = gui.nodeSelected.connect (std::bind (
                &NodeUIContentView::stabilizeContent, this));

        auto newNode = gui.getSelectedNode();

        if (newNode != node)
        {
            clearEditor();
            node = newNode;
            editor.reset (createEmbededEditor());
            if (editor)
                addAndMakeVisible (editor.get());
            resized();
        }
    }

    void NodeUIContentView::clearEditor()
    {
        if (editor == nullptr)
            return;
        GraphNodePtr object = node.getGraphNode();
        auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        if (auto* aped = dynamic_cast<AudioProcessorEditor*> (editor.get()))
        {
            if (proc)
                proc->editorBeingDeleted (aped);
        }

        removeChildComponent (editor.get());
        editor.reset (nullptr);
    }

    Component* NodeUIContentView::createEmbededEditor()
    {
        if (node.isIONode())
        {
            return nullptr;
        }

        GraphNodePtr object = node.getGraphNode();
        auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        if (proc != nullptr)
        {
            return new GenericAudioProcessorEditor (proc);
        }
        else if (node.getIdentifier() == EL_INTERNAL_ID_PROGRAM_CHANGE_MAP)
        {
            return new ProgramChangeMapEditor (node);
        }

        return nullptr;
    }
}
