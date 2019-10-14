package main

import "time"

type Model struct {
	ID        uint      `gorm:"primary_key" json:"id"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

type Deletable struct {
	DeletedAt *time.Time `sql:"index" json:"deleted_at"`
}
