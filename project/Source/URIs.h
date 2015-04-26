/*
    URIs.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ELEMENT_APP_URIS_H
#define ELEMENT_APP_URIS_H

#define BEATTHANG_URI "http://bketech.com/ns/beatthang"

#define ELEMENT_ABOUT_BOX       BEATTHANG_URI "#aboutBox"
#define ELEMENT_PREFERENCES     BEATTHANG_URI "#preferences"
#define ELEMENT_LEGACY_WINDOW   BEATTHANG_URI "#legacyWindow"
#define ELEMENT_PLUGIN_MANAGER  BEATTHANG_URI "#pluginManager"

namespace Element {

    class URIs {
    public:
        URIs() { }
    };
    
}

#endif // ELEMENT_APP_URIS_H
