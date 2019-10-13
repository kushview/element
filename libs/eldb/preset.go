package main

import "github.com/jinzhu/gorm"

type Preset struct {
	gorm.Model
	Name   string
	Format string
}
