/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/AppConfig.h"
#include "juce_StandaloneFilterWindow.h"

//==============================================================================
class gstepstandaloneApplication  : public JUCEApplication, public Timer
{
public:
    //==============================================================================
    gstepstandaloneApplication() {}

    const String getApplicationName() {
        return ProjectInfo::projectName;
    }
    const String getApplicationVersion() {
        return ProjectInfo::versionString;
    }
    bool moreThanOneInstanceAllowed() {
        return true;
    }

    bool is_first_call_back;
    //==============================================================================
    void initialise (const String&)
    {
        is_first_call_back = false;

#ifndef IS_MOBILE_APP
        startTimer(5);
#endif
        String DEVEL_VERSION = getApplicationName() + String(" ") + getApplicationVersion();
#if JUCE_LINUX || JUCE_WINDOWS
        standaloneFilterWindow = new StandaloneFilterWindow(DEVEL_VERSION);
#else
        standaloneFilterWindow = new StandaloneFilterWindow(DEVEL_VERSION);
#endif
#ifndef IS_MOBILE_APP
#if JUCE_LINUX || JUCE_WINDOWS
        standaloneFilterWindow->setUsingNativeTitleBar( false );
#else
        // HERE ENABLE NATIVE MAC WINDOWS
        standaloneFilterWindow->setUsingNativeTitleBar( false );
#endif
        standaloneFilterWindow->setDropShadowEnabled( true );
#endif
#if JUCE_IOS || JUCE_ANDROID
        standaloneFilterWindow->setResizable (true, false);
#	if ! JUCE_IOS
        standaloneFilterWindow->setDropShadowEnabled( false );
#	endif
#else
        // HERE ENABLE NATIVE MAC WINDOWS
//#	ifdef JUCE_MAC
//        standaloneFilterWindow->setTitleBarButtonsRequired (DocumentWindow::minimiseButton | DocumentWindow::closeButton | DocumentWindow::maximiseButton, true );
//#	else
        standaloneFilterWindow->setTitleBarButtonsRequired (DocumentWindow::minimiseButton | DocumentWindow::closeButton | DocumentWindow::maximiseButton, false );
//#	endif
        standaloneFilterWindow->setResizable(true,true);
        //standaloneFilterWindow->ResizableWindow::resizableCorner->setAlwaysOnTop(true);
#endif
    }

    void timerCallback()
    {
        if( !is_first_call_back )
        {
            is_first_call_back = true;
            startTimer(5); // 750
        }
        else
        {
            stopTimer();
            standaloneFilterWindow->addToDesktop();
            standaloneFilterWindow->setVisible (true);
            //standaloneFilterWindow->toFront(true);
            //standaloneFilterWindow->editor->startTimer(1000.0f/30);

            standaloneFilterWindow->setEnabled(true);

            DBG( "SHOW MAINWINDOW" );
        }
    }

    void shutdown()
    {
        // Add your application's shutdown code here..
        DBG( "USR QUIT") ;

        standaloneFilterWindow = nullptr; // (deletes our window)
    }

//==============================================================================
    void suspended() override
    {
#ifdef JUCE_ANDROID
        standaloneFilterWindow->store->save_settings();
#endif
        //standaloneFilterWindow->suspended();
    }
    void resumed() override
    {
#ifdef JUCE_ANDROID
        // TODO is this required?
        standaloneFilterWindow->store->save_settings();
#endif
        //standaloneFilterWindow->resumed();
    }

    void systemRequestedQuit()
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        DBG( "SYS QUIT") ;

        quit();
    }

    void anotherInstanceStarted (const String&)
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

private:
    ScopedPointer<StandaloneFilterWindow> standaloneFilterWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (gstepstandaloneApplication)
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (gstepstandaloneApplication)








