package main

import (
	"github.com/jinzhu/gorm"
)

// Migrate the database
func Migrate(db *gorm.DB) {
	db.AutoMigrate(&Plugin{})
	db.AutoMigrate(&Preset{})
}
