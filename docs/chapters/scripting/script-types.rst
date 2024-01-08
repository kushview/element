.. _el.AudioBuffer: https://kushview.net/api/el/latest/classes/kv.AudioBuffer.html
.. _el.Widget: https://kushview.net/api/el/latest/classes/el.Widget.html
.. _el.MidiPipe: https://kushview.net/api/el/latest/classes/el.MidiPipe.html
.. _el.MidiBuffer: https://kushview.net/api/el/latest/classes/el.MidiBuffer.html
.. _el.Parameter: https://kushview.net/api/el/latest/classes/el.Parameter.html

Script Types
============
Element can execute a range of Lua script types to customize application behavior.

DSP
---
This is a script which runs inside a 'Script' internal instance.  They can be 
used in making specialized Nodes -- for example a specialized MIDI filter or set 
of output parameters acting as a meta control to other Nodes.

**Execution Mode:**
    The DSP type is self-contained and loads for each instance needed.  In other words
    the script itself is the instance.  This is done to reduce calls into Lua, usage
    of extra metatables, etc... that could affect realtime performance.  In terms
    of OOP, 'class' variables can be declared in the global scope, yet do not affect
    other instances.

**Arguments:**
    None

**Return value:**
    Descriptor table in the following form:

.. lua:attribute:: type: string

    The type of script. MUST equal ``DSP`` for this kind of script.

.. lua:function:: layout()

    Specify the number of audio and midi inputs and outputs. Return a table
    with keys specifying the data type (audio or midi) and values defining
    the channel counts.  The values should be pairs port counts, or Control 
    Tables.

    :return: Table specifying input and output channels.
    :rtype: table
    
    .. code-block:: lua
        
        -- A 1 MIDI in and 2 Audio out DSP script
        local function my_layout()
            return {
                -- two ins, two outs
                audio = { 2, 2 },
                -- only need one MIDI in.
                midi  = { 1, 0 },
                -- ins and outs for parameters is the same as audio and midi, but
                -- the in/out pairs of of tables.
                control = {                
                    {
                        -- List of 'paramters', e.g. control input ports.
                        {
                            name = "Volume",
                            symbol = "volume",
                            min = -90.0,
                            max = 24.0,
                            default = 0.0
                        },
                        {
                            name = "Another",
                            symbol = "another",
                            min = 0.0,
                            max = 1.0,
                            default = 1.0
                        }
                    },
                    {
                        -- List of 'controls', e.g. control output ports.    
                        {
                            name = "MIDI CC",
                            symbol = "midi_cc",
                            min = 0.0,
                            max = 127.0,
                            default = 1.0
                        }
                    }
                }
            }
        end

.. lua:function:: prepare (rate, block)
    
    Prepare for rendering. Allocate needed resources here.

    :param rate: Sample rate
    :type rate: number
    :param block: Block size
    :type block: integer

.. lua:function:: process (a, m, p, c, t)

    Process audio and MIDI. The passed in audio and midi buffers expect replace 
    processing.

    :param a: The audio buffer to use
    :type a: `el.AudioBuffer`_
    :param m: The midi pipe to use
    :type m: `el.MidiPipe`_
    :param p: Array of control input values ("parameters")
    :type p: 
    :param c: Array of control output values ("controls")
    :type c: 
    :param t: Time information. See Position section below.
    :type p: 

.. lua:function:: release()

    Release allocated resources.

.. lua:function:: save()

    Save the current state. This is an optional function you can implement to save state.  
    The host will prepare the IO stream so all you have to do is 
    ``io.write(...)`` your data.

    Note: Parameter values will automatically be saved and restored,
    you do not need to handle them here.

    .. code-block:: lua

        local function my_save()
            io.write ("some custom state data")
        end

.. lua:function:: restore()

    Restore state. This is an optional function you can implement to restore state.  
    The host will prepare the IO stream so all you have to do is 
    ``io.read(...)`` your data previsouly written in ``save()``
    
    .. code-block:: lua

        function my_restore()
            print ("restored data:")
            print (io.read ("*a"));
        end

Control Table
*************
Complete list of fields when defining controls in the layout.  Fields without a
default value are required.

===========     ================================
**name**        (string) The parameter name
**name**        (string) A unique symbol for the param. Must be unique across all ports.
**label**       (string) Value label. e.g. dB.  default is blank
**type**        (string) Type of value. default=float
**min**         (number) Minimum value
**max**         (number) Maximum value
**default**     (number) Default value
===========     ================================

Position Object
***************
All items in the below table are methods on the object. Since the values represent
the current time of the transport, they are 0-indexed unlike `el.AudioBuffer`_
and `el.MidiBuffer`_ which are 1-indexed.

================= ==============================================================
**playing()**     (bool) Returns true if playing.
**recording()**   (bool) Returns true if recording.
**looping()**     (bool) Returns true if looping.
**valid()**       (bool) Returns true if time info is sane.
**frame()**       (number) Current time in audio frames.
**seconds()**     (number) Current time in seconds.
**bpm()**         (number) Current beats per minute.
**beatsPerBar()** (number) Numerator of the time signature.
**beatUnit()**    (number) Denominator of the time signature.
**bar()**         (number) The current bar.
**beat()**        (number) The current beat in terms of quarter note.
================= ==============================================================

DSPUI
-----
This is a UI for a DSP script.

**Arguments:**

    None

**Return value:**

    A descriptor table in the following form:

.. lua:attribute:: type: string

    Must always equal ``DSPUI``

.. lua:function:: instantiate (ctx)

    Implement this and return an `el.Widget`_ to be used as the editor for the DSP 
    script. The editor UI will be displayed in the Plugin Window of the Script Node.

    Note: DSPUI scripts must be able to create multiple instances of it's widgets. Do
    not create singleton widgets and return them in this method.

    :param ctx: The owner context of this
    :type ctx: table

    :return: The widget to use as the editor.
    :rtype: `el.Widget`_

Context
*******
The ``ctx`` parameter in ``instantiate(...)`` is a table containing these properties:

============    ============================================================
**params**      (table) List of control input objects. e.g Parameters. Indexes 
                will match the order defined in the DSP layout. See `Control Object`_
                below for details.
**controls**    (table) List of control output objects e.g. Controls. Indexes 
                will match the order defined in the DSP layout. See `Control Object`_
                below for details.
**symbol**      (table) Control ins and outs will all be assigned to a field keyed
                with its symbol.  For example, if your control's symbol is 
                'volume', then then you will also have `ctx.volume` available 
                during instantiation. See `Control Object`_
                below for details.
============    ============================================================

Control Object
**************
=============== ================================================================
**min()**       (number) returns the minimum value.
**max()**       (number) returns the maximum value.
**get()**       (number) returns the current value of the control.
**set (value)** (void) Set a new value.
**changed()**   (field) Set this to a plain 'void' function to handle when the
                value changes.
=============== ================================================================


.. View
.. -----
.. This is a View.

.. **Arguments:**

..     None

.. **Return value:**

..     A descriptor table in the following form:

.. .. lua:attribute:: type: string

..     Must always equal ``DSPUI``

.. .. lua:function:: instantiate()

..     Implement this and return a el.Widget to be used as a View in the UI. 

..     :return: The widget to use as the editor.
..     :rtype: el.Widget

.. Anonymous
.. ---------
.. This type of script's behavior is determined by the script author.

.. **Arguments:**

..     Defined by the script

.. **Return value:**

..     Defined by the script
