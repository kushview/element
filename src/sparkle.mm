// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>

#include <memory>

#include <element/juce/core.hpp>
#include <element/ui/updater.hpp>

// An updater delegate class that mainly takes care of updating the check for updates menu item's state
// This class can also be used to implement other updater delegate methods
@interface AppUpdaterDelegate : NSObject <SPUUpdaterDelegate>

@property (assign, nonatomic) SPUStandardUpdaterController* updaterController;
@property (strong, nonatomic) NSURL* feedURL;

@end

@implementation AppUpdaterDelegate

- (void)observeCanCheckForUpdatesWithAction:(void*)ctx
{
    [_updaterController.updater addObserver:self
                                 forKeyPath:NSStringFromSelector(@selector(canCheckForUpdates))
                                    options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew)
                                    context:(void*)ctx];
}

- (void)observeValueForKeyPath:(NSString*)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey, id>*)change context:(void*)context
{
    if ([keyPath isEqualToString:NSStringFromSelector(@selector(canCheckForUpdates))])
    {
        // TODO: implement
        // QAction *menuAction = (QAction *)context;
        // menuAction->setEnabled(_updaterController.updater.canCheckForUpdates);
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)dealloc
{
    [super dealloc];
    @autoreleasepool
    {
        [_updaterController.updater removeObserver:self
                                        forKeyPath:NSStringFromSelector(@selector(canCheckForUpdates))];
    }
}

- (nullable NSString*)feedURLStringForUpdater:(SPUUpdater*)updater
{
    return [_feedURL absoluteString];
}

@end

namespace element
{
class SparkleUpdater : public Updater
{
   public:
    SparkleUpdater()
    {
        @autoreleasepool
        {
            _delegate = [[AppUpdaterDelegate alloc] init];
            _delegate.updaterController = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES
                                                                                        updaterDelegate:_delegate
                                                                                     userDriverDelegate:nil];

            // connect(checkForUpdatesAction, &QAction::triggered, this, &Updater::checkForUpdates);

            [_delegate observeCanCheckForUpdatesWithAction:dynamic_cast<Updater*>(this)];
        }
    }

    void check(bool async) override
    {
        juce::ignoreUnused(async);
        @autoreleasepool
        {
            [_delegate.updaterController checkForUpdates:nil];
        }
    }

    void setFeedUrl(const std::string& url) override
    {
        @autoreleasepool
        {
            if (url.empty())
            {
                _delegate.feedURL = nil;
            }
            else
            {
                NSString* nsString = [NSString stringWithUTF8String:url.c_str()];
                _delegate.feedURL = [NSURL URLWithString:nsString];
            }
        }
    }

   private:
    AppUpdaterDelegate* _delegate{nullptr};
};

std::unique_ptr<Updater> Updater::create()
{
    return std::make_unique<SparkleUpdater>();
}

} // namespace element

#if 0
// Creates and starts the updater. There's nothing else required.
Updater::Updater(QAction *checkForUpdatesAction)
{
    @autoreleasepool {
        _delegate = [[AppUpdaterDelegate alloc] init];
        _delegate.updaterController = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES updaterDelegate:_delegate userDriverDelegate:nil];
        
        connect(checkForUpdatesAction, &QAction::triggered, this, &Updater::checkForUpdates);
        
        [_delegate observeCanCheckForUpdatesWithAction:checkForUpdatesAction];
    }
}

// Called when the user checks for updates
void Updater::checkForUpdates()
{
    @autoreleasepool {
        [_delegate.updaterController checkForUpdates:nil];
    }
}
#endif
