package main

import (
	"./plugin"
	"github.com/jinzhu/gorm"
)

func seed(db *gorm.DB) {
	db.Create(&Preset{Name: "Cool Preset 1", Format: "LV2"})
	db.Create(&Preset{Name: "Cool Preset 2", Format: "LV2"})

	db.Create(&plugin.Plugin{Name: "Amplifier", Format: "LV2", Favorite: false})
	db.Create(&plugin.Plugin{Name: "Compressor", Format: "LV2", Favorite: false})
}
