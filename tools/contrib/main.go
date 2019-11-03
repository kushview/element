// This program prints contributors to the project on the console
package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
)

const contribURL = "https://api.github.com/repos/kushview/element/contributors"
const token = "<token>"

// User return object from github api
type User struct {
	Login         string
	URL           string
	Contributions int
}

// Contributor return object from github api
type Contributor struct {
	Name  string
	Login string
}

func main() {
	r, err := http.Get(contribURL)
	if err != nil {
		panic(err)
	}

	defer r.Body.Close()
	bytes, err := ioutil.ReadAll(r.Body)
	if err != nil {
		panic(err)
	}

	users := make([]User, 0)
	json.Unmarshal(bytes, &users)

	contribs := make([]Contributor, 0)
	for _, user := range users {
		if user.Login == "mfisher31" {
			continue
		}
		cr, err := http.Get(user.URL)
		if err != nil {
			continue
		}
		defer cr.Body.Close()
		bytes, err := ioutil.ReadAll(cr.Body)
		if err != nil {
			continue
		}
		var c Contributor
		if nil == json.Unmarshal(bytes, &c) {
			contribs = append(contribs, c)
		}
	}

	for _, c := range contribs {
		if len(c.Name) > 0 && len(c.Login) > 0 {
			fmt.Printf("%s (%s)\n", c.Name, c.Login)
		} else if len(c.Name) > 0 {
			fmt.Println(c.Name)
		} else if len(c.Login) > 0 {
			fmt.Println(c.Login)
		}
	}
}
