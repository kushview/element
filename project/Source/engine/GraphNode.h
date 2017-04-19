#ifndef EL_GRAPH_NODE_H
#define EL_GRAPH_NODE_H

#include "ElementApp.h"

namespace Element {
    
class GraphProcessor;
    
/** Represents one of the nodes, or processors, in an AudioProcessorGraph.

    To create a node, call ProcessorGraph::addNode().
*/
class GraphNode : public ReferenceCountedObject
{
public:
    /** The ID number assigned to this node.
        This is assigned by the graph that owns it, and can't be changed. */
    const uint32 nodeId;

    /** A set of user-definable properties that are associated with this node.

        This can be used to attach values to the node for whatever purpose seems
        useful. For example, you might store an x and y position if your application
        is displaying the nodes on-screen.
    */
    NamedValueSet properties;

    /** Returns the actual processor object that this node represents. */
    Processor* getProcessor() const noexcept { return proc; }

    AudioPluginInstance* getAudioPluginInstance() const;

    int getNumAudioInputs() const;
    int getNumAudioOutputs() const;
    uint32 getMidiInputPort() const;
    uint32 getMidiOutputPort() const;
    
    void getPluginDescription (PluginDescription& desc);
    
    /** The actual processor object dynamic_cast'd to ProcType */
    template<class ProcType>
    inline ProcType* processor() const { return dynamic_cast<ProcType*> (proc.get()); }

    /** Returns true if the process is a graph */
    bool isSubgraph() const noexcept;

    /** Returns true if the processor is suspended */
    bool isSuspended() const;
    void suspendProcessing (const bool);

    void setInputGain(const float f);

    void setGain(const float f);

    inline float getInputGain() const { return inputGain.get(); }
    inline float getGain() const { return gain.get(); }
    inline float getLastGain() const { return lastGain.get(); }
    inline float getLastInputGain() const { return lastInputGain.get(); }
    inline void updateGain() 
    {
        if (lastGain.get() != gain.get())
            lastGain = gain;
        if (lastInputGain.get() != inputGain.get())
            lastInputGain = inputGain;
    }

    ValueTree getMetadata() const { return metadata; }
    void setMetadata (const ValueTree& meta, bool copy = false)
    {
        metadata = (copy) ? meta.createCopy() : meta;
    }

    bool isAudioIONode() const;
    bool isMidiIONode() const;

    /* returns the parent graph. If one has not been set, then
       this will return nullptr */
    GraphProcessor* getParentGraph() const;

    void setInputRMS (int chan, float val);
    float getInputRMS(int chan) const { return (chan < inRMS.size()) ? inRMS.getUnchecked(chan)->get() : 0.0f; }
    void setOutputRMS (int chan, float val);
    float getOutpputRMS(int chan) const { return (chan < outRMS.size()) ? outRMS.getUnchecked(chan)->get() : 0.0f; }

    void connectAudioTo (const GraphNode* other);

private:
    friend class GraphProcessor;

    const ScopedPointer<Processor> proc;
    bool isPrepared;
    GraphNode (uint32 nodeId, Processor*) noexcept;

    void setParentGraph (GraphProcessor*);
    void prepare (double sampleRate, int blockSize, GraphProcessor*);
    void unprepare();

    Atomic<float> gain, lastGain, inputGain, lastInputGain;
    OwnedArray<AtomicValue<float> > inRMS, outRMS;

    ValueTree metadata;
    GraphProcessor* parent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphNode)
};

/** A convenient typedef for referring to a pointer to a node object. */
typedef ReferenceCountedObjectPtr<GraphNode> GraphNodePtr;

}

#endif // EL_GRAPH_NODE_H
