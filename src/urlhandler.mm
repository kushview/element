// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>

#include <element/application.hpp>
#include <element/juce/core.hpp>

using namespace juce;

/** URL event handler that doesn't interfere with JUCE's delegate */
@interface ElementURLHandler : NSObject
@property (nonatomic, assign) element::Application* app;
- (void)handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent;
@end

@implementation ElementURLHandler

- (void)handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    NSString* urlString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];

    if (urlString && _app)
    {
        String juceURL = String::fromUTF8([urlString UTF8String]);
        auto* appPtr = _app;

        MessageManager::callAsync([appPtr, juceURL]()
                                  {
            if (appPtr)
                appPtr->handleURLSchemeCallback(juceURL); });
    }
}

@end

namespace element
{

static ElementURLHandler* urlHandler = nil;

void Application::registerURLSchemeHandler()
{
    if (urlHandler == nil)
    {
        urlHandler = [[ElementURLHandler alloc] init];
        urlHandler.app = this;

        // Register with Apple Event Manager (doesn't interfere with JUCE's delegate)
        NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
        [appleEventManager setEventHandler:urlHandler
                               andSelector:@selector(handleURLEvent:withReplyEvent:)
                             forEventClass:kInternetEventClass
                                andEventID:kAEGetURL];

        Logger::writeToLog("URL scheme handler (Apple Event Manager) registered");
    }
}

void Application::unregisterURLSchemeHandler()
{
    if (urlHandler != nil)
    {
        NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
        [appleEventManager removeEventHandlerForEventClass:kInternetEventClass
                                                andEventID:kAEGetURL];

        [urlHandler release];
        urlHandler = nil;
    }
}

} // namespace element
