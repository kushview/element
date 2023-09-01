// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include "ui/viewhelpers.hpp"
#include "ui/block.hpp"
#include "scopedcallback.hpp"

namespace element {

class BlockComponent;
class BlockFactory;
class ConnectorComponent;
class PortComponent;
class PluginWindow;

/** A panel that displays and edits a Graph. */
class GraphEditorComponent : public Component,
                             public ChangeListener,
                             public DragAndDropTarget,
                             public FileDragAndDropTarget,
                             private ValueTree::Listener,
                             public ViewHelperMixin,
                             public LassoSource<uint32>
{
public:
    GraphEditorComponent();
    virtual ~GraphEditorComponent();

    /** Set the currently displayed graph */
    void setNode (const Node& n);

    /** Returns the displayed graph */
    Node getGraph() const { return graph; }

    //=========================================================================
    void findLassoItemsInArea (Array<uint32>& itemsFound, const Rectangle<int>& area) override;
    SelectedItemSet<uint32>& getLassoSelection() override { return selectedNodes; }

    /** Selects the given node */
    void selectNode (const Node& n);
    void selectAllNodes();

    /** Removes the selected nodes */
    void deleteSelectedNodes();

    //=========================================================================
    void setZoomScale (float scale);
    float getZoomScale() const noexcept { return zoomScale; }

    //=========================================================================
    /** Returns true if the layout is vertical */
    bool isLayoutVertical() const { return verticalLayout; }

    /** Changes the layout to vertical or not */
    void setVerticalLayout (const bool isVertical);

    //=========================================================================
    Rectangle<int> getRequiredSpace() const;

    //=========================================================================
    /** Stabilize all nodes without changing position */
    void stabilizeNodes();

    void updateComponents (const bool doNodePositions = true);

    //=========================================================================
    void changeListenerCallback (ChangeBroadcaster*) override;

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDown (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDropped (const SourceDetails& details) override;
    bool shouldDrawDragImageWhenOver() override { return true; }

    bool isInterestedInFileDrag (const StringArray& files) override;
    void filesDropped (const StringArray& files, int x, int y) override;

    //=========================================================================
    std::function<bool (const StringArray&, int, int)> onFilesDropped;
    std::function<void (BlockComponent&)> onBlockMoved;
    std::function<void()> onZoomChanged;

protected:
    virtual Component* wrapAudioProcessorEditor (AudioProcessorEditor* ed, ProcessorPtr editorNode);
    void createNewPlugin (const PluginDescription* desc, int x, int y);

private:
    friend class ConnectorComponent;
    friend class BlockComponent;
    friend class PortComponent;

    Node graph;
    ValueTree data;

    float lastDropX = 0.5f;
    float lastDropY = 0.5f;

    std::unique_ptr<ConnectorComponent> draggingConnector;
    std::unique_ptr<BlockFactory> factory;

    bool verticalLayout = true;

    LassoComponent<uint32> lasso;
    friend class SelectedNodes;
    class SelectedNodes : public SelectedItemSet<uint32>
    {
    public:
        SelectedNodes (GraphEditorComponent& owner)
            : editor (owner) {}
        void itemSelected (uint32 nodeId) override;
        void itemDeselected (uint32 nodeId) override;
        GraphEditorComponent& editor;
    } selectedNodes;

    bool ignoreNodeSelected = false;

    float zoomScale = 1.0;

    void setSelectedNodesCompact (bool selected);

    Component* createContainerForNode (ProcessorPtr node, bool useGenericEditor);
    AudioProcessorEditor* createEditorForNode (ProcessorPtr node, bool useGenericEditor);

    void updateBlockComponents (const bool doPosition = true);
    void updateConnectorComponents (bool async = false);

    void beginConnectorDrag (const uint32 sourceFilterID, const int sourceFilterChannel, const uint32 destFilterID, const int destFilterChannel, const MouseEvent& e);
    void dragConnector (const MouseEvent& e);
    void endDraggingConnector (const MouseEvent& e);

    BlockComponent* createBlock (const Node&);

    BlockComponent* getComponentForFilter (const uint32 filterID) const;
    ConnectorComponent* getComponentForConnection (const Arc& conn) const;
    PortComponent* findPinAt (const int x, const int y) const;

    void updateSelection();

    void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override {}
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved,
                                     int oldIndex,
                                     int newIndex) override {}
    void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override {}
    void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override {}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphEditorComponent)
};

