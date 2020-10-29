--- Test Script
-- Convert something to XML
-- @script toxml
-- @usage
-- local xml = element.script ('toxml')
-- print (xml)
-- @usage
-- # From command line...

local sess = element.world():session()
return sess:toxmlstring()
