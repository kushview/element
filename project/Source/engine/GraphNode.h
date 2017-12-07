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

    virtual ~GraphNode();
    
    /** A set of user-definable properties that are associated with this node.

        This can be used to attach values to the node for whatever purpose seems
        useful. For example, you might store an x and y position if your application
        is displaying the nodes on-screen.
    */
    NamedValueSet properties;

    static GraphNode* createForRoot (GraphProcessor*);
    
    /** Returns the processor as an AudioProcessor */
    AudioProcessor* getAudioProcessor() const noexcept { return proc; }
    
    /** Returns the actual processor object that this node represents. */
    Processor* getProcessor() const noexcept { return dynamic_cast<Processor*> (proc.get()); }

    /** Returns the processor as an Audio Plugin Instance */
    AudioPluginInstance* getAudioPluginInstance() const;

    int getNumAudioInputs() const;
    int getNumAudioOutputs() const;
    
    PortType getPortType (const uint32 port) const;
    uint32 getNumPorts() const;
    int getNumPorts (const PortType type, const bool isInput) const;
    uint32 getPortForChannel (const PortType type, const int channel, const bool isInput) const;
    
    int getChannelPort (const uint32 port) const;
    
    int getNthPort (const PortType portType, const int inputChan, bool, bool) const;
    
    /** Returns true if an input port */
    bool isPortInput (const uint32 port) const;

    /** Returns true if an output port */
    bool isPortOutput (const uint32 port) const;
    
    bool isGraph() const;
    
    uint32 getMidiInputPort() const;
    uint32 getMidiOutputPort() const;
    
    /** If an audio plugin instance, fill the details */
    void getPluginDescription (PluginDescription& desc);
    
    /** The actual processor object dynamic_cast'd to ProcType */
    template<class ProcType>
    inline ProcType* processor() const { return dynamic_cast<ProcType*> (proc.get()); }

    /** Returns true if the process is a graph */
    bool isSubgraph() const noexcept;

    /** Returns true if the processor is suspended */
    bool isSuspended() const;
    
    /** Suspend processing */
    void suspendProcessing (const bool);

    /** Set the Input Gain of this Node */
    void setInputGain (const float f);

    /** Set the Gain of this Node */
    void setGain (const float f);

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
    friend class EngineController;
    friend class Node;

    ScopedPointer<AudioProcessor> proc;
    bool isPrepared;
    GraphNode (uint32 nodeId, AudioProcessor*) noexcept;

    void setParentGraph (GraphProcessor*);
    void prepare (double sampleRate, int blockSize, GraphProcessor*);
    void unprepare();
    void resetPorts();
    
    Atomic<float> gain, lastGain, inputGain, lastInputGain;
    OwnedArray<AtomicValue<float> > inRMS, outRMS;
    
    ChannelConfig channels;
    ValueTree metadata, node;
    GraphProcessor* parent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphNode)
};

/** A convenient typedef for referring to a pointer to a node object. */
typedef ReferenceCountedObjectPtr<GraphNode> GraphNodePtr;

}

#endif // EL_GRAPH_NODE_H
