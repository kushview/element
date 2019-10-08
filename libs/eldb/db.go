package main

import (
	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
)

const database = "test.db"

func open() *gorm.DB {
	db, _ := gorm.Open("sqlite3", database)
	return db
}
