package plugin

import "C"
import (
	"encoding/json"
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

//=============================================================================

// TableName ...
func (Plugin) TableName() string { return Table }

//=============================================================================

// ToJSON converts to a JSON encoded byte array
func (c Collection) ToJSON() []byte {
	if out, err := json.Marshal(c); err == nil {
		return out
	}
	return make([]byte, 0)
}

//=============================================================================

// All plugins
func All(db *gorm.DB) Collection {
	r := make(Collection, 0)
	db.Find(&r)
	return r
}

// Search for plugins
func Search(q string, db *gorm.DB) *gorm.DB {
	q = fmt.Sprintf("%%%s%%", q)
	return db.Where("name LIKE ?", q)
}

// Favorites ...
func Favorites(db *gorm.DB) *gorm.DB {
	return db.Where("favorite = ?", true)
}
