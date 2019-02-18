
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/nodes/GenericNodeEditor.h"
#include "gui/LookAndFeel.h"
#include "gui/ContextMenus.h"
#include "gui/NodeEditorContentView.h"
#include "gui/ViewHelpers.h"

#include "session/DeviceManager.h"
#include "Globals.h"

namespace Element {

    static String noteValueToString (double value)
    {
        return MidiMessage::getMidiNoteName (roundToInt (value), true, true, 3);
    }

    NodeEditorContentView::NodeEditorContentView()
    {
        // const Font font (12.f);
        // setWantsKeyboardFocus (false);
        // setMouseClickGrabsKeyboardFocus (false);
        // setInterceptsMouseClicks (true, true);

        addAndMakeVisible (nodesCombo);
        nodesCombo.addListener (this);

        addAndMakeVisible (menuButton);
        menuButton.setIcon (Icon (getIcons().falBarsOutline, 
            findColour (TextButton::textColourOffId)));
        menuButton.setTriggeredOnMouseDown (true);
        menuButton.onClick = [this]()
        {
            NodePopupMenu menu (node, [this](NodePopupMenu& nodeMenu) {
                nodeMenu.addItem (1, "Sticky", true, isSticky());
            });
            
            menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&menuButton),
                ModalCallbackFunction::forComponent (nodeMenuCallback, this));
        };
    }

    NodeEditorContentView::~NodeEditorContentView()
    {
        menuButton.onClick = nullptr;
        nodesCombo.removeListener (this);
        selectedNodeConnection.disconnect();
    }

    void NodeEditorContentView::nodeMenuCallback (int result, NodeEditorContentView* view)
    {
        if (result == 1)
        {
            view->setSticky (! view->isSticky());
            DBG("[EL] node editor panel is now sticky: " << (int) view->isSticky());
        }
    }

    void NodeEditorContentView::paint (Graphics& g)
    {
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void NodeEditorContentView::comboBoxChanged (ComboBox*)
    {
        const auto selectedNode = graph.getNode (nodesCombo.getSelectedItemIndex());
        if (selectedNode.isValid())
        {
            if (sticky)
                setNode (selectedNode);
            ViewHelpers::findContentComponent(this)->getAppController()
                .findChild<GuiController>()->selectNode (selectedNode);
        }
    }

    void NodeEditorContentView::resized()
    {
        auto r1 = getLocalBounds().reduced (2);
        r1.removeFromTop (4);
        auto r2 = r1.removeFromTop (20);
        
        nodesCombo.setBounds (r2.removeFromLeft (jmax (100, r2.getWidth() - 24)));
        menuButton.setBounds (r2.withWidth(22).withX (r2.getX() + 2));
        
        if (editor)
        {
            r1.removeFromTop (2);
            editor->setBounds (r1);
        }
    }

    void NodeEditorContentView::setSticky (bool shouldBeSticky)
    {
        if (sticky == shouldBeSticky)
            return;
        sticky = shouldBeSticky;
        resized();
    }

    void NodeEditorContentView::stabilizeContent()
    {
        auto *cc = ViewHelpers::findContentComponent (this);
        auto session = ViewHelpers::getSession (this);
        jassert (cc && session);
        auto& gui = *cc->getAppController().findChild<GuiController>();
        if (! selectedNodeConnection.connected())
            selectedNodeConnection = gui.nodeSelected.connect (std::bind (
                &NodeEditorContentView::stabilizeContent, this));

        if (! sticky || ! node.isValid())
        {
            setNode (gui.getSelectedNode());
        }
        
        if (! node.isValid())
        {
            setNode (session->getActiveGraph().getNode (0));
        }
    }

    void NodeEditorContentView::setNode (const Node& newNode)
    {
        auto newGraph = newNode.getParentGraph();
        
        if (newGraph != graph)
        {
            graph = newGraph;
            nodesCombo.addNodes (graph);
        }

        if (newNode != node)
        {
            clearEditor();
            node = newNode;

            editor.reset (createEmbededEditor());
            if (editor)
                addAndMakeVisible (editor.get());
            nodesCombo.selectNode (node);

            resized();
        }
    }
    void NodeEditorContentView::clearEditor()
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

    Component* NodeEditorContentView::createEmbededEditor()
    {
        auto* const world = ViewHelpers::getGlobals (this);
        
        if (node.isAudioInputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new AudioDeviceSelectorComponent (world->getDeviceManager(), 
                    1, DeviceManager::maxAudioChannels, 0, 0, 
                    false, false, false, false);
            }
            else
            {
                return nullptr;
            }
        }

        if (node.isAudioOutputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new AudioDeviceSelectorComponent (world->getDeviceManager(), 
                    0, 0, 1, DeviceManager::maxAudioChannels, 
                    false, false, false, false);
            }
            else
            {
                return nullptr;
            }
        }

        if (node.isMidiInputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new AudioDeviceSelectorComponent (world->getDeviceManager(), 
                    0, 0, 0, 0, true, false, false, false);
            }
            else
            {
                return nullptr;
            }
        }

        if (node.isMidiOutputNode())
        {
            if (node.isChildOfRootGraph())
            {
                return new AudioDeviceSelectorComponent (world->getDeviceManager(), 
                    0, 0, 0, 0, false, true, false, false);
            }
            else
            {
                return nullptr;
            }
        }

        GraphNodePtr object = node.getGraphNode();
        auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        if (proc != nullptr)
        {
            return new GenericNodeEditor (node);
        }
        else if (node.getIdentifier() == EL_INTERNAL_ID_PROGRAM_CHANGE_MAP)
        {
            auto* const programChangeMapEditor = new ProgramChangeMapEditor (node);
            programChangeMapEditor->setStoreSize (false);
            return programChangeMapEditor;
        }

        return nullptr;
    }
}
