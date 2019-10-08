package schema

import (
	"github.com/jinzhu/gorm"
)

type NodeType struct {
	gorm.Model

	ID     int
	Name   string
	Format string

	Code  string
	Price uint
}

type Preset struct {
	gorm.Model

	ID     int
	Name   string
	Format string

	Code  string
	Price uint
}

func Migrate(db *gorm.DB) {
	db.AutoMigrate(&NodeType{}, &Preset{})
}
