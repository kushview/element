package main

import "C"
import (
	"log"
	"net/http"

	"./plugin"
	"github.com/jinzhu/gorm"
)

func withDB(fs ...func(*gorm.DB)) {
	db := open()
	for _, f := range fs {
		f(db)
	}
	db.Close()
}

func migrate(db *gorm.DB) {
	Migrate(db)
}

func searchPlugins(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	keys, ok := r.URL.Query()["q"]
	if !ok || len(keys[0]) < 1 {
		return
	}

	q := keys[0]

	db := open()
	defer db.Close()

	db = plugin.Search(q, db)
	plugins := make(plugin.Collection, 0)
	db.Find(&plugins)
	w.Write(plugins.ToJSON())
}

func serve() {
	pc := new(presetsController)
	http.HandleFunc("/presets.json", pc.index)
	http.HandleFunc("/plugins/search.json", searchPlugins)
	log.Fatal(http.ListenAndServe(":5000", nil))
}

func init() {}

func main() {
	db := open()
	migrate(db)
	seed(db)
	db.Close()
	serve()
}

//export eldb_migrate
func eldb_migrate() {
	db := open()
	defer db.Close()
	migrate(db)
}

//export eldb_preset_count
func eldb_preset_count() C.int {
	return 0
}

//export eldb_plugin_search
func eldb_plugin_search(q *C.char) *C.char {
	db := open()
	db = plugin.Search(C.GoString(q), db)
	c := make(plugin.Collection, 0)
	db.Find(&c)
	db.Close()

	return C.CString(string(c.ToJSON()))
}
