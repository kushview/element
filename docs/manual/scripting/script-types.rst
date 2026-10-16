.. include:: /shortcuts.rstext

.. _el.AudioBuffer: https://kushview.github.io/element-ldoc/classes/el.AudioBuffer.html
.. _el.Widget: https://kushview.github.io/element-ldoc/classes/el.Widget.html
.. _el.MidiPipe: https://kushview.github.io/element-ldoc/classes/el.MidiPipe.html
.. _el.MidiBuffer: https://kushview.github.io/element-ldoc/classes/el.MidiBuffer.html
.. _el.Parameter: https://kushview.github.io/element-ldoc/classes/el.Parameter.html

.. _scripting-script-types:

Script Types
============
|El| runs several kinds of Lua script. Each returns a descriptor table whose
``type`` field names the kind, and whose other fields are the callbacks
described here.

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

.. lua:attribute:: ui: string

    Optional. The identifier of the DSPUI script that provides this script's
    editor, for example ``"ampui"``.

.. lua:function:: init()

    Optional. Called once after the script is loaded, before ``prepare``.

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
                -- ins and outs for parameters are the same as audio and midi, but
                -- the in/out pairs are tables.
                control = {                
                    {
                        -- List of 'parameters', e.g. control input ports.
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
    :type p: table
    :param c: Array of control output values ("controls")
    :type c: table
    :param t: Time information. See Position section below.
    :type t: table

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
    ``io.read(...)`` your data previously written in ``save()``
    
    .. code-block:: lua

        function my_restore()
            print ("restored data:")
            print (io.read ("*a"));
        end

Control Table
~~~~~~~~~~~~~
Complete list of fields when defining controls in the layout.  Fields without a
default value are required.

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Field
     - Description
   * - ``name``
     - (string) The parameter name.
   * - ``symbol``
     - (string) A unique symbol for the parameter. Must be unique across
       all ports.
   * - ``label``
     - (string) Value label, e.g. dB. Default is blank.
   * - ``type``
     - (string) Type of value. Default is ``float``.
   * - ``min``
     - (number) Minimum value.
   * - ``max``
     - (number) Maximum value.
   * - ``default``
     - (number) Default value.

Position Object
~~~~~~~~~~~~~~~
All items in the below table are methods on the object. Since the values represent
the current time of the transport, they are 0-indexed unlike `el.AudioBuffer`_
and `el.MidiBuffer`_ which are 1-indexed.

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Method
     - Description
   * - ``playing()``
     - (bool) True if playing.
   * - ``recording()``
     - (bool) True if recording.
   * - ``looping()``
     - (bool) True if looping.
   * - ``valid()``
     - (bool) True if the time information is sane.
   * - ``frame()``
     - (number) Current time in audio frames.
   * - ``seconds()``
     - (number) Current time in seconds.
   * - ``bpm()``
     - (number) Current beats per minute.
   * - ``beatsPerBar()``
     - (number) Numerator of the time signature.
   * - ``beatUnit()``
     - (number) Denominator of the time signature.
   * - ``bar()``
     - (number) The current bar.
   * - ``beat()``
     - (number) The current beat, in quarter notes.

DSPUI
-----
This is a UI for a DSP script.

**Arguments:**

    None

**Return value:**

    A descriptor table in the following form:

.. lua:attribute:: type: string

    Must always equal ``DSPUI``

.. lua:function:: instantiate (context)

    Implement this and return an `el.Widget`_ to be used as the editor for the DSP 
    script. The editor UI will be displayed in the Plugin Window of the Script Node.

    `Note`: DSPUI scripts should always return a new instance of `el.Widget`.

    :param context: The owner context of this UI
    :type context: table

    :return: The widget to use as the editor.
    :rtype: `el.Widget`_

.. lua:function:: destroy (widget)

    Called by Element when the editor is will be deleted. This is handy to use, 
    for example, when resources need freed but can't or don't want to wait for 
    Lua's :term:`garbage collector`. All references to the widget should be dropped
    if possible.

Context
~~~~~~~
The ``context`` parameter in ``instantiate(...)`` is a table containing these 
properties:

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Field
     - Contents
   * - ``params``
     - (table) List of control input objects, e.g. parameters. Indexes match
       the order defined in the DSP layout. See `Control Object`_ below.
   * - ``controls``
     - (table) List of control output objects, e.g. controls. Indexes match
       the order defined in the DSP layout. See `Control Object`_ below.
   * - ``symbol``
     - Every control input and output is also assigned to a field keyed by
       its symbol. If a control's symbol is ``volume``, ``ctx.volume`` is
       available during instantiation. See `Control Object`_ below.

Control Object
~~~~~~~~~~~~~~
.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Method
     - Description
   * - ``min()``
     - (number) Returns the minimum value.
   * - ``max()``
     - (number) Returns the maximum value.
   * - ``get()``
     - (number) Returns the current value of the control.
   * - ``set (value)``
     - Sets a new value.
   * - ``changed``
     - (field) Set this to a function with no arguments to handle value
       changes.

Content
-------

Experimental. A Content script provides a widget that |El| shows as a main
view.

**Return value:**

    A descriptor table with ``type = "Content"`` and an ``instantiate (ctx)``
    function returning an `el.Widget`_.

View
----

Experimental. A View script provides a widget shown in the user interface
and is told when the graph in the editor changes.

**Return value:**

    A descriptor table with ``type = "View"``, an ``instantiate()`` function
    returning an ``el.View``, and an optional ``graph_changed (self, graph)``
    callback.

.. code-block:: lua

   local object = require ('el.object')

   local function instantiate()
       local w = object.new ('el.View')
       w.paint = function (_, g)
           g:fillAll (0xff00ff00)
           g:setColor (0xffffffff)
           g:drawText ("Hello World", w:localBounds())
       end
       return w
   end

   return {
       type = "View",
       instantiate = instantiate,
       graph_changed = function (_, graph) end
   }

Anonymous
---------

An Anonymous script's arguments and return value are defined by its author.
They are run from the console or from other scripts with
``script.exec (name)``, and can do anything the API allows: the
:ref:`helloworld example <scripting-examples>` opens a window with a custom
widget, and the console's own start-up script is Anonymous too.
