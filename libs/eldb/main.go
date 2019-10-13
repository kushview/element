package main

import "C"
import (
	"fmt"
	"log"
	"net/http"
)

func init() {}

func dbopen() {
	// db := schema.open()
	// defer db.Close()

	// schema.Migrate(db)

	// // Migrate the schema
	// db.AutoMigrate(&NodeType{})

	// // Create
	// db.Create(&NodeType{Code: "L1212", Price: 1000})

	// // Read
	// var plugin NodeType
	// db.First(&plugin, 1)                   // find Plugin with id 1
	// db.First(&plugin, "code = ?", "L1212") // find Plugin with code l1212

	// // Update - update Plugin's price to 2000
	// db.Model(&plugin).Update("Price", 2000)
	// fmt.Println(plugin.Price)
	// // Delete - delete Plugin
	// db.Delete(&plugin)
}

func presets(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	fmt.Fprintf(w, "{ \"text\": \"Hi there, I love %s!\" }", r.URL.Path[1:])
}

func main() {
	http.HandleFunc("/presets.json", presets)
	log.Fatal(http.ListenAndServe(":8080", nil))
}
