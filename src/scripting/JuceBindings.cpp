
#include "JuceHeader.h"
#include "sol/sol.hpp"

namespace Element {
namespace Lua {

template<typename T>
static auto addRange (sol::table& view, const char* name)
{
    using R = Range<T>;
    return view.new_usertype<R> (name, sol::no_constructor,
        sol::call_constructor, sol::factories (
            []() { return R(); },
            [] (T start, T end) { return R (start, end); }
        ),
        "empty",            sol::readonly_property (&R::isEmpty),
        "start",            sol::property (&R::getStart,  &R::setStart),
        "length",           sol::property (&R::getLength, &R::setLength),
        "end",              sol::property (&R::getEnd,    &R::setEnd),
        "clip",             &R::clipValue,
        "contains",         [](R* self, R* other) { return self->contains (*other); },
        "intersects",       [](R* self, R* other) { return self->intersects (*other); },
        "expanded",         &R::expanded
    );
}

template<typename T>
static auto addRectangle (sol::table& view, const char* name)
{
    using R = Rectangle<T>;
    return view.new_usertype<R> (name, 
        sol::constructors<R(), R(T, T, T, T), R(T, T), R(Point<T>, Point<T>)>(),
        
        sol::meta_method::to_string, [](R& self) {
            return self.toString().toStdString();
        },

        "leftTopRightBottom", R::leftTopRightBottom,

        "isEmpty",          &R::isEmpty,
        "isFinite",         &R::isFinite,
        "getX",             &R::getX,
        "getY",             &R::getY,
        "getWidth",         &R::getWidth,
        "getHeight",        &R::getHeight,
        "getRight",         &R::getRight,
        "getBottom",        &R::getBottom,
        "getCentreX",       &R::getCentreX,
        "getCentreY",       &R::getCentreY,
        "getCentre",        &R::getCentre,
        "getAspectRatio", sol::overload (
            [](R& self) { return self.getAspectRatio(); },
            [](R& self, bool widthOverHeight) { return self.getAspectRatio (widthOverHeight); }
        ),

        "getPosition",     &R::getPosition,
        "setPosition", sol::overload (
            sol::resolve<void(Point<T>)> (&R::setPosition),
            sol::resolve<void(T, T)> (&R::setPosition)
        ),
        "getTopLeft",           &R::getTopLeft,
        "getTopRight",          &R::getTopRight,
        "getBottomLeft",        &R::getBottomLeft,
        "getBottomRight",       &R::getBottomRight,
        "getHorizontalRange",   &R::getHorizontalRange,
        "getVerticalRange",     &R::getVerticalRange,
        "setSize",              &R::setSize,
        "setBounds",            &R::setBounds,
        "setX",                 &R::setX,
        "setY",                 &R::setY,
        "setWidth",             &R::setWidth,
        "setHeight",            &R::setHeight,
        "setCentre", sol::overload (
            sol::resolve<void(T, T)> (&R::setCentre),
            sol::resolve<void(Point<T>)> (&R::setCentre)
        ),
        "setHorizontalRange",   &R::setHorizontalRange,
        "setVerticalRange",     &R::setVerticalRange,
        "withX",                &R::withX,
        "withY",                &R::withY,
        "withRightX",           &R::withRightX,
        "withBottomY",          &R::withBottomY,
        "withPosition", sol::overload (
            [](R& self, T x, T y)       { return self.withPosition (x, y); },
            [](R& self, Point<T> pt)    { return self.withPosition (pt); }
        ),
        "withZeroOrigin",       &R::withZeroOrigin,
        "withCentre",           &R::withCentre,

        "withWidth",            &R::withWidth,
        "withHeight",           &R::withHeight,
        "withSize",             &R::withSize,
        "withSizeKeepingCentre",&R::withSizeKeepingCentre,
        "setLeft",              &R::setLeft,
        "withLeft",             &R::withLeft,

        "setTop",               &R::setTop,
        "withTop",              &R::withTop,
        "setRight",             &R::setRight,
        "withRight",            &R::withRight,
        "setBottom",            &R::setBottom,
        "withBottom",           &R::withBottom,
        "withTrimmedLeft",      &R::withTrimmedLeft,
        "withTrimmedRight",     &R::withTrimmedRight,
        "withTrimmedTop",       &R::withTrimmedTop,
        "withTrimmedBottom",    &R::withTrimmedBottom,
        "translate",            &R::translate,
        "translated",           &R::translated,
        "expand",               &R::expand,
        "expanded", sol::overload (
            [](R& self, T dx, T dy) { return self.expanded (dx, dy); },
            [](R& self, T d)        { return self.expanded (d); }
        ),
        "reduce",               &R::reduce,
        "reduced", sol::overload (
            [](R& self, T dx, T dy) { return self.reduced (dx, dy); },
            [](R& self, T d)        { return self.reduced (d); }
        ),
        "removeFromTop",        &R::removeFromTop,
        "removeFromLeft",       &R::removeFromLeft,
        "removeFromRight",      &R::removeFromRight,
        "removeFromBottom",     &R::removeFromBottom,

        "getConstrainedPoint",  &R::getConstrainedPoint,

        "getRelativePoint",     [](R& self, T rx, T ry) { return self.getRelativePoint (rx, ry); },
        "proportionOfWidth",    [](R& self, T p)        { return self.proportionOfWidth (p); },
        "proportionOfHeight",   [](R& self, T p)        { return self.proportionOfHeight (p); },
        "getProportion",        [](R& self, R pr)       { return self.getProportion (pr); }
#if 0
        "",     &R::,
#endif
    );
}

//=============================================================================
void openJUCE (sol::state& lua)
{
    auto M = lua.create_table();
    auto tmp = lua.create_table();

    addRange<int> (M, "Range");
    addRectangle<int> (M, "Rectangle");

    M.new_usertype<Justification> ("Justification", sol::no_constructor);
    M["Justification"]["Flags"] = tmp.new_enum ("Flags",
        "left",                     Justification::left,
        "right",                    Justification::right,
        "horizontallyCentred",      Justification::horizontallyCentred,
        "top",                      Justification::top,
        "bottom",                   Justification::bottom,
        "verticallyCentred",        Justification::verticallyCentred,
        "horizontallyJustified",    Justification::horizontallyJustified,
        "centredLeft",              Justification::centredLeft,
        "centredRight",             Justification::centredRight,
        "centredTop",               Justification::centredTop,
        "centredBottom",            Justification::centredBottom,
        "topLeft",                  Justification::topLeft,
        "topRight",                 Justification::topRight,
        "bottomLeft",               Justification::bottomLeft,
        "bottomRight",              Justification::bottomRight
    );
    tmp.clear();

    M.new_usertype<Colour> ("Colour", sol::no_constructor,
        "new", sol::factories (
            [](int code) { return Colour (static_cast<uint32> (code)); },
            [](int r, int g, int b) { 
                return Colour (
                    static_cast<uint8> (jlimit (0, 127, r)),
                    static_cast<uint8> (jlimit (0, 127, g)),
                    static_cast<uint8> (jlimit (0, 127, b))
                );
            }
        ),
        "fromRGB",                      Colour::fromRGB,
        "fromRGBA",                     Colour::fromRGBA,
        "fromFloatRGBA",                Colour::fromFloatRGBA,
        "fromHSV",                      Colour::fromHSV,
        "fromHSL",                      Colour::fromHSL,

        "getRed",                       &Colour::getRed,
        "getGreen",                     &Colour::getGreen,
        "getBlue",                      &Colour::getBlue,
        "getFloatRed",                  &Colour::getFloatRed,
        "getFloatGreen",                &Colour::getFloatGreen,
        "getFloatBlue",                 &Colour::getFloatBlue,
        "getPixelARGB",                 &Colour::getPixelARGB,
        "getARGB",                      &Colour::getARGB,

        "getAlpha",                     &Colour::getAlpha,
        "getFloatAlpha",                &Colour::getFloatAlpha,
        "isOpaque",                     &Colour::isOpaque,
        "isTransparent", &Colour::isTransparent,
        "withAlpha", sol::overload (
            [](Colour& self, int alpha) -> Colour {
                return self.withAlpha ((uint8) jlimit (0, 127, alpha));
            },
            [](Colour& self, float alpha) -> Colour {
                return self.withAlpha (alpha);
            }
        ),
        "withMultipliedAlpha",          &Colour::withMultipliedAlpha,

        "overlaidWith",                 &Colour::overlaidWith,
        "interpolatedWith",             &Colour::interpolatedWith,

        "getHue",                       &Colour::getHue,
        "getSaturation",                &Colour::getSaturation,
        "getSaturationHSL",             &Colour::getSaturationHSL,
        "getBrightness",                &Colour::getBrightness,
        "getLightness",                 &Colour::getLightness,
        "getPerceivedBrightness",       &Colour::getPerceivedBrightness,
        "getHSB", [](Colour& self) {
            float h, s, b; self.getHSB (h, s, b);
            return std::make_tuple (h, s, b);
        },
        "getHSL", [](Colour& self) {
            float h, s, l; self.getHSL (h, s, l);
            return std::make_tuple (h, s, l);
        },
        
        "withHue",                      &Colour::withHue,
        "withSaturation",               &Colour::withSaturation,
        "withSaturationHSL",            &Colour::withSaturationHSL,
        "withBrightness",               &Colour::withBrightness,
        "withLightness",                &Colour::withLightness,
        "withRotatedHue",               &Colour::withRotatedHue,
        "withMultipliedSaturation",     &Colour::withMultipliedSaturation,
        "withMultipliedSaturationHSL",  &Colour::withMultipliedSaturationHSL,
        "withMultipliedBrightness",     &Colour::withMultipliedBrightness,
        "withMultipliedLightness",      &Colour::withMultipliedLightness,
        
        "brighter", sol::overload (
            [](Colour& self) { return self.brighter(); },
            [](Colour& self, float amt) { return self.brighter (amt); }
        ),
        "darker", sol::overload (
            [](Colour& self) { return self.darker(); },
            [](Colour& self, float amt) { return self.darker (amt); }
        ),

        "contrasting", sol::overload (
            [](Colour& self) { return self.contrasting (); },
            [](Colour& self, float amt) { return self.contrasting (amt); },
            [](Colour& self, Colour c, float ml) { return self.contrasting (c, ml); }
        ),
        
        // renamed contrasted because sol can't handle static
        "contrasted", [](Colour c1, Colour c2) { 
            return Colour::contrasting (c1, c1);
        },

        "greyLevel",                    Colour::greyLevel,
        
        "toString",                     &Colour::toString,
        "fromString",                   Colour::fromString,
        "toDisplayString",              &Colour::toDisplayString
    );

    M.new_usertype<Graphics> ("Graphics", sol::no_constructor,
        "setColour", sol::overload (
            [](Graphics& g, int color) { g.setColour (Colour (color)); },
            [](Graphics& g, Colour color) { g.setColour (color); }
        ),
        "drawText", sol::overload (
            [](Graphics& g, std::string t, int x, int y, int w, int h) {
                g.drawText (t, x, y, w, h, Justification::centred, true);
            }
        ),
        "fillAll", sol::overload (
            [](Graphics& g)                 { g.fillAll(); },
            [](Graphics& g, int color)      { g.fillAll (Colour (color)); },
            [](Graphics& g, Colour color)   { g.fillAll (color); }
        )
    );

    M.new_usertype<juce::Component> ("Component", sol::no_constructor,
        "new", sol::constructors<Component(), Component(std::string)>(),
        "getName",          [](Component& self)  { return self.getName().toStdString(); },
        "setName",          [](Component& self, const char* name) { self.setName (name); },
        "getComponentID",   [](Component& self) { return self.getComponentID().toStdString(); },
        "setComponentID",   [](Component& self, const char* cid) { self.setComponentID (cid); },
        "setVisible",                       &Component::setVisible,
        "isVisible",                        &Component::isVisible,
        "visibilityChanged",                &Component::visibilityChanged,
        "isShowing",                        &Component::isShowing,
        "addToDesktop", sol::overload (
            [](Component& self, int flags, void* native) { self.addToDesktop (flags, native); },
            [](Component& self, int flags) { self.addToDesktop (flags, nullptr); }
        ),
        "removeFromDesktop",                &Component::removeFromDesktop,
        "isOnDesktop",                      &Component::isOnDesktop,
        "getPeer",                          &Component::getPeer,
        "userTriedToCloseWindow",           &Component::userTriedToCloseWindow,
        "minimisationStateChanged",         &Component::minimisationStateChanged,
        "getDesktopScaleFactor",            &Component::getDesktopScaleFactor,
        "toFront",                          &Component::toFront,
        "toBack",                           &Component::toBack,
        "toBehind",                         &Component::toBehind,
        "setAlwaysOnTop",                   &Component::setAlwaysOnTop,
        "isAlwaysOnTop",                    &Component::isAlwaysOnTop,
        "getX",                             &Component::getX,
        "getY",                             &Component::getY,
        "getWidth",                         &Component::getWidth,
        "getHeight",                        &Component::getHeight,
        "getRight",                         &Component::getRight,
        "getPosition",                      &Component::getPosition,
        "getBottom",                        &Component::getBottom,
        "getBounds",                        &Component::getBounds,
        "getLocalBounds",                   &Component::getLocalBounds,
        "getBoundsInParent",                &Component::getBoundsInParent,
        "getScreenX",                       &Component::getScreenX,
        "getScreenY",                       &Component::getScreenY,
        "getScreenPosition",                &Component::getScreenPosition,
        "getScreenBounds",                  &Component::getScreenBounds,
        "getLocalPoint", sol::overload (
            [](Component& self, Component* source, Point<int> relPoint) {
                return self.getLocalPoint (source, relPoint);
            },
            [](Component& self, Component* source, Point<float> relPoint) {
                return self.getLocalPoint (source, relPoint);
            }
        ),
        "getLocalArea", sol::overload (
            [](Component& self, Component* source, Rectangle<int> rect) {
                return self.getLocalArea (source, rect);
            },
            [](Component& self, Component* source, Rectangle<float> rect) {
                return self.getLocalArea (source, rect);
            }
        ),
        "localPointToGlobal", sol::overload (
            [](Component& self, Point<int> pt)   { return self.localPointToGlobal (pt); },
            [](Component& self, Point<float> pt) { return self.localPointToGlobal (pt); }
        ),
        "localAreaToGlobal", sol::overload (
            [](Component& self, Rectangle<int> rect)   { return self.localAreaToGlobal (rect); },
            [](Component& self, Rectangle<float> rect) { return self.localAreaToGlobal (rect); }
        ),
        "setTopLeftPosition", sol::overload (
            [](Component& self, int x, int y)  { self.setTopLeftPosition (x, y); },
            [](Component& self, Point<int> pt) { self.setTopLeftPosition (pt); }
        ),
        "setTopRightPosition", sol::overload (
            [](Component& self, int x, int y)  { self.setTopRightPosition (x, y); }
        ),
        
        "setSize",                          &Component::setSize,
        "setBounds", sol::overload (
            [](Component& self, int x, int y, int w, int h) { self.setBounds (x, y, w, h); },
            [](Component& self, Rectangle<int> rect) { self.setBounds (rect); }
        ),
        "setBoundsRelative", sol::overload (
            [](Component& self, float x, float y, float w, float h) { self.setBoundsRelative (x, y, w, h); },
            [](Component& self, Rectangle<float> rect) { self.setBoundsRelative (rect); }
        ),
        "setBoundsInset",                   &Component::setBoundsInset,
        "setBoundsToFit",                   &Component::setBoundsToFit,
        "setCentrePosition", sol::overload (
            [](Component& self, int x, int y)  { self.setCentrePosition (x, y); },
            [](Component& self, Point<int> pt) { self.setCentrePosition (pt); }
        ),
        "setCentreRelative",                &Component::setCentreRelative,
        "centreWithSize",                   &Component::centreWithSize,
        "setTransform",                     &Component::setTransform,
        "getTransform",                     &Component::getTransform,
        "isTransformed",                    &Component::isTransformed,
        "getApproximateScaleFactorForComponent", Component::getApproximateScaleFactorForComponent,
        "proportionOfWidth",                &Component::proportionOfWidth,
        "proportionOfHeight",               &Component::proportionOfHeight,
        "getParentWidth",                   &Component::getParentWidth,
        "getParentHeight",                  &Component::getParentHeight,
        "getParentMonitorArea",             &Component::getParentMonitorArea,

        "getNumChildComponents",            &Component::getNumChildComponents,
        "getChildComponent",                &Component::getChildComponent,
        "getIndexOfChildComponent",         &Component::getIndexOfChildComponent,
        "getChildren",                      &Component::getChildren,
        "findChildWithID",                  &Component::findChildWithID,
        "addChildComponent", sol::overload (
            [](Component& self, Component& comp, int zorder) { self.addChildComponent (comp, zorder); },
            [](Component& self, Component& comp) { self.addChildComponent (comp, -1); }
        ),
        "addAndMakeVisible", sol::overload (
            [](Component& self, Component& comp, int zorder) { self.addAndMakeVisible (comp, zorder); },
            [](Component& self, Component& comp) { self.addAndMakeVisible (comp, -1); }
        ),
        "addChildAndSetID",                 &Component::addChildAndSetID,
        "removeChildComponent", sol::overload (
            [](Component& self, Component* comp) { self.removeChildComponent (comp); },
            [](Component& self, int index) { self.removeChildComponent (index); }
        ),
        "removeAllChildren",                &Component::removeAllChildren,
        "deleteAllChildren",                &Component::deleteAllChildren,
        "getParentComponent",               &Component::getParentComponent,
        "getTopLevelComponent",             &Component::getTopLevelComponent,
        "isParentOf",                       &Component::isParentOf,
        "parentHierarchyChanged",           &Component::parentHierarchyChanged,
        "childrenChanged",                  &Component::childrenChanged,

        "hitTest",                          &Component::hitTest,
        "setInterceptsMouseClicks",         &Component::setInterceptsMouseClicks,
        "getInterceptsMouseClicks", [](Component& self) {
            bool a, b; self.getInterceptsMouseClicks (a, b);
            return std::make_tuple (a, b);
        },
        "contains",  [](Component& self, Point<int> pt) { return self.contains (pt); },
        "reallyContains",           &Component::reallyContains,
        "getComponentAt", sol::overload (
            [](Component& self, int x, int y) { return self.getComponentAt (x, y); },
            [](Component& self, Point<int> pt) { return self.getComponentAt (pt); }
        ),

        "repaint", sol::overload (
            [](Component& self) { self.repaint(); },
            [](Component& self, int x, int y, int w, int h) { return self.repaint (x, y, w, h); },
            [](Component& self, Rectangle<int> area) { return self.repaint (area); }
        ),
        "setBufferedToImage",       &Component::setBufferedToImage,
        "createComponentSnapshot", sol::overload (
            [](Component& self, Rectangle<int> area) {
                return self.createComponentSnapshot (area);
            },
            [](Component& self, Rectangle<int> area, bool clip) {
                return self.createComponentSnapshot (area, clip);
            },
            [](Component& self, Rectangle<int> area, bool clip, float scale) {
                return self.createComponentSnapshot (area, clip, scale);
            }
        ),

        "paintEntireComponent",             &Component::paintEntireComponent,
        "setPaintingIsUnclipped",           &Component::setPaintingIsUnclipped,
        "isPaintingUnclipped",              &Component::isPaintingUnclipped,
        "setComponentEffect",               &Component::setComponentEffect,
        "getComponentEffect",               &Component::getComponentEffect,
        "getLookAndFeel",                   &Component::getLookAndFeel,
        "setLookAndFeel",                   &Component::setLookAndFeel,
        "lookAndFeelChanged",               &Component::lookAndFeelChanged,
        "sendLookAndFeelChange",            &Component::sendLookAndFeelChange,
        "setOpaque",                        &Component::setOpaque,
        "isOpaque",                         &Component::isOpaque,
        "setBroughtToFrontOnMouseClick",    &Component::setBroughtToFrontOnMouseClick,
        "isBroughtToFrontOnMouseClick",     &Component::isBroughtToFrontOnMouseClick,
        "setWantsKeyboardFocus",            &Component::setWantsKeyboardFocus,
        "getWantsKeyboardFocus",            &Component::getWantsKeyboardFocus,
        "setMouseClickGrabsKeyboardFocus",  &Component::setMouseClickGrabsKeyboardFocus,
        "getMouseClickGrabsKeyboardFocus",  &Component::getMouseClickGrabsKeyboardFocus,
        "grabKeyboardFocus",                &Component::grabKeyboardFocus,
        "hasKeyboardFocus",                 &Component::hasKeyboardFocus,
        "getCurrentlyFocusedComponent",     Component::getCurrentlyFocusedComponent,
        "unfocusAllComponents",             Component::unfocusAllComponents,
        "moveKeyboardFocusToSibling",       &Component::moveKeyboardFocusToSibling,
        "createFocusTraverser",             &Component::createFocusTraverser,
        "getExplicitFocusOrder",            &Component::getExplicitFocusOrder,
        "setExplicitFocusOrder",            &Component::setExplicitFocusOrder,
        "setFocusContainer",                &Component::setFocusContainer,
        "isFocusContainer",                 &Component::isFocusContainer,
        "isEnabled",                        &Component::isEnabled,
        "setEnabled",                       &Component::setEnabled,
        "enablementChanged",                &Component::enablementChanged,
        "getAlpha",                         &Component::getAlpha,
        "setAlpha",                         &Component::setAlpha,
        "alphaChanged",                     &Component::alphaChanged,
        "setMouseCursor",                   &Component::setMouseCursor,
        "getMouseCursor",                   &Component::getMouseCursor,
        "updateMouseCursor",                &Component::updateMouseCursor,
        "paint",                            &Component::paint,
        "paintOverChildren",                &Component::paintOverChildren,
        "mouseMove",                        &Component::mouseMove,
        "mouseEnter",                       &Component::mouseEnter,
        "mouseExit",                        &Component::mouseExit,
        "mouseDown",                        &Component::mouseDown,
        "mouseDrag",                        &Component::mouseDrag,
        "mouseUp",                          &Component::mouseUp,
        "mouseDoubleClick",                 &Component::mouseDoubleClick,
        "mouseWheelMove",                   &Component::mouseWheelMove,
        "mouseMagnify",                     &Component::mouseMagnify,
        "beginDragAutoRepeat",              Component::beginDragAutoRepeat,
        "setRepaintsOnMouseActivity",       &Component::setRepaintsOnMouseActivity,
        "addMouseListener",                 &Component::addMouseListener,
        "removeMouseListener",              &Component::removeMouseListener,
        "addKeyListener",                   &Component::addKeyListener,
        "removeKeyListener",                &Component::removeKeyListener,
        "keyPressed",                       &Component::keyPressed,
        "keyStateChanged",                  &Component::keyStateChanged,
        "modifierKeysChanged",              &Component::modifierKeysChanged,
        "focusGained",                      &Component::focusGained,
        "focusLost",                        &Component::focusLost,
        "focusOfChildComponentChanged",     &Component::focusOfChildComponentChanged,
        "isMouseOver", sol::overload (
            [](Component& self) { return self.isMouseOver(); },
            [](Component& self, bool includeChildren) { return self.isMouseOver (includeChildren); }
        ),
        "isMouseButtonDown", sol::overload (
            [](Component& self) { return self.isMouseButtonDown(); },
            [](Component& self, bool includeChildren) { return self.isMouseButtonDown (includeChildren); }
        ),
        "isMouseOverOrDragging", sol::overload (
            [](Component& self) { return self.isMouseOverOrDragging(); },
            [](Component& self, bool includeChildren) { return self.isMouseOverOrDragging (includeChildren); }
        ),
        "isMouseButtonDownAnywhere",        Component::isMouseButtonDownAnywhere,
        "getMouseXYRelative",               &Component::getMouseXYRelative,
        "resized",                          &Component::resized,
        "moved",                            &Component::moved,
        "childBoundsChanged",               &Component::childBoundsChanged,
        "parentSizeChanged",                &Component::parentSizeChanged,
        "broughtToFront",                   &Component::broughtToFront,
        "addComponentListener",             &Component::addComponentListener,
        "removeComponentListener",          &Component::removeComponentListener,
        "postCommandMessage",               &Component::postCommandMessage,
        "handleCommandMessage",             &Component::handleCommandMessage,
       #if JUCE_MODAL_LOOPS_PERMITTED
        "runModalLoop", &Component::runModalLoop,
       #endif
        "enterModalState", sol::overload (
            [](Component& self) { self.enterModalState(); },
            [](Component& self, bool focus) {
                self.enterModalState (focus);
            },
            [](Component& self, bool focus, ModalComponentManager::Callback* cb) {
                self.enterModalState (focus, cb);
            },
            [](Component& self, bool focus, ModalComponentManager::Callback* cb, bool del) { 
                self.enterModalState (focus, cb, del);
            }
        ),
        "exitModalState",                   &Component::exitModalState,
        "isCurrentlyModal", sol::overload (
            [](Component& self) { return self.isCurrentlyModal(); },
            [](Component& self, bool foremost) { return self.isCurrentlyModal (foremost); }
        ),
        "getNumCurrentlyModalComponents",   Component::getNumCurrentlyModalComponents,
        "getCurrentlyModalComponent", sol::overload (
            []() { return Component::getCurrentlyModalComponent(); },
            [](int index) { return Component::getCurrentlyModalComponent (index); }
        ),
        "isCurrentlyBlockedByAnotherModalComponent", &Component::isCurrentlyBlockedByAnotherModalComponent,
        "canModalEventBeSentToComponent",   &Component::canModalEventBeSentToComponent,
        "inputAttemptWhenModal",            &Component::inputAttemptWhenModal,
        "getProperties", [](Component& self) -> NamedValueSet& { return self.getProperties(); },
        "findColour", sol::overload (
            [](Component& self, int cid) -> Colour { return self.findColour (cid); },
            [](Component& self, int cid, bool inherit) -> Colour { return self.findColour (cid, inherit); }
        ),
        "setColour",                        &Component::setColour,
        "removeColour",                     &Component::removeColour,
        "isColourSpecified",                &Component::isColourSpecified,
        "copyAllExplicitColoursTo",         &Component::copyAllExplicitColoursTo,
        "colourChanged",                    &Component::colourChanged,
        "getWindowHandle",                  &Component::getWindowHandle,
        "getPositioner",                    &Component::getPositioner,
        "setPositioner",                    &Component::setPositioner,
        "setCachedComponentImage",          &Component::setCachedComponentImage,
        "getCachedComponentImage",          &Component::getCachedComponentImage,
        "setViewportIgnoreDragFlag",        &Component::setViewportIgnoreDragFlag,
        "getViewportIgnoreDragFlag",        &Component::getViewportIgnoreDragFlag
    );

    // comp.new_enum ("FocusChangeType",
    //     "focusChangedByMouseClick",     Component::focusChangedByMouseClick,
    //     "focusChangedByTabKey",         Component::focusChangedByTabKey,
    //     "focusChangedDirectly",         Component::focusChangedDirectly
    // );

    // comp.new_usertype<juce::Component::Positioner> ("Positioner", sol::no_constructor,
    //     "getComponent",                 &Component::Positioner::getComponent,
    //     "applyNewBounds",               &Component::Positioner::applyNewBounds
    // );

    M.new_usertype<DocumentWindow> ("DocumentWindow", sol::no_constructor,
        "new", sol::factories (
            [](std::string name, int background, int buttons, bool desktop) -> DocumentWindow* {
                return new DocumentWindow (name, Colour (background), buttons, desktop);
            },
            [](std::string name, Colour background, int buttons, bool desktop) -> DocumentWindow* {
                return new DocumentWindow (name, background, buttons, desktop);
            }
        ),
        "setSize",               &DocumentWindow::setSize,
        "centreWithSize",        &DocumentWindow::centreWithSize,
        "setVisible",            &DocumentWindow::setVisible,
        "clearContentComponent", &DocumentWindow::clearContentComponent,
        "setContentOwned",       &DocumentWindow::setContentOwned,
        "setContentNonOwned",    &DocumentWindow::setContentNonOwned,
        "addToDesktop",          sol::overload (
            [](DocumentWindow* self) { self->addToDesktop(); }
        ),
        sol::base_classes, sol::bases<juce::Component, juce::MouseListener>()
    );

    // AudioSampleBuffer
    M.new_usertype<AudioSampleBuffer> ("AudioSampleBuffer",
        sol::no_constructor,
        "hasBeenCleared",       &AudioSampleBuffer::hasBeenCleared,
        "getNumChannels",       &AudioSampleBuffer::getNumChannels,
        "getNumSamples",        &AudioSampleBuffer::getNumSamples,
        "setSize", sol::overload (
            [](AudioSampleBuffer& self, int nc, int ns) { self.setSize (nc, ns); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep) { self.setSize (nc, ns, keep); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep, bool clear) { self.setSize (nc, ns, keep, clear); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep, bool clear, bool avoid) { self.setSize (nc, ns, keep, clear, avoid); }),
        "makeCopyOf", sol::overload (
            [](AudioSampleBuffer& self, const AudioSampleBuffer& other) { self.makeCopyOf (other); },
            [](AudioSampleBuffer& self, const AudioSampleBuffer& other, bool avoidReallocate) { self.makeCopyOf (other, avoidReallocate); }),
        "clear", sol::overload (
            sol::resolve<void()> (&AudioSampleBuffer::clear),
            sol::resolve<void(int,int)> (&AudioSampleBuffer::clear),
            sol::resolve<void(int,int,int)> (&AudioSampleBuffer::clear)),
        "getSample",        &AudioSampleBuffer::getSample,
        "setSample",        &AudioSampleBuffer::setSample,
        "addSample",        &AudioSampleBuffer::addSample,
        "applyGain", sol::overload (
            sol::resolve<void(int,int,int,float)> (&AudioSampleBuffer::applyGain),
            sol::resolve<void(int,int,float)> (&AudioSampleBuffer::applyGain),
            sol::resolve<void(float)> (&AudioSampleBuffer::applyGain)),
        "applyGainRamp", sol::overload (
            sol::resolve<void(int,int,int,float,float)> (&AudioSampleBuffer::applyGainRamp),
            sol::resolve<void(int,int,float,float)> (&AudioSampleBuffer::applyGainRamp)),
        "addFrom", sol::overload (
            [](AudioSampleBuffer& self, int dc, int dss, AudioSampleBuffer& src, int sc, int sss, int ns) {
                self.addFrom (dc, dss, src, sc, sss, ns);
            },
            [](AudioSampleBuffer& self, int dc, int dss, AudioSampleBuffer& src, int sc, int sss, int ns, float gain) {
                self.addFrom (dc, dss, src, sc, sss, ns, gain);
            }),
        "addFromWithRamp",   &AudioSampleBuffer::addFromWithRamp,
        "copyFrom", sol::overload (
            sol::resolve<void(int,int,const AudioSampleBuffer&,int,int,int)> (&AudioSampleBuffer::copyFrom),
            sol::resolve<void(int,int,const float*, int)> (&AudioSampleBuffer::copyFrom),
            sol::resolve<void(int,int,const float*, int, float)> (&AudioSampleBuffer::copyFrom)),
        "copyFromWithRamp",  &AudioSampleBuffer::copyFromWithRamp,
        "findMinMax",        &AudioSampleBuffer::findMinMax,
        "getMagnitude", sol::overload (
            [](const AudioSampleBuffer& self, int c, int s, int n) { return self.getMagnitude (c, s, n); },
            [](const AudioSampleBuffer& self, int s, int n) { return self.getMagnitude (s, n); }),
        "getRMSLevel", &AudioSampleBuffer::getRMSLevel,
        "reverse", sol::overload (
            [](const AudioSampleBuffer& self, int c, int s, int n) { return self.reverse (c, s, n); },
            [](const AudioSampleBuffer& self, int s, int n) { return self.reverse (s, n); })
    );

    lua["juce"] = M;
}

}}
