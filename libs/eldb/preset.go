package main

import (
	"encoding/json"
	"net/http"
)

// Preset model
type Preset struct {
	Model
	Name   string `json:"name"`
	Format string `json:"format"`
}

type presetsController struct {
}

func (pc *presetsController) index(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")

	db := open()
	defer db.Close()
	var pset Preset
	db.First(&pset)

	if out, err := json.Marshal(pset); err == nil {
		w.Write(out)
	}
}
