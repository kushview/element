package main

import (
	"./plugin"
	"github.com/jinzhu/gorm"
)

// Migrate the database
func Migrate(db *gorm.DB) {
	db.AutoMigrate(&plugin.Plugin{})
	db.AutoMigrate(&Preset{})
}
