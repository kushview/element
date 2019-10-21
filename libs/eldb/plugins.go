package main

import "C"
import (
	"./plugin"
)

//export eldb_plugin_search
func eldb_plugin_search(q *C.char) *C.char {
	db := open()
	db = plugin.Search(C.GoString(q), db)
	c := make(plugin.Collection, 0)
	db.Find(&c)
	db.Close()

	return C.CString(string(c.ToJSON()))
}
