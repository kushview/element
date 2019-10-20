package plugin

import (
	"fmt"
	"time"

	"github.com/jinzhu/gorm"
)

const (
	Table = "plugins"
)

// Plugin plugin description
type Plugin struct {
	ID        uint      `gorm:"primary_key" json:"id"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
	Name      string    `json:"name"`
	Format    string    `json:"format"`
	Favorite  bool      `json:"favorite"`
}

// Collection of plugins
type Collection []Plugin

// All plugins
func All(db *gorm.DB) Collection {
	r := make(Collection, 0)
	db.Find(&r)
	return r
}

// Search for plugins
func Search(q string, db *gorm.DB) Collection {
	r := make(Collection, 0)
	q = fmt.Sprintf("%%%s%%", q)
	db.Where("name LIKE ?", q).Find(&r)
	return r
}

// TableName ...
func (Plugin) TableName() string { return Table }
