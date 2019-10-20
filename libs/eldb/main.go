package main

import "C"
import (
	"encoding/json"
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

	if out, err := json.Marshal(plugin.Search(q, db)); err == nil {
		w.Write(out)
	}

	db.Close()
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

//export eldb_preset_count
func eldb_preset_count() C.int {
	return 0
}
