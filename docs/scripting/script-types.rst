.. _kv.AudioBuffer: https://api.kushview.net/lua/kv/latest/classes/kv.AudioBuffer.html
.. _kv.Widget: https://api.kushview.net/lua/kv/latest/classes/kv.Widget.html
.. _el.MidiPipe: https://api.kushview.net/lua/el/latest/classes/el.MidiPipe.html
.. _el.Parameter: https://api.kushview.net/lua/el/latest/classes/el.Parameter.html

Script Types
============
Element can execute a range of Lua script types to customize application behavior.

Anonymous
---------
This type of script's behavior is determined by the script author.

**Arguments:**

    Defined by the script

**Return value:**

    Defined by the script

DSP
---
This is a script which executes in a Script Node.  It's purpose is to create custom
audio and MIDI processing in your Session.

**Arguments:**

    None

**Return value:**

    Descriptor table in the following form:

.. lua:attribute:: type: string

    The type of script. MUST equal ``DSP`` for this kind of script.

.. lua:function:: layout()

    Specify the number of audio and midi inputs and outputs. Return a table
    with keys specifying the data type (audio or midi) and values defining
    the channel counts (pair of integers)

    :return: Table specifying input and output channels.
    :rtype: table
    
    .. code-block:: lua
        
        -- A 1 MIDI in and 2 Audio out DSP script
        local function my_layout()
            return {
                audio = { 0, 2 },
                midi  = { 1, 0 }
            }
        end

.. lua:function:: parameters()
    
    Specify the parameters. Return a list of tables with key/value pairs 
    defining the params.

    ===========     ================================
    **name**        (string) The parameter name
    **label**       (string) Value label. e.g. dB
    **min**         (number) Minimum value
    **max**         (number) Maximum value
    **default**     (number) Default value
    ===========     ================================

    :return: The parameters.
    :rtype: table

.. lua:function:: prepare (rate, block)
    
    Prepare for rendering. Allocate needed resources here.

    :param rate: Sample rate
    :type rate: number
    :param block: Block size
    :type block: integer

.. lua:function:: process (audio, midi, params)

    Process audio and MIDI. The passed in buffers require replace processing.

    :param audio: The audio buffer to use
    :type audio: `kv.AudioBuffer`_
    :param midi: The midi to use
    :type midi: `el.MidiPipe`_
    :param params: Array of parameter values
    :type params: array

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

DSPUI
-----
This is a UI for a DSP script.

**Arguments:**

    None

**Return value:**

    A descriptor table in the following form:

.. lua:attribute:: type: string

    Must always equal ``DSPUI``

.. lua:function:: editor(ctx)

    Implement this and return a kv.Widget to be used as the editor for the DSP 
    script. The editor UI will be displayed in the Plugin Window of the Script Node.

    Note: DSPUI scripts must be able to create multiple instances of it's widgets. Do
    not create singleton widgets and return them in this method.

    The ``ctx`` parameter is a table with the following keys.

    ==========  ==========================================
    **params**  (array) List of `el.Parameter`_ objects as 
                defined in the DSP script.
    ==========  ==========================================

    :param ctx: The owner context of this
    :type ctx: table

    :return: The widget to use as the editor.
    :rtype: `kv.Widget`_
