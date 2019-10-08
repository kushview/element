package main

import "C"
import (
	"./schema"
)

func init() {}

func main() {
	db := open()
	defer db.Close()

	schema.Migrate(db)

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
