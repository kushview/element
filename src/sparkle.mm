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
            _updaterDelegate = [[AppUpdaterDelegate alloc] init];
            _updaterDelegate.updaterController = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES
                                                                                               updaterDelegate:_updaterDelegate
                                                                                            userDriverDelegate:nil];

            // connect(checkForUpdatesAction, &QAction::triggered, this, &Updater::checkForUpdates);

            [_updaterDelegate observeCanCheckForUpdatesWithAction:dynamic_cast<Updater*>(this)];
        }
    }

    void check(bool async) override
    {
        juce::ignoreUnused(async);
        @autoreleasepool
        {
            [_updaterDelegate.updaterController checkForUpdates:nil];
        }
    }

   private:
    AppUpdaterDelegate* _updaterDelegate{nullptr};
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
        _updaterDelegate = [[AppUpdaterDelegate alloc] init];
        _updaterDelegate.updaterController = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES updaterDelegate:_updaterDelegate userDriverDelegate:nil];
        
        connect(checkForUpdatesAction, &QAction::triggered, this, &Updater::checkForUpdates);
        
        [_updaterDelegate observeCanCheckForUpdatesWithAction:checkForUpdatesAction];
    }
}

// Called when the user checks for updates
void Updater::checkForUpdates()
{
    @autoreleasepool {
        [_updaterDelegate.updaterController checkForUpdates:nil];
    }
}
#endif
