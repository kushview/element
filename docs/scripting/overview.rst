Scripting Overview
==================

Scripts in Element are written in `Lua <https://www.lua.org>`_.

Defining a Script
-----------------

Scripts must include an `ldoc` style header that define basic metadata about 
it. The header should include a title, description and a list of properties 
encapsulated in a Lua comment section.

**Title**

    This should be the first line of the script.

**Description**

    Free form text after the first line.

**Properties**

    =========== ======================================
    Key         Description
    =========== ======================================
    @script     The script's identifier. **required**
    @type       The type of script. **required if not 
                an Anonymous script**
    @author     Who wrote it
    @license    License information
    =========== ======================================    

**Example**
    
    .. code-block:: lua

        --- Script Title.
        --
        -- The script's description.
        --
        -- @script      com.example.script
        -- @type        DSP
        -- @license     GPL v3
        -- @author      Example Author
    
        ...
