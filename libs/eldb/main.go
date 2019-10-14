package main

import "C"
import (
	"encoding/json"
	"log"
	"net/http"

	"github.com/jinzhu/gorm"
)

func init() {}

func dbopen() {
	// db := schema.open()
	// defer db.Close()

	// schema.Migrate(db)

	// // Migrate the schema
	// db.AutoMigrate(&NodeType{})

	// // Update - update Plugin's price to 2000
	// db.Model(&plugin).Update("Price", 2000)
	// fmt.Println(plugin.Price)
	// // Delete - delete Plugin
	// db.Delete(&plugin)
}

func seed(db *gorm.DB) {
	db.Create(&Preset{Name: "Cool Preset", Format: "LV2"})
}

type presetsController struct {
}

func withDB(fs ...func(*gorm.DB)) {
	db := open()
	for _, f := range fs {
		f(db)
	}
	db.Close()
}

func (pc *presetsController) index(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")

	db := open()
	defer db.Close()
	var pset Preset
	db.First(&pset)

	if out, err := json.Marshal(pset); err == nil {
		w.Write(out)
	}
}

func initDB() {
	db := open()
	db.AutoMigrate(&Plugin{}, &Preset{})
	seed(db)
	db.Close()
}

func serve() {
	initDB()
	pc := new(presetsController)
	http.HandleFunc("/presets.json", pc.index)
	log.Fatal(http.ListenAndServe(":5000", nil))
}

func main() {
	serve()
}

//export eldb_model_count
func eldb_model_count() C.int {
	return 0
}