class GraphEditor : public juce::Component
{
public:
    GraphEditor()
    {
        addAndMakeVisible (_viewport);

        setSize (640, 360);
        _viewport.setViewedComponent (&_editor, false);
        _viewport.setScrollBarsShown (true, true, false, false);
        _viewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
        _viewport.setBounds (_editor.getLocalBounds());

        _editor.onFilesDropped = [this] (const StringArray& sa, int a, int b) -> bool {
            if (onFilesDropped)
                return onFilesDropped (sa, a, b);
            return false;
        };

        _editor.onBlockMoved = [this] (BlockComponent& block) {
            const int resizeBy = 12;
            const int edgeSpeed = 6;
            const int maxSpeed = 10;

            // restrict top/left out of bounds scroll
            auto pos = block.getBounds().getTopLeft();
            if (block.getX() < 0)
                pos.setX (0);
            if (block.getY() < 0)
                pos.setY (0);

            // save top left
            const auto revertTopLeftPos = pos;
            const bool revertTopLeft = pos.getX() != block.getX() || pos.getY() != block.getY();
            ScopedCallback defer ([&block, &revertTopLeftPos, &revertTopLeft]() {
                if (revertTopLeft)
                {
                    block.setNodePosition (revertTopLeftPos);
                    block.updatePosition();
                }
            });

            // no action if mouse within viewable area
            const auto mp = _viewport.getLocalPoint (nullptr, Desktop::getInstance().getMousePosition());
            if (mp.getX() > 0 && mp.getX() < _viewport.getViewWidth() && mp.getY() > 0 && mp.getY() < _viewport.getViewHeight())
            {
                return;
            }

            // expand and scroll bottom/right
            pos = block.getBounds().getBottomRight();
            auto gb = _editor.getBounds();
            bool sizeShouldChange = false;
            if (pos.x > gb.getWidth())
            {
                gb.setWidth (pos.x + resizeBy);
                sizeShouldChange = true;
            }
            if (pos.y > gb.getHeight())
            {
                gb.setHeight (pos.y + resizeBy);
                sizeShouldChange = true;
            }
            if (sizeShouldChange)
            {
                _editor.setBounds (gb);
            }

            pos = _viewport.getLocalPoint (&_editor, pos.toFloat()).toInt();
            _viewport.autoScroll (pos.x, pos.y, edgeSpeed, maxSpeed);
        };

        // _editor.onZoomChanged = [this]() {
        //     auto s = settings();
        //     if (s.isValid())
        //     {
        //         s.setProperty ("zoomScale", _editor.getZoomScale(), nullptr);
        //     }
        // };
    }

    ~GraphEditor()
    {
        _editor.onFilesDropped = nullptr;
        _viewport.setViewedComponent (nullptr);
    }

    void setNode (const Node& node) { _editor.setNode (node); }
    void deleteSelectedNodes() { _editor.deleteSelectedNodes(); }

    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void resized() override
    {
        auto r = getLocalBounds();
        _viewport.setBounds (r);
        if (_editor.getWidth() < _viewport.getWidth() || _editor.getHeight() < _viewport.getHeight())
        {
            _editor.setBounds (_viewport.getBounds());
        }
    }

    juce::Viewport& viewport() { return _viewport; }

    // FIXME: api: need some TLC to make api friendly in the future.
    void stabilizeNodes() { _editor.stabilizeNodes(); }
    void updateComponents (const bool doNodePositions = true) { _editor.updateComponents (doNodePositions); }
    void selectNode (const Node& node) { _editor.selectNode (node); }
    void selectAllNodes() { _editor.selectAllNodes(); }
    std::function<bool (const StringArray&, int, int)> onFilesDropped;
    // END FIXME

private:
    GraphEditorComponent _editor;
    juce::Viewport _viewport;
};

} // namespace element
