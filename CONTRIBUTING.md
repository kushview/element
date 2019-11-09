# Contributing to Element
This document is currently incomplete. More details coming soon!

### Static Data, Global Variables, and Singletons
Because Element is designed to run as a plugin in other software, usage of these types is strictly forbidden.  Certain cases might be ok such as embedding binary data (e.g. fonts, images), but should still be avoided.

### Changing the Audio/Rendering Engine
If you change things in the Engine layer, please thoroughly test before submitting pull requests.  The slightest changes may work fine on your system but cause nasty crashes or unexpected behavior elsewhere.

### Adding Features to or New Nodes
When adding properties to Nodes or making new ones, Please make sure that the state can be saved or restored.  There's no point in adding new functions to Nodes if they can't restore state when a session is saved then opened later.

### Adding New Files
Please use Copyright 20xx.  Kushview, LLC. and below this Author Your Name <your@email.com>
