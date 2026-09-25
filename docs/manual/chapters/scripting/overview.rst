Overview
--------

Scripts in Element are written in `Lua <https://www.lua.org>`_.

Defining a Script
~~~~~~~~~~~~~~~~~

Scripts should include an `ldoc` style header that define basic metadata about 
it. The header should include a title, description and a list of properties 
as outlined in a Lua comment section.  The script itself should also return a 
descriptor table containing full implementation details.

**Title**
    This should be the first line of the script ending with a period ``.``

**Description**
    Free form text after the first line.

**Properties**

    =========== ======================================
    Key         Description
    =========== ======================================
    @script     The script's identifier. **required**
    @type       The type of script. **required**
    @author     Who wrote it
    @license    License information
    =========== ======================================    

**Example**
    
    Below is a bare-bones script definition comment.

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
