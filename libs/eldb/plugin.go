package main

import "github.com/jinzhu/gorm"

type Plugin struct {
	gorm.Model
	Name     string
	Format   string
	Favorite bool
}
