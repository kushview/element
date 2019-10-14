package main

type Preset struct {
	Model
	Name   string `json:"name"`
	Format string `json:"format"`
}
