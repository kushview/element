package main

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/jinzhu/gorm"
)

// Preset model
type Preset struct {
	Model
	Name   string `json:"name"`
	Format string `json:"format"`
}

type presetsController struct {
}

func searchPresets(q string, db *gorm.DB) []Preset {
	psets := make([]Preset, 0)
	q = fmt.Sprintf("%%%s%%", q)
	fmt.Println(q)
	db.Where("name LIKE ?", q).Find(&psets)
	return psets
}

func (pc *presetsController) index(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")

	db := open()
	defer db.Close()

	psets := searchPresets("Preset", db)
	if out, err := json.Marshal(psets); err == nil {
		w.Write(out)
	}
}
