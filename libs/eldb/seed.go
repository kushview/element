package main

import "github.com/jinzhu/gorm"

func seed(db *gorm.DB) {
	db.Create(&Preset{Name: "Cool Preset", Format: "LV2"})
}
