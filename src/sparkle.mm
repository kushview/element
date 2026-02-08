// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>

#include <memory>

#include <element/juce/core.hpp>
#include <element/ui/updater.hpp>

@interface AppUpdaterDelegate : NSObject <SPUUpdaterDelegate>

@property (assign, nonatomic) SPUStandardUpdaterController* updaterController;
@property (strong, nonatomic) NSURL* feedURL;
@property BOOL isBackgroundCheck;

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
        // TODO: implement by enabling/disabling menu?
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _isBackgroundCheck = YES;
    }
    return self;
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

- (void)updater:(SPUUpdater*)updater didFindValidUpdate:(id)item
{
    juce::Logger::writeToLog("[element] sparkle: update available");
    if (_isBackgroundCheck)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
          [_updaterController checkForUpdates:nil];
        });
    }
}

@end
// clang-format off
namespace element {

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

            auto controller = _delegate.updaterController;
            [_delegate observeCanCheckForUpdatesWithAction:dynamic_cast<Updater*>(this)];
        }
    }

    void check(bool background) override
    {
        @autoreleasepool
        {
            if (_delegate.updaterController.updater.sessionInProgress)
                return;
            _delegate.isBackgroundCheck = background;
            if (background)
                [_delegate.updaterController.updater checkForUpdatesInBackground];
            else
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
// clang-format on
