package main

import "C"
import (
	"log"
	"net/http"

	"github.com/jinzhu/gorm"
)

func withDB(fs ...func(*gorm.DB)) {
	db := open()
	for _, f := range fs {
		f(db)
	}
	db.Close()
}

func initDB() {
	db := open()
	db.AutoMigrate(&Plugin{}, &Preset{})
	db.Close()
}

func serve() {
	initDB()
	pc := new(presetsController)
	http.HandleFunc("/presets.json", pc.index)
	log.Fatal(http.ListenAndServe(":5000", nil))
}

func init() {}

func main() {
	serve()
}

//export eldb_preset_count
func eldb_preset_count() C.int {
	return 0
}
