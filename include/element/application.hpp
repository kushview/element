// Copyright 2025 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/events.hpp>
#include <element/juce/gui_basics.hpp>
#include <element/ui/content.hpp>

namespace element {

class Context;
class Startup;

/** The main application class for Element.
    
    This class manages the application lifecycle, including initialization,
    shutdown, session handling, and audio device management. It inherits from
    JUCEApplication to integrate with the JUCE framework's application model.
*/
class Application : public juce::JUCEApplication,
                    public juce::ActionListener,
                    private juce::Timer {
public:
    /** Constructor. */
    Application();

    /** Destructor. */
    virtual ~Application();

    /** Returns the name of the application. 
        @return The app name
    */
    const juce::String getApplicationName() override;

    /** Returns the current version string of the application.
        @return The version string
    */
    const juce::String getApplicationVersion() override;

    /** Indicates whether multiple instances of the application are allowed.
        @return true to allow multiple instances
    */
    bool moreThanOneInstanceAllowed() override;

    /** Called when the application starts.
        
        This initializes the application context, sets up the module path,
        and launches the main application window and services.
        
        @param commandLine The command line arguments passed to the application
    */
    void initialise (const juce::String& commandLine) override;

    /** Receives action messages from ActionBroadcasters.
        
        Currently handles the "finishedLaunching" message to complete
        application startup after initialization.
        
        @param message The action message string
    */
    void actionListenerCallback (const juce::String& message) override;

    /** Called when the application is shutting down.
        
        Performs cleanup including saving settings, closing audio devices,
        shutting down services, and releasing resources.
    */
    void shutdown() override;

    /** Called when the user requests the application to quit.
        
        Prompts the user to save the current session if needed before
        allowing the application to exit.
    */
    void systemRequestedQuit() override;

    /** Attempts to open a session or graph file from a command line path.
        
        Supports .els (Element session) and .elg (Element graph) file formats.
        
        @param commandLine The file path to attempt to open
    */
    void maybeOpenCommandLineFile (const juce::String& commandLine);

    /** Called when another instance of the application is started.
        
        Attempts to open any file specified in the command line of the new instance.
        
        @param commandLine The command line from the new instance
    */
    void anotherInstanceStarted (const juce::String& commandLine) override;

    /** Called when the application is suspended (mobile platforms). */
    void suspended() override;

    /** Called when the application resumes from suspension.
        
        Restarts the last used audio device.
    */
    void resumed() override;

    /** Completes the application launch process.
        
        Scans for plugins if needed, starts services, and enables update checking.
    */
    void finishLaunching();

protected:
    Context& context() { return *world; }
    virtual std::unique_ptr<ContentFactory> createContentFactory() { return nullptr; }

private:
    juce::String launchCommandLine;                     ///< The command line used to launch the app
    std::unique_ptr<Context> world;                     ///< The application context and services
    std::unique_ptr<Startup> startup;                   ///< Handles startup initialization
    juce::OwnedArray<juce::ChildProcessWorker> workers; ///< Worker processes (e.g., plugin scanner)

    /** Prints the copyright notice to the log. */
    void printCopyNotice();

    /** Attempts to launch a plugin scanner worker process.
        
        @param commandLine The command line to check for scanner process ID
        @return true if a scanner worker was launched
    */
    bool maybeLaunchScannerWorker (const juce::String& commandLine);

    /** Initiates the application launch sequence. */
    void launchApplication();

    /** Sets up the ELEMENT_MODULE_PATH environment variable. */
    void initializeModulePath();

    /** Timer callback to check for updates after launch. */
    void timerCallback() override;
};

} // namespace element
