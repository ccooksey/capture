/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#if TARGET_RT_MAC_MACHO
#ifndef __CARBON__
#include <Carbon/Carbon.h>
#endif
#else
#include <MacTypes.h>
#include <Appearance.h>
#endif

#include <LGrowZone.h>
#include <PP_Messages.h>
#include <PP_Resources.h>
#include <UDrawingState.h>
#include <UMemoryMgr.h>
#include <URegistrar.h>
#include <UEnvironment.h>

#include <LWindow.h>
#include <LDialogBox.h>
#include <LIconControl.h>
#include <LPopupButton.h>
#include <LPushButton.h>
#include <LRadioButton.h>
#include <LRadioGroupView.h>
#include <LBevelButton.h>
#include <LCheckBox.h>
#include <LStaticText.h>
#include <LTextGroupBox.h>
#include <LTabGroup.h>
#include <LCheckBoxGroupBox.h>
#include <LEditText.h>

#include <LAMControlImp.h>
#include <LAMPopupButtonImp.h>
#include <LAMPushButtonImp.h>
#include <LAMBevelButtonImp.h>
#include <LAMStaticTextImp.h>
#include <LAMTextGroupBoxImp.h>
#include <LAMCheckBoxGroupBoxImp.h>
#include <LAMEditTextImp.h>

#include "CCaptureApplication.h"
#include "CCaptureSnapshotSettings.h"
#include "CCaptureSnapshotSetup.h"
#include "CCaptureRecordSetup.h"
#include "CUAboutBoxHandler.h"
#include "CUIconView.h"
#include "CUPreferences.h"
#include "CURLCaption.h"
#include "CUtilities.h"
#include "CUtilities2.h"
//#include "CCaptureSynchronize.h"
#include "CaptureConstants.h"
#include "GetPID.h"

#include "NTZRelease.h"
#include "NTZLogging.h"

/*--------------------------------------------------------------------------------------*/
static Boolean fPStrEqual( Str255 s1, Str255 s2 );
static Boolean fOnlyCopy( void );

/*--------------------------------------------------------------------------------------*/
static Str255 gManufacturerName;
static Str255 gLicenseeShortName;
static Str255 gProductName;
static Str255 gPreferenceFolderName;
static Str255 gPreferenceName;
#if kNTZPackageNumber == '3Cam'
SInt32 g3CamRunMode = k3CamRunModeNormal;
#endif

/*======================================================================================*/
int main( void ) {

	SetDebugThrow_( debugAction_Alert );
	SetDebugSignal_( debugAction_Alert );

	InitializeHeap( 3 );
	#if TARGET_API_MAC_CARBON
	UQDGlobals::InitializeToolbox();
	#else
	UQDGlobals::InitializeToolbox( &qd );
	#endif
	new LGrowZone( 20000 );

	::GetIndString( gManufacturerName, kStrings, kStringsManufacturer );
	::GetIndString( gLicenseeShortName, kStrings, kStringsLicenseeShort );
	::GetIndString( gProductName, kStrings, kStringsProduct );
	CUtilities::fPStrCopy( gPreferenceFolderName, gLicenseeShortName );
	#if kNTZPackageNumber == 'mgvr'
	CUtilities::fPStrConcat( gPreferenceFolderName, "\p RYVT" );
	#endif
	CUtilities::fPStrCopy( gPreferenceName, gProductName );

	Boolean run = true;
	UEnvironment::InitEnvironment();
	UQuickTime::Initialize();

	#if kNTZPackageNumber == '3Cam'
	/* Make sure we are the only copy running. This is a Logical Choice requirement */
	if( run )
		if( !fOnlyCopy() ) {
			::ParamText( "\p3Cam", "\p", "\p", "\p" );
			::StopAlert( ALRT_ApplicationAlreadyRunning, NULL );
			run = false;
			}
	#endif

	/* Make sure QuickTime is present. Completely stupid check */
	if( run && !UQuickTime::QuickTimeIsPresent() ) {
		::ParamText( gProductName, "\p", "\p", "\p" );
		::StopAlert( ALRT_NoQuickTime, NULL );
		run = false;
		}

	/* Create the application object and run */
	if( run ) {
		CCaptureApp	theApp;
		theApp.Run();
		}

	return( 0 );
	}

/*======================================================================================*/
static Boolean fOnlyCopy( void ) {
/* Return true if this is the only copy running.										*/

	Boolean onlyCopy = true;

	if( UEnvironment::IsRunningOSX() ) {
		unsigned int allCount = 0;
		unsigned int count = 0;
		pid_t pids[10];
		if( ::GetAllPIDsForProcessName( "3Cam Control Panel Mode", pids, 10, &count, NULL ) == noErr )
			allCount += count;
		if( ::GetAllPIDsForProcessName( "3Cam Full Screen Mode", pids, 10, &count, NULL ) == noErr )
			allCount += count;
		if( ::GetAllPIDsForProcessName( "3Cam Video Recorder Mode", pids, 10, &count, NULL ) == noErr )
			allCount += count;
		onlyCopy = allCount < 2;

		#if kNTZPackageNumber == '3Cam'
		/* For Logical Choice, figure out from the bundle name which run mode we should	*/
		/* be in. Note that the executable name in the Contents/MacOS folder is the		*/
		/* same. We want to look at the name of the Contents/.. folder, e.g.			*/
		/* 3CamFS.app.																	*/
		if( onlyCopy ) {
			g3CamRunMode = k3CamRunModeNormal;
			CFBundleRef mainBundle = ::CFBundleGetMainBundle();
			if( mainBundle != NULL ) {
				CFURLRef bundleURL = ::CFBundleCopyBundleURL( mainBundle );
				if( bundleURL != NULL ) {
					CFURLRef bundleURLWithoutExtension = ::CFURLCreateCopyDeletingPathExtension( NULL, bundleURL );
					if( bundleURLWithoutExtension != NULL ) {
						CFStringRef bundleName = ::CFURLCopyLastPathComponent( bundleURLWithoutExtension );
						if( bundleName != NULL ) {
							// CFShow( bundleName );
							CFRange rangeFS = ::CFStringFind( bundleName, CFSTR( "Full Screen" ), kCFCompareCaseInsensitive );
							CFRange rangeVR = ::CFStringFind( bundleName, CFSTR( "Video Recorder" ), kCFCompareCaseInsensitive );
							if( rangeFS.location >= 0 )
								g3CamRunMode = k3CamRunModeFullScreen;
							else
								if( rangeVR.location >= 0 )
									g3CamRunMode = k3CamRunModeVideoRecorder;
							::CFRelease( bundleName );
							}
						::CFRelease( bundleURLWithoutExtension );
						}
					::CFRelease( bundleURL );
					}
				}

//#warning "delete"
//g3CamRunMode = k3CamRunModeFullScreen;
//g3CamRunMode = k3CamRunModeVideoRecorder;

			if( g3CamRunMode == k3CamRunModeFullScreen ) {
				CUtilities::fPStrCopy( gProductName, "\p3Cam Full Screen Mode" );
				CUtilities::fPStrCopy( gPreferenceName, "\p3Cam FS" );
				}
			else
				if( g3CamRunMode == k3CamRunModeVideoRecorder ) {
					CUtilities::fPStrCopy( gProductName, "\p3Cam Video Recorder Mode" );
					CUtilities::fPStrCopy( gPreferenceName, "\p3Cam VR" );
					}
				else
					CUtilities::fPStrCopy( gPreferenceName, "\p3Cam CP" );
			}
		#endif
		}

	return( onlyCopy );
	}

/*======================================================================================*/
CCaptureApp::CCaptureApp( void ) {

	fRegisterClasses();

	mMonitorList = NULL;
	mDontSaveClosedState = false;

	#if kNTZPackageNumber == '3Cam'

	mLCTOptionPressed = CUtilities2::fOptionPressed();

	#else

	if( CUtilities2::fOptionPressed() )
		gPreferences.fPreferencesInit( gPreferenceFolderName, gPreferenceName, kPrefsVersion, (OSType)-1, false, true, true );

	#endif

	if( CUtilities2::fOptionPressed() &&
		CUtilities2::fCommandPressed() )
		gPreferences.fPreferencesInit( gPreferenceFolderName, gPreferenceName, kPrefsVersion, (OSType)-1, false, true, true, true );

	gPreferences.fPreferencesInit( gPreferenceFolderName, gPreferenceName, kPrefsVersion );
	fGetPreferences();
	}

/*======================================================================================*/
void CCaptureApp::fGetPreferences( void ) {

	SInt32 keepAwake = 1;
	gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscKeepAwake, &keepAwake );
	mPrefKeepAwake = keepAwake != 0;
	SInt32 backgroundMute = 1;
	gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscBackgroundMute, &backgroundMute );
	mPrefBackgroundMute = backgroundMute != 0;
	mNextSleepTickle = 0;
	SInt32 addClipsToITunes = 0;
	gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscAddClipsToITunes, &addClipsToITunes );
	mPrefAddClipsToITunes = addClipsToITunes != 0;

	#if kNTZLicenseeNumber == 'gTc ' || kNTZLicenseeNumber == 'MAG ' || kNTZLicenseeNumber == 'ttC '
	mPrefSnapshotButtonMode = kSnapshotButtonModeRecordAutoName;
	#else
	mPrefSnapshotButtonMode = kSnapshotButtonModeSnapshot;
	#endif
	gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscSnapshotButtonMode, &mPrefSnapshotButtonMode );
	}

/*======================================================================================*/
CCaptureApp::~CCaptureApp( void ) {

	while( mMonitorList != NULL ) {
		sMonitorPtr next = mMonitorList->next;
		sMonitorPtr monitor = mMonitorList;
		if( monitor->window != NULL )
			delete monitor->window;
		::free( monitor );
		mMonitorList = next;
		}
	}

/*======================================================================================*/
void CCaptureApp::fRegisterClasses( void ) {
/* To reduce clutter within the Application object's constructor, class registrations	*/
/* appear here in this separate function for ease of use.								*/

	::RegisterAppearanceClient();

	/* Register core PowerPlant classes */
	RegisterClass_( LWindow );
	RegisterClass_( LDialogBox );

	/* Register the Appearance Manager/GA classes. You may want to remove this use of	*/
	/* UControlRegistry and instead perform a "manual" registration of the classes.		*/
	/* This cuts down on extra code being linked in and streamlines your app and		*/
	/* project. However, use UControlRegistry as a reference/index for your work, and	*/
	/* ensure to check UControlRegistry against your registrations each PowerPlant		*/
	/* release in case any mappings might have changed.									*/
	//	UControlRegistry::RegisterClasses();

	RegisterClass_( LIconControl );
	RegisterClass_( LPopupButton );
	RegisterClass_( LPushButton );
	RegisterClass_( LRadioButton );
	RegisterClass_( LRadioGroupView );
	RegisterClass_( LBevelButton );
	RegisterClass_( LCheckBox );
	RegisterClass_( LStaticText );
	RegisterClass_( LTextGroupBox );
	RegisterClass_( LTabGroup );
	RegisterClass_( LCheckBoxGroupBox );
	RegisterClass_( LEditText );
	RegisterClass_( LWindowThemeAttachment );

	RegisterClassID_( LAMControlImp, LIconControl::imp_class_ID );
	RegisterClassID_( LAMControlImp, LCheckBox::imp_class_ID );
	RegisterClassID_( LAMPopupButtonImp, LPopupButton::imp_class_ID );
	RegisterClassID_( LAMPushButtonImp, LPushButton::imp_class_ID );
	RegisterClassID_( LAMControlImp, LRadioButton::imp_class_ID );
	RegisterClassID_( LAMBevelButtonImp, LBevelButton::imp_class_ID );
	RegisterClassID_( LAMStaticTextImp, LStaticText::imp_class_ID );
	RegisterClassID_( LAMTextGroupBoxImp, LTextGroupBox::imp_class_ID );
	RegisterClassID_( LAMCheckBoxGroupBoxImp, LCheckBoxGroupBox::imp_class_ID );
	RegisterClassID_( LAMEditTextImp, LEditText::imp_class_ID );

	RegisterClass_( CCaptureWindow );
	RegisterClass_( CCaptureSnapshotSettings );
	RegisterClass_( CCaptureSnapshotSetup );
	RegisterClass_( CCaptureRecordSetup );
	RegisterClass_( CClickWindow );
	RegisterClass_( CClickView );
	RegisterClass_( CIconView );
	RegisterClass_( CURLCaption );
	}

/*======================================================================================*/
void CCaptureApp::StartUp( void ) {
/* Perform an action in response to the Open Application AppleEvent						*/

	SInt32 i;
	SInt32 j;
	Str255 s;
	SInt32 theErr = noErr;

	LMenuBar *menuBar = LMenuBar::GetCurrentMenuBar();
	LMenu *menu;

	/* If not release software, add the Edit Registers command */
	#if EXPIRES || EDIT_REGISTERS
	#pragma warning "Edit registers enabled"
	if( theErr == noErr ) {
		menu = menuBar->FetchMenu( kMenuEdit );
		menu->InsertCommand( "\p-", 0, 1000 );
		menu->InsertCommand( CUtilities2::fGetStringASCII( "EditRegisters",
			"kEditRegisters", s ), kCmdEditRegisters, 1000 );
		}
	#endif

	/* Get the list of devices. Note that InterView Lite devices will set mSerialized */
	sDeviceListPtr list = NULL;
	mSerialized = false;
	if( theErr == noErr )
		theErr = CCaptureWindow::fNewDeviceListCopy( &list, &mSerialized );

	/* 3/8/06. Serialization is being turned off for the Empia product. Since the		*/
	/* Capture application is common to all products we will just turn it off			*/
	/* universally. There is no point not protecting it in one product, especially		*/
	/* one with wide distribution, and continuing to protect it in all the others.		*/
	/* See CCaptureApp::SpendTime() for the only place serialization was ever actually	*/
	/* enforced in the Capture app.														*/

	#if 0
	/* Check serialization. If not serialized, calculate the appropriate cut-off time */
	if( theErr == noErr ) {
		if( !mSerialized )
			mSerialized = fSerialNumberinPrefs();
		if( !mSerialized )
			mEndTime = ::TickCount() + (DEMO_MAXMINUTES + 5) * 60 * 60;
		}
	#endif

	#if kNTZPackageNumber == '3Cam'
	if( g3CamRunMode == k3CamRunModeNormal ) {
	#endif

	#if kNTZPackageNumber == 'ttvr'
	/* Savage some menus for TerraTec */
	if( theErr == noErr )
		if( ::strcmp( CUtilities2::fGetLanguageCode(), "eng" ) == 0 ) {			
			ResIDT menuID;
			MenuHandle menuHandle;
			SInt16 item;
			LMenuBar *menuBar = LMenuBar::GetCurrentMenuBar();
			menuBar->FindMenuItem( cmd_Close, menuID, menuHandle, item );
			if( menuHandle != NULL )
				::SetMenuItemText( menuHandle, item, "\pClose Control Screen" );
			menuBar->FindMenuItem( kCmdAutoSnapshotSetup, menuID, menuHandle, item );
			if( menuHandle != NULL )
				::SetMenuItemText( menuHandle, item, "\pAuto-Filename SetupÉ" );
			menuBar->FindMenuItem( kCmdFullSize, menuID, menuHandle, item );
			if( menuHandle != NULL )
				::SetMenuItemText( menuHandle, item, "\pOriginal Size" );
			menuBar->FindMenuItem( kCmdAutoRecordSetup, menuID, menuHandle, item );
			if( menuHandle != NULL )
				::SetMenuItemText( menuHandle, item, "\pAuto-Filename SetupÉ" );
			}
	#endif

	#if kNTZPackageNumber == 'mgvr'
	/* Savage some menus for MAGIX */
	if( theErr == noErr ) {
		menu = new LMenu( kMenuSnapshotMAGIX );
		menuBar->InstallMenu( menu, kMenuSnapshot );
		menu = menuBar->FetchMenu( kMenuSnapshot );
		menuBar->RemoveMenu( menu );
		menu = new LMenu( kMenuRecordMAGIX );
		menuBar->InstallMenu( menu, kMenuRecord );
		menu = menuBar->FetchMenu( kMenuRecord );
		menuBar->RemoveMenu( menu );
		menu = new LMenu( kMenuDigitizersMAGIX );
		menuBar->InstallMenu( menu, kMenuDigitizers );
		menu = menuBar->FetchMenu( kMenuDigitizers );
		menuBar->RemoveMenu( menu );
		menu = new LMenu( kMenuHelpMAGIX );
		menuBar->InstallMenu( menu, 0 );
		}
	#endif

	/* Run through the device list and open all the digitizers that should be open */
	Boolean atLeastOneOpened = false;
	#if kNTZPackageNumber == 'mgvr'
	menu = menuBar->FetchMenu( kMenuDigitizersMAGIX );
	#else
	menu = menuBar->FetchMenu( kMenuDigitizers );
	#endif
	SInt32 index = 0;
	if( theErr == noErr )
		for( i = 0; i < list->count && theErr == noErr; i++ )
			for( j = 0; j < list->devices[i].inputCount || j == 0; j++ ) {
				sMonitorPtr monitor = NULL;
				if( theErr == noErr ) {
					monitor = (sMonitorPtr)::malloc( sizeof( sMonitor ) );
					if( monitor == NULL )
						theErr = memFullErr;
					}
				/* Make sure we want this device open */
				Boolean closed = false;
				#if kNTZPackageNumber == 'agsp'
				closed = list->devices[i].manufacturer != 'AGr ';
				#else
				if( theErr == noErr )
					fLoadClosedState( &list->devices[i], j, &closed );
				#endif
				/* Open a window for the device, and add it to the menu */
				if( theErr == noErr ) {
					monitor->window = NULL;
					if( !closed )
						if( fNewMonitor( index, &list->devices[i], j, &monitor->window ) == noErr )
							atLeastOneOpened = true;
					monitor->device = list->devices[i];
					monitor->input = j;
					/* Tack on to end of the list */
					monitor->next = NULL;
					if( mMonitorList == NULL )
						mMonitorList = monitor;
					else {
						sMonitorPtr next = mMonitorList;
						while( next->next != NULL )
							next = next->next;
						next->next = monitor;
						}
					/* Add to end of menu */
					if( list->devices[i].inputCount == 0 )
						menu->InsertCommand( list->devices[i].name, kCmdDigitizerBase + index, 1 );
					else
						menu->InsertCommand( list->devices[i].inputNames[j], kCmdDigitizerBase + index, 1 );
					}
				if( theErr != memFullErr )
					theErr = noErr;
				index++;
				}

		/* We will hold off turning the sound on until all the initial windows are open. We	*/
		/* call ActivateSelf() on the last window opened so that it can turn its sound on.	*/
		sMonitorPtr last = NULL;
		sMonitorPtr next = mMonitorList;
		while( next != NULL ) {
			if( next->window != NULL ) {
				last = next;
				next->window->fActivateSound();
				}
			next = next->next;
			}
		if( last != NULL )
			last->window->ActivateSelf();

		/* Handle no digitizers, or no digitizers open scenarios */
		if( mMonitorList == NULL ) {
			/* No sequencers found */
			StDialogHandler dialog( PPob_NoDigitizers, this );
			while( dialog.DoDialog() != msg_OK ) {}
			ObeyCommand( cmd_Quit, NULL );
			}
		else
			if( !atLeastOneOpened ) {
				StDialogHandler dialog( PPob_NoDigitizersOpen, this );
				while( dialog.DoDialog() != msg_OK ) {}
				}

	#if kNTZPackageNumber == '3Cam'
		}
	else
		theErr = fLogicalChoiceStartUpInMode( list );
	#endif

	if( list != NULL )
		CCaptureWindow::fDisposeDeviceListCopy( list );

	#if kNTZPackageNumber == 'mgvr'
	#endif
	
	/* Start checking for the cut-off time, if any, and keep the machine awake if desired */
	StartIdling();
	StartListening();
	}

#if kNTZPackageNumber == '3Cam'
/*======================================================================================*/
SInt32 CCaptureApp::fLogicalChoiceStartUpInMode( sDeviceListPtr list ) {
/* Logical Choice specialized run modes. We need all this mainly becase LCT only wants	*/
/* one digitizer up at a time, so we have to do what Startup would have done, but with	*/
/* only one device made available.														*/

	SInt32 i;
	SInt32 theErr = noErr;
	FSSpec newSpec;
	sDevicePtr device = NULL;
	SInt32 input = 0;

	/* Remove menus now so that they don't appear during this brief UI phase */
	if( theErr == noErr )
		if( g3CamRunMode == k3CamRunModeFullScreen ||
			g3CamRunMode == k3CamRunModeVideoRecorder ) {
			LMenu *menu;
			LMenuBar *menuBar = LMenuBar::GetCurrentMenuBar();
			menu = menuBar->FetchMenu( kMenuFileAqua );
			menuBar->RemoveMenu( menu );
			menu = menuBar->FetchMenu( kMenuEdit );
			menuBar->RemoveMenu( menu );
			menu = menuBar->FetchMenu( kMenuSnapshot );
			menuBar->RemoveMenu( menu );
			menu = menuBar->FetchMenu( kMenuRecord );			
			menuBar->RemoveMenu( menu );
			menu = menuBar->FetchMenu( kMenuDigitizers );
			menuBar->RemoveMenu( menu );
			}

	/* Look for the digitizer to use */
	if( theErr == noErr ) {

		/* No digitizers */
		if( list->count == 0 ) {
			StDialogHandler dialog( PPob_LogicalChoiceNoDigitizers, this );
			while( dialog.DoDialog() != msg_OK ) {}
			ObeyCommand( cmd_Quit, NULL );
			theErr = -128;
			}

		/* 1 digitizer with 1 input */
		else if( list->count == 1 && list->devices[0].inputCount <= 1 )
			device = &list->devices[0];

		/* Multiple inputs. Select one */
		else
			theErr = fLogicalChoiceSelectDevice( list, &device, &input );
		}

	if( theErr == noErr )
		if( device != NULL ) {

			/* Add the device to the digitizer menu. Unnecessary */
			if( theErr == noErr ) {
				LMenu *menu;
				LMenuBar *menuBar = LMenuBar::GetCurrentMenuBar();
				menu = menuBar->FetchMenu( kMenuDigitizers );
				if( menu != NULL )
					menu->InsertCommand( device->name, kCmdDigitizerBase, 1 );
				}

			/* If in vr mode, select the window size */
			SInt32 resolution;
			if( theErr == noErr )
				if( g3CamRunMode == k3CamRunModeVideoRecorder )
					theErr = fLogicalChoiceSelectResolution( &resolution );

			/* Create the window */
			CCaptureWindow *window;
			if( theErr == noErr )
				theErr = fNewMonitor( 0, device, input, &window );

			/* Activate the window */
			if( theErr == noErr ) {
				window->fActivateSound();
				window->ActivateSelf();
				}

			/* In full screen mode, use the whole screen */
			if( theErr == noErr )
				if( g3CamRunMode == k3CamRunModeFullScreen )
					window->ListenToMessage( kMsgLogicalChoiceFSSetup, NULL );

			/* In video recorder mode set the selected window size, and add the panel */
			if( theErr == noErr )
				if( g3CamRunMode == k3CamRunModeVideoRecorder )
					window->ListenToMessage( kMsgLogicalChoiceVRSetup, (void *)resolution );
			}

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureApp::fLogicalChoiceSelectDevice( sDeviceListPtr list, sDevicePtr *device,
	SInt32 *input ) {
/* If auto-select is disabled, or the user is holding down the option key, have the		*/
/* user manually select between the inputs. Note that we do not use device names here.	*/
/* we use input names. So if there are 2 device with 2 inputs each, there will be four	*/
/* menu items based on the input names.													*/

	SInt32 i;
	SInt32 j;
	SInt32 theErr = noErr;
	Str255 menuName;
	Str255 preferredInputName;

	/* See if auto-select is enabled */
	SInt32 autoSelect = 0;
	gPreferences.fGetPreference( kPrefTagLCT, kPrefLCTAutoSelectInput, &autoSelect );

	/* Get the index of the input we used last time. If we don't see the input we		*/
	/* prefer, turn auto-select off for this run.										*/
	SInt32 preferredDeviceIndex = 0;
	SInt32 preferredInputIndex = 0;
	if( gPreferences.fPreferenceExists( kPrefTagLCT, kPrefLCTPreferredInput ) ) {
		SInt32 length = 256;
		gPreferences.fGetPreference( kPrefTagLCT, kPrefLCTPreferredInput, (char *)preferredInputName, &length );
		Boolean found = false;
		for( i = 0; i < list->count && !found; i++ ) {
			for( j = 0; (j < list->devices[i].inputCount || j == 0) && !found; j++ ) {
				if( list->devices[i].inputCount == 0 )
					found = ::memcmp( preferredInputName, list->devices[i].name, length ) == 0;
				else
					found = ::memcmp( preferredInputName, list->devices[i].inputNames[j], length ) == 0;
				if( found ) {
					preferredDeviceIndex = i;
					preferredInputIndex = j;
					}
				}
			}
		if( !found )
			autoSelect = 0;
		}

	/* Auto-select the device to begin with */
	*device = &list->devices[preferredDeviceIndex];
	*input = preferredInputIndex;

	/* Manually select if necessary, or desired */
	if( autoSelect == 0 || mLCTOptionPressed ) {

		StDialogHandler dialog( PPob_LogicalChoiceSelectDigitizer, this );
		LWindow *window = dialog.GetDialog();
		LPopupButton *popDigitizers = dynamic_cast<LPopupButton *>( window->FindPaneByID( 'Digi' ) );
		LCheckBox *chkAlwaysUse = dynamic_cast<LCheckBox *>( window->FindPaneByID( 'Alws' ) );
		LStaticText *text = dynamic_cast<LStaticText *>( dialog.GetDialog()->FindPaneByID( 'warn' ) );
		LStr255 textString;
		chkAlwaysUse->GetDescriptor( textString );

		/* Add the input names to the popup menu */
		popDigitizers->DeleteAllMenuItems();
		for( i = 0; i < list->count; i++ )
			for( j = 0; j < list->devices[i].inputCount || j == 0; j++ )
				if( list->devices[i].inputCount == 0 )
					popDigitizers->AppendMenu( list->devices[i].name );
				else
					popDigitizers->AppendMenu( list->devices[i].inputNames[j] );

		/* Set the preferred device, if any, in the menu */
		SInt32 menuIndex = fLogicalChoiceGetInputMenuIndex( list, preferredDeviceIndex, preferredInputIndex );
		popDigitizers->SetCurrentMenuItem( menuIndex );

		/* Set the state of the auto-select checkbox */
		chkAlwaysUse->SetValue( autoSelect );
		LStr255 newString = textString;
		popDigitizers->GetMenuItemText( popDigitizers->GetCurrentMenuItem(), menuName );
		newString.Replace( newString.Find( "\p^1" ), 2, menuName );
		chkAlwaysUse->SetDescriptor( newString );
		if( autoSelect )
			text->Show();

		Boolean finished = false;
		Boolean selected = false;
		while( !finished ) {
			MessageT message = dialog.DoDialog();
			switch( message ) {
				case msg_OK:
					fLogicalChoiceGetPreferredIndices( list, popDigitizers->GetCurrentMenuItem(),
						&preferredDeviceIndex, &preferredInputIndex );
					*device = &list->devices[preferredDeviceIndex];
					*input = preferredInputIndex;
					gPreferences.fSetPreference( kPrefTagLCT, kPrefLCTAutoSelectInput,
						chkAlwaysUse->GetValue() );
					if( (*device)->inputCount == 0 )
						CUtilities::fPStrCopy( preferredInputName, (*device)->name );
					else
						CUtilities::fPStrCopy( preferredInputName, (*device)->inputNames[*input] );
					gPreferences.fSetPreference( kPrefTagLCT, kPrefLCTPreferredInput,
						(char *)preferredInputName, preferredInputName[0] + 1 );
					finished = true;
					selected = true;
					break;
				case msg_Cancel:
					finished = true;
					break;
				case 'Digi': {
					LStr255 newString = textString;
					popDigitizers->GetMenuItemText( popDigitizers->GetCurrentMenuItem(), menuName );
					newString.Replace( newString.Find( "\p^1" ), 2, menuName );
					chkAlwaysUse->SetDescriptor( newString );
					break;
					}
				case 'Alws':
					if( chkAlwaysUse->GetValue() ) {
						::SysBeep( 1 );
						text->Show();
						}
					else
						text->Hide();
					break;
				}
			}

		if( !selected ) {
			ObeyCommand( cmd_Quit, NULL );
			theErr = -128;
			}
		}

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureApp::fLogicalChoiceGetInputMenuIndex( sDeviceListPtr list,
	SInt32 deviceIndex, SInt32 inputIndex ) {

	SInt32 i;
	SInt32 j;

	SInt32 index = 0;
	Boolean found = false;
	for( i = 0; i < list->count && !found; i++ )
		for( j = 0; (j < list->devices[i].inputCount || j == 0) && !found; j++ ) {
			found = i == deviceIndex && j == inputIndex;
			if( !found )
				index++;
			}
	if( !found )
		index = 0;

	return( index + 1 );
	}

/*======================================================================================*/
void CCaptureApp::fLogicalChoiceGetPreferredIndices( sDeviceListPtr list,
	SInt32 menuIndex, SInt32 *deviceIndex, SInt32 *inputIndex ) {

	SInt32 i;
	SInt32 j;

	*deviceIndex = 0;
	*inputIndex = 0;

	SInt32 index = 0;
	Boolean found = false;
	for( i = 0; i < list->count && !found; i++ )
		for( j = 0; (j < list->devices[i].inputCount || j == 0) && !found; j++ ) {
			found = ++index == menuIndex;
			if( found ) {
				*deviceIndex = i;
				*inputIndex = j;
				}
			}
	}

/*======================================================================================*/
#define kResolution320x240		((320 << 16) | 240)
#define kResolution640x480		((640 << 16) | 480)
#define kResolution1280x960		((1280 << 16) | 960)

SInt32 CCaptureApp::fLogicalChoiceSelectResolution( SInt32 *resolution ) {
/* If auto-select is disabled, or the user is holding down the option key, have the		*/
/* user manually select the resolution.													*/

	SInt32 i;
	SInt32 theErr = noErr;

	/* See if auto-select is enabled */
	SInt32 autoSelect = 0;
	gPreferences.fGetPreference( kPrefTagLCT, kPrefLCTAutoSelectResolution, &autoSelect );

	/* Get the resolution we used last time. If we no longer support the preferred		*/
	/* resolution turn off aut-select and go back to default.							*/
	*resolution = kResolution320x240;
	gPreferences.fGetPreference( kPrefTagLCT, kPrefLCTPreferredResolution, resolution );
	if( *resolution != kResolution320x240 &&
		*resolution != kResolution640x480 &&
		*resolution != kResolution1280x960 ) {
		*resolution = kResolution320x240;
		autoSelect = 0;
		}

	/* Manually select if necessary, or desired */
	if( autoSelect == 0 || mLCTOptionPressed ) {

		StDialogHandler dialog( PPob_LogicalChoiceSelectResolution, this );
		LWindow *window = dialog.GetDialog();
		LRadioGroupView *radioGroup = dynamic_cast<LRadioGroupView *>( window->FindPaneByID( 'rgrp' ) );
		LCheckBox *chkAlwaysUse = dynamic_cast<LCheckBox *>( window->FindPaneByID( 'Alws' ) );
		LStaticText *text = dynamic_cast<LStaticText *>( dialog.GetDialog()->FindPaneByID( 'warn' ) );
		LStr255 textString;
		chkAlwaysUse->GetDescriptor( textString );

		/* Set the preferred resolution, if any, in the radio group */
		radioGroup->SetCurrentRadioID( *resolution );

		/* Set the state of the auto-select checkbox */
		chkAlwaysUse->SetValue( autoSelect );
		LStr255 newString = textString;
		newString.Replace( newString.Find( "\p^1" ), 2, fResolutionToString( *resolution ) );
		chkAlwaysUse->SetDescriptor( newString );
		if( autoSelect )
			text->Show();

		Boolean finished = false;
		Boolean selected = false;
		while( !finished ) {
			MessageT message = dialog.DoDialog();
			switch( message ) {
				case msg_OK:
					*resolution = radioGroup->GetCurrentRadioID();
					gPreferences.fSetPreference( kPrefTagLCT, kPrefLCTAutoSelectResolution, chkAlwaysUse->GetValue() );
					gPreferences.fSetPreference( kPrefTagLCT, kPrefLCTPreferredResolution, *resolution );
					finished = true;
					selected = true;
					break;
				case msg_Cancel:
					finished = true;
					break;
				case kResolution320x240:
				case kResolution640x480:
				case kResolution1280x960: {
					LStr255 newString = textString;
					newString.Replace( newString.Find( "\p^1" ), 2,
						fResolutionToString( radioGroup->GetCurrentRadioID() ) );
					chkAlwaysUse->SetDescriptor( newString );
					break;
					}
				case 'Alws':
					if( chkAlwaysUse->GetValue() ) {
						::SysBeep( 1 );
						text->Show();
						}
					else
						text->Hide();
					break;
				}
			}

		if( !selected ) {
			ObeyCommand( cmd_Quit, NULL );
			theErr = -128;
			}
		}

	return( theErr );
	}

/*======================================================================================*/
LStr255 CCaptureApp::fResolutionToString( SInt32 resolution ) {

	switch( resolution ) {
		case kResolution640x480:
			return( "\p640 x 480" );
		case kResolution1280x960:
			return( "\p1280 x 960" );
		}

	return( "\p320 x 240" );
	}

#endif
/*======================================================================================*/
SInt32 CCaptureApp::fNewMonitor( SInt32 index, sDevicePtr device, SInt32 input, CCaptureWindow **window ) {

	SInt32 theErr = noErr;

	*window = NULL;

	ResIDT resID = PPob_Monitor;
	#if kNTZPackageNumber == '3Cam'
	if( g3CamRunMode == k3CamRunModeFullScreen )
		resID = PPob_LogicalChoiceMonitorFS;
	if( g3CamRunMode == k3CamRunModeVideoRecorder )
		resID = PPob_LogicalChoiceMonitorVR;
	#endif
	if( theErr == noErr ) {
		*window = (CCaptureWindow *)CCaptureWindow::CreateWindow( resID, this );
		if( *window == NULL )
			theErr = memFullErr;
		}
	if( theErr == noErr )
		(*window)->AddListener( this );
	if( theErr == noErr )
		theErr = (*window)->fStartSequencer( index, device, input );
	if( theErr == noErr )
		#if kNTZPackageNumber == '3Cam'
		if( g3CamRunMode == k3CamRunModeVideoRecorder )
			(*window)->StartIdling( 0.033 );
		else
		#endif
		(*window)->StartIdling( 0.010 );

	if( theErr != noErr )
		if( *window != NULL ) {
			delete *window;
			*window = NULL;
			}

	return( theErr );
	}

/*======================================================================================*/
Boolean CCaptureApp::AllowSubRemoval( LCommander *inSub ) {
/* One of the monitors is closing. Make a note.											*/

	CCaptureWindow *window = dynamic_cast<CCaptureWindow *>( inSub );
	if( window != NULL ) {

		/* Indicate that the window is closed */
		sMonitorPtr next = mMonitorList;
		Boolean found = false;
		while( next != NULL && !found ) {
			if( next->window == window &&
				next->window != NULL ) {
				found = true;
				next->window->RemoveListener( this );
				next->window = NULL;
				}
			if( !found )
				next = next->next;
			}

		if( found )
			fSaveClosedState( &next->device, next->input, true );
		}

	return( true );	
	}

/*======================================================================================*/
void CCaptureApp::FindCommandStatus( CommandT inCommand, Boolean &outEnabled,
	Boolean &outUsesMark, UInt16 &outMark, Str255 outName ) {
/* Determine the status of a Command for the purposes of menu updating.					*/

	LMenuBar *menuBar;
	Boolean cmdHandled = false;

	/* Empia version cannot contain large snapshot support for the Nogatech devices.	*/
	/* Doing so would violate confidentiality agreements with Zoran.					*/
	#if kNTZLicenseeNumber == 'eEm '
	static Boolean gLargeSnapshotRemoved = false;
	if( !gLargeSnapshotRemoved ) {
		menuBar = LMenuBar::GetCurrentMenuBar();
		LMenu *menu = menuBar->FetchMenu( kMenuSnapshot );
		menu->RemoveCommand( kCmdSnapshotLarge );
		MenuRef menuHandle = menu->GetMacMenuH();
		Str255 partial;
		::SetMenuItemText( menuHandle, 1, CUtilities2::fGetStringASCII( "Capture", "kCaptureSnapshpt", partial ); );
		gLargeSnapshotRemoved = true;
		}
	#endif

	if( !cmdHandled )
		switch( inCommand ) {

			case cmd_About:
				LApplication::FindCommandStatus( inCommand, outEnabled, outUsesMark, outMark,
					outName );
				menuBar = LMenuBar::GetCurrentMenuBar();
				if( menuBar != NULL ) {
					ResIDT menuID;
					MenuHandle menuHandle;
					SInt16 item;
					menuBar->FindMenuItem( cmd_About, menuID, menuHandle, item );
					if( menuID != 0 ) {
						LStr255 menuText;
						GetIndString( menuText, kStrings, kStringsAbout );
						menuText.Replace( menuText.Find( "\p^0" ), 2, gProductName );
						CUtilities::fPStrCopy( outName, menuText );
						}
					}
				cmdHandled = true;
				break;

			case cmd_Preferences:
				outEnabled = true;
				cmdHandled = true;
				break;

			case kCmdHelp:
				outEnabled = true;
				cmdHandled = true;
				break;

			case kCmdRegisterProduct:
				outEnabled = true;
				cmdHandled = true;
				break;

			case kCmdSnapshotSettings:
			case kCmdAutoSnapshotSetup:
			case kCmdAutoRecordSetup:
				outEnabled = true;
				cmdHandled = true;
				break;

			case kCmdRecordVideo:
			case kCmdRecordSound:
				outUsesMark = true;
				outMark = noMark;
				outEnabled = false;
				cmdHandled = true;
				break;

			default:
				if( inCommand / 1000 == kCmdDigitizerBase / 1000 ) {
					outUsesMark = true;
					outMark = 0;
					CCaptureWindow *window = dynamic_cast<CCaptureWindow *>( UDesktop::FetchTopRegular() );
					if( window != NULL ) {
						SInt32 index = inCommand % 1000;
						SInt32 i = 0;
						Boolean found = false;
						sMonitorPtr next = mMonitorList;
						while( next != NULL && !found ) {
							if( next->window == window &&
								next->window != NULL &&
								i == index ) {
								found = true;
								outMark = checkMark;
								}
							i++;
							next = next->next;
							}
						}
					outEnabled = true;
					cmdHandled = true;
					}
				break;
			}

	if( !cmdHandled )
		LApplication::FindCommandStatus( inCommand, outEnabled, outUsesMark, outMark, outName );
	}

/*======================================================================================*/
Boolean CCaptureApp::ObeyCommand( CommandT inCommand, void *ioParam ) {
/* Respond to Commands. Returns true if the Command was handled, false if not.			*/

	SInt32 i;
	Boolean cmdHandled = false;

	if( !cmdHandled )
		switch( inCommand ) {
			case kCmdHelp:
				cmdHandled = true;
				fOpenHelpFile( "MAGIX", ".pdf" );
				break;
			case kCmdRegisterProduct:
				cmdHandled = true;
				CURLCaption::fOpenURL( "\phttp://rdir.magix.net/?page=UMFMLDIXNTUH" );
				break;
			case kCmdSnapshotSettings:
				cmdHandled = true;
				LWindow::CreateWindow( PPob_SnapshotSettings, this );
				break;
			case kCmdAutoSnapshotSetup:
				cmdHandled = true;
				LWindow::CreateWindow( PPob_AutoSnapshotSetup, this );
				break;
			case kCmdAutoRecordSetup:
				cmdHandled = true;
				LWindow::CreateWindow( PPob_AutoRecordSetup, this );
				break;
			default:
				/* Digitizer menu */
				if( inCommand / 1000 == kCmdDigitizerBase / 1000 ) {
					cmdHandled = true;
					SInt32 which = inCommand - kCmdDigitizerBase;
					sMonitorPtr monitor = mMonitorList;
					for( i = 0; i < which && monitor != NULL; i++ )
						monitor = monitor->next;
					if( monitor != NULL ) {
						if( monitor->window != NULL )
							monitor->window->Select();
						else
							if( fNewMonitor( which, &monitor->device, monitor->input, &monitor->window ) == noErr ) {
								monitor->window->fActivateSound();
								monitor->window->Select();
								fSaveClosedState( &monitor->device, monitor->input, false );
								}
						}
					}
				break;
			}

	if( !cmdHandled )
		cmdHandled = LApplication::ObeyCommand( inCommand, ioParam );

	return( cmdHandled );
	}

/*======================================================================================*/
void CCaptureApp::ShowAboutBox( void ) {

	CUAboutBoxHandler aboutBox( PPob_AboutWindow, this, gLicenseeShortName, gProductName );

	/* Special "copyright" for iRez */
	#if kNTZLicenseeNumber == 'iRz ' || kNTZLicenseeNumber == 'lct ' || kNTZLicenseeNumber == 'MAG ' || kNTZLicenseeNumber == 'ttC '
	LStaticText *pane = (LStaticText *)aboutBox.GetDialog()->FindPaneByID( 'COPY' );
	if( pane != NULL ) {
		LStr255 s;
		CUtilities2::fGetStringASCII( "Capture", "kCaptureDistributed", s );
		pane->SetDescriptor( s + LStr255( "\r" ) + LStr255( kNTZLicenseeNameLong ) );
		pane->ResizeFrameBy( 0, 14, true );
		}
	aboutBox.GetDialog()->ResizeWindowBy( 0, 14 );
	#endif

	#if kNTZLicenseeNumber == 'MAG '
	LStaticText *paneBlurb = (LStaticText *)aboutBox.GetDialog()->FindPaneByID( 'Blrb' );
	if( paneBlurb != NULL )
		paneBlurb->Hide();
	#endif

	aboutBox.GetDialog()->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = aboutBox.DoDialog();
		switch( message ) {
			case msg_OK:
			case msg_Cancel:
				finished = true;
				break;
			}
		}
	}

/*======================================================================================*/
void CCaptureApp::DoPreferences( void ) {

	StDialogHandler handler( PPob_Preferences, this );
	LWindow *dialog = handler.GetDialog();

	LCheckBox *chkWake = dynamic_cast<LCheckBox *>( dialog->FindPaneByID( 'wake' ) );
	chkWake->SetValue( mPrefKeepAwake ? 1 : 0 );

	LCheckBox *chkMute = dynamic_cast<LCheckBox *>( dialog->FindPaneByID( 'mute' ) );
	chkMute->SetValue( mPrefBackgroundMute ? 1 : 0 );

	LCheckBox *chkITunes = dynamic_cast<LCheckBox *>( dialog->FindPaneByID( 'tune' ) );
	chkITunes->SetValue( mPrefAddClipsToITunes ? 1 : 0 );

	//LCheckBox *chkSync = dynamic_cast<LCheckBox *>( dialog->FindPaneByID( 'sync' ) );
	//SInt32 synchronize;
	//CCaptureSynchronize::fGetPreferenceSynchronize( &synchronize );
	//chkSync->SetValue( synchronize );
	//if( !CCaptureSynchronize::fSupported() )
	//	chkSync->Disable();

	LPopupButton *popSnapshot = dynamic_cast<LPopupButton *>( dialog->FindPaneByID( 'SBut' ) );
	popSnapshot->SetValue( mPrefSnapshotButtonMode + 1 );

	LPushButton *butAutoNameSetup = dynamic_cast<LPushButton *>( dialog->FindPaneByID( 'ANSu' ) );
	if( popSnapshot->GetValue() - 1 < kSnapshotButtonModeSnapshotAutoName )
		butAutoNameSetup->Disable();
	else
		butAutoNameSetup->Enable();

	Boolean finished = false;
	while( !finished ) {
		MessageT message = handler.DoDialog();
		switch( message ) {

			case 'SBut':
				if( popSnapshot->GetValue() - 1 < kSnapshotButtonModeSnapshotAutoName )
					butAutoNameSetup->Disable();
				else
					butAutoNameSetup->Enable();
				break;

			case 'ANSu':
				if( popSnapshot->GetValue() - 1 == kSnapshotButtonModeSnapshotAutoName )
					LWindow::CreateWindow( PPob_AutoSnapshotButtonSetup, this );
				else
					if( popSnapshot->GetValue() - 1 == kSnapshotButtonModeRecordAutoName )
						LWindow::CreateWindow( PPob_AutoRecordButtonSetup, this );
				break;

			case msg_OK:
				finished = true;
				mPrefKeepAwake = chkWake->GetValue() != 0;
				gPreferences.fSetPreference( kPrefTagMisc, kPrefMiscKeepAwake, mPrefKeepAwake ? 1 : 0 );
				mPrefBackgroundMute = chkMute->GetValue() != 0;
				gPreferences.fSetPreference( kPrefTagMisc, kPrefMiscBackgroundMute, mPrefBackgroundMute ? 1 : 0 );
				mPrefAddClipsToITunes = chkITunes->GetValue() != 0;
				gPreferences.fSetPreference( kPrefTagMisc, kPrefMiscAddClipsToITunes, mPrefAddClipsToITunes ? 1 : 0 );
				mPrefSnapshotButtonMode = popSnapshot->GetValue() - 1;
				gPreferences.fSetPreference( kPrefTagMisc, kPrefMiscSnapshotButtonMode, mPrefSnapshotButtonMode );
				//CCaptureSynchronize::fSetPreferenceSynchronize( chkSync->GetValue() );
				break;
			case msg_Cancel:
				finished = true;
				break;
			}
		}
	}

/*======================================================================================*/
void CCaptureApp::HandleAppleEvent( const AppleEvent &inAppleEvent, AppleEvent &outAEReply,
	AEDesc &outResult, SInt32 inAENumber ) {
/* Here's where we catch our fun events. Pass on anything we don't handle to the		*/
/* king vampire.																		*/
/* We need to modify something somewhere to translate events to magical numbers.		*/
/* It's an 'aedt' resource. It can be any ID you like. See UAppleEventsMgr.cp for		*/
/* details.																				*/

	StAEDescriptor theDesc;
	Boolean specGood;
	FSSpec spec;
	Str255 windowName;
	CCaptureWindow *monitor;
	Boolean large;
	UInt32 duration;
	WindowPtr windowP;

	SetDebugThrow_( debugAction_Nothing );
	switch( inAENumber ) {

		case 16000:
		case 16001:
		case 16002:
			specGood = false;
			try {
				theDesc.GetOptionalParamDesc( inAppleEvent, 'kFil', typeFSS );
				if( theDesc.IsNotNull() ) {
					UExtractFromAEDesc::TheFSSpec( (AEDesc &)theDesc, spec );
					specGood = true;
					}
				}
			catch( ... ) {
				}
			if( !specGood ) {
				try {
					theDesc.GetOptionalParamDesc( inAppleEvent, 'kFil', typeChar );
					if( theDesc.IsNotNull() ) {
						Str255 name;
						UExtractFromAEDesc::ThePString( (AEDesc &)theDesc, name );
						specGood = fConvertNameToSpec( name, &spec ) == noErr;
						}
					}
				catch( ... ) {
					}
				}

			windowName[0] = 0;
			try {
				theDesc.GetOptionalParamDesc( inAppleEvent, 'kWch', typeChar );
				if( theDesc.IsNotNull() )
					UExtractFromAEDesc::ThePString( (AEDesc &)theDesc, windowName, 255 );
				}
			catch( ... ) {
				}

			large = false;
			try {
				theDesc.GetOptionalParamDesc( inAppleEvent, 'kLrg', typeBoolean );
				if( theDesc.IsNotNull() )
					UExtractFromAEDesc::TheBoolean( (AEDesc &)theDesc, large );
				}
			catch( ... ) {
				}

			duration = 0;
			try {
				theDesc.GetOptionalParamDesc( inAppleEvent, 'kDur', typeUInt32 );
				if( theDesc.IsNotNull() )
					UExtractFromAEDesc::TheUInt32( (AEDesc &)theDesc, duration );
				}
			catch( ... ) {
				}

			fFindMonitorByName( windowName, &monitor );
			break;

		case ae_Close:
			/* I'll get this if no parameter is specified in the AppleScript */
			break;
		}
	SetDebugThrow_( debugAction_Alert );

	switch( inAENumber ) {

		case 16000:
			/* Start recording */
			if( monitor != NULL )
				monitor->fStartRecording( duration, specGood ? &spec : NULL, NULL, false );
			break;

		case 16001:
			/* Stop recording */
			::SysBeep( 1 );
			if( monitor != NULL )
				monitor->fStopRecording();
			break;

		case 16002:
			/* Take snapshot */
			if( monitor != NULL )
				#if kNTZLicenseeNumber != 'eEm '
				if( large )
					monitor->fSnapshotLarge( specGood ? &spec : NULL, false );
				else
				#endif
					monitor->fSnapshotSmall( specGood ? &spec : NULL, false );
			break;

		case ae_Close:
			windowP = UWindows::FindNthWindow( 1 );
			if( windowP != NULL ) {
				LWindow *window = LWindow::FetchWindowObject( windowP );
				if( window != NULL )
					window->DoClose();
				}
			else
				Throw_( errAEEventNotHandled );
			break;

		default:
			try {
				LApplication::HandleAppleEvent( inAppleEvent, outAEReply, outResult, inAENumber );
				}
			catch( ... ) {
				/* Quitting QuickTime Player if opened from the Movie recorded			*/
				/* dialog sends an application died event that is unhandled and thrown	*/
				/* that will crash us in OS 10.5 if we don't catch it here.				*/
				}
			break;
		}
	}

/*======================================================================================*/
void CCaptureApp::GetSubModelByName( DescType inModelID, Str255 inName, AEDesc &outToken ) const {
/* Open the desired digitizer window so that the default method will find it. After		*/
/* that, the window will be either opened or closed, depending on the apple event		*/
/* which, unfortunately, we have no knowledge of at this point.							*/

	SInt32 i;

	switch( inModelID ) {
		case cWindow:
			WindowPtr windowP = UWindows::FindNamedWindow( inName );
			if( windowP == NULL ) {
				#if kNTZPackageNumber == 'mgvr'
				LMenu *menu = LMenuBar::GetCurrentMenuBar()->FetchMenu( kMenuDigitizersMAGIX );
				#else
				LMenu *menu = LMenuBar::GetCurrentMenuBar()->FetchMenu( kMenuDigitizers );
				#endif
				ThrowIfNULL_( menu );
				SInt32 index = menu->IndexFromCommand( kCmdDigitizerBase );
				MenuHandle menuH = menu->GetMacMenuH();
				SInt32 menuCount = ::CountMenuItems( menuH );
				Boolean found = false;
				SInt32 command;
				for( i = index; i <= menuCount && !found; i++ ) {
					Str255 itemText;
					::GetMenuItemText( menuH, i, itemText );
					found = LString::ToolboxCompareText( &itemText[1], &inName[1], itemText[0], inName[0] ) == 0;
					if( found )
						command = menu->CommandFromIndex( i );
					}
				if( found )
					const_cast<CCaptureApp *>( this )->ObeyCommand( command, NULL );
				}
			break;
		}

	LApplication::GetSubModelByName( inModelID, inName, outToken );
	}

/*======================================================================================*/
SInt32 CCaptureApp::fConvertNameToSpec( Str255 name, FSSpec *spec ) {
/* If a user just provides a text name for the file name, root the path at the desktop.	*/

	SInt32 theErr = noErr;

	SInt16 vRefNum;
	SInt32 dirID;
	if( theErr == noErr )
		theErr = ::FindFolder( kOnAppropriateDisk, kDesktopFolderType, false,
			&vRefNum, &dirID );
	if( theErr == noErr )
		::FSMakeFSSpec( vRefNum, dirID, name, spec );

	return( theErr );
	}

/*======================================================================================*/
void CCaptureApp::fFindMonitorByName( Str255 windowName, CCaptureWindow **window ) {
/* Default to front window.																*/

	*window = dynamic_cast<CCaptureWindow *>( UDesktop::FetchTopRegular() );
	if( windowName[0] != 0 ) {
		Boolean found = false;
		sMonitorPtr next = mMonitorList;
		while( next != NULL && !found ) {
			Str255 nextName;
			if( next->window != NULL ) {
				next->window->GetDescriptor( nextName );
				found = ::fPStrEqual( nextName, windowName );
				}
			if( found )
				*window = next->window;
			next = next->next;
			}
		}
	}

/*======================================================================================*/
void CCaptureApp::fCloseAll( void ) {

	sMonitorPtr next = mMonitorList;
	while( next != NULL ) {
		if( next->window != NULL )
			next->window->AttemptClose();
		next = next->next;
		}
	}

/*======================================================================================*/
void CCaptureApp::fSaveClosedState( sDevicePtr device, SInt32 input, Boolean closed ) {
/* Update the closed device preference.													*/

	if( !mDontSaveClosedState ) {
		SInt32 preferenceID;
		if( CCaptureWindow::fGetPreferenceID( device, input, &preferenceID ) )
			gPreferences.fSetPreference( kPrefTagDigitizerClosed, preferenceID, closed ? 1 : 0 );
		}
	}

/*======================================================================================*/
void CCaptureApp::fLoadClosedState( sDevicePtr device, SInt32 input, Boolean *closed ) {
/* Update the closed device preference.													*/

	*closed = false;
	SInt32 preferenceID;
	if( CCaptureWindow::fGetPreferenceID( device, input, &preferenceID ) ) {
		SInt32 state = 0;
		gPreferences.fGetPreference( kPrefTagDigitizerClosed, preferenceID, &state );
		*closed = state != 0;
		}
	}

/*======================================================================================*/
static Boolean fPStrEqual( Str255 s1, Str255 s2 ) {
/* Returns true if the pascal strings are the same.									*/

	SInt32 i;
	if( s1[0] != s2[0] )
		return( 0 );
	for( i = 0; i <= s1[0]; i++ )
		if( s1[i] != s2[i] )
			return( 0 );
	return( 1 );
	}

/*======================================================================================*/
void CCaptureApp::SpendTime( const EventRecord &inMacEvent ) {

	#pragma unused( inMacEvent )

	/* 3/8/06. Serialization is being turned off for the Empia product. Since the		*/
	/* Capture application is common to all products we will just turn it off			*/
	/* universally. There is no point not protecting it in one product, especially		*/
	/* one with wide distribution, and continuing to protect it in all the others.		*/

	#if 0
	if( !mSerialized ) {
		UInt32 currentTime = ::TickCount();
		if( currentTime > mEndTime ) {

			StopIdling();

			mDontSaveClosedState = true;
			fCloseAll();
			mDontSaveClosedState = false;

			StDialogHandler dialog( PPob_Unserialized, this );
			LStaticText *text = dynamic_cast<LStaticText *>( dialog.GetDialog()->FindPaneByID( 'text' ) );
			ThrowIfNULL_( text );
			LStr255 string;
			text->GetDescriptor( string );
			string.Replace( string.Find( "\p^0" ), 2, gProductName );
			text->SetDescriptor( string );
			while( dialog.DoDialog() != msg_OK ) {}

			SendAEQuit();
			}
		}
	#endif

	/* Keep the machine awake (if there are open digitizers) */
	if( mPrefKeepAwake ) {
		sMonitorPtr next = mMonitorList;
		Boolean open = false;
		while( next != NULL && !open ) {
			open = next->window != NULL;
			next = next->next;
			}
		if( open ) {
			UInt32 tickCount = ::TickCount();
			if( mNextSleepTickle < tickCount ) {
				mNextSleepTickle = tickCount + 25 * 60;
				::UpdateSystemActivity( UsrActivity );
				}
			}
		}
	}

/*======================================================================================*/
void CCaptureApp::ListenToMessage( MessageT inMessage, void *ioParam ) {
/* Delivered by a capture window when various events occur that should cause something	*/
/* to happen in all the other capture windows.											*/

	sMonitorPtr next = mMonitorList;
	while( next != NULL ) {
		if( next->window != NULL )
			switch( inMessage ) {
				case 'paus':
				case 'resu':
					next->window->fPause( inMessage == 'paus', (CCaptureWindow *)ioParam );
					break;
				case 'aStp':
					/* Stop the audio everywhere except the ioParamth window */
					if( next->window != (CCaptureWindow *)ioParam )
						next->window->fStopSoundPreview();
					break;
				}
		next = next->next;
		}
	}

/*======================================================================================*/
Boolean CCaptureApp::fSerialNumberinPrefs( void ) {
/* Just look for any serial number in the "product Preferences" file. There is no		*/
/* need to validate it.																	*/

	Boolean found = false;
	CUPreferences preferences;
	Str255 driverName;
	::GetIndString( driverName, kStrings, kStringsDriverName );
	if( preferences.fPreferencesInit( gPreferenceFolderName, driverName, 0, (OSType)-1, true ) == noErr )
		found = preferences.fPreferenceExists( 'pres', 1000 );

	return( found );
	}

/*======================================================================================*/
SInt32 CCaptureApp::fOpenHelpFile( const char *beginsWith, const char *endsWith ) {

	SInt32 i;
	SInt32 theErr = noErr;

	FSRef applicationFolder;

	/* Get the .app folder */
	if( theErr == noErr )
		theErr = CUtilities2::fGetApplicationBundleFSRef( &applicationFolder );

	/* Get it's parent folder */
	FSRef parentRef;
	if( theErr == noErr )
		theErr = ::FSGetCatalogInfo( &applicationFolder, kFSCatInfoNone, NULL, NULL, NULL, &parentRef );

	/* Search the parent ref for a file ending in .pdf */
	FSRef helpFile;
	Boolean found = false;
	if( theErr == noErr ) {
		FSIterator iterator = NULL;
		if( theErr == noErr )
			theErr = ::FSOpenIterator( &parentRef, kFSIterateFlat, &iterator );
		while( theErr == noErr && !found ) {
			/* Get info for the next kNumObjects objects */
			#define kNumObjects 20
			UInt32 actualObjects;
			Boolean containerChanged;
			FSRef fsRefList[kNumObjects];
			if( theErr == noErr )
				theErr = ::FSGetCatalogInfoBulk( iterator, kNumObjects, &actualObjects,
					&containerChanged, 0, NULL, fsRefList, NULL, NULL );
			/* For each object found test it */
			if( (theErr == noErr || theErr == errFSNoMoreItems) && actualObjects != 0 )
				for( i = 0; i < actualObjects && !found; i++ ) {
					FSRef target = fsRefList[i];

					/* See if this is the help file. If so, remember it and stop		*/
					/* iterating.														*/
					FSSpec spec;
					if( ::FSGetCatalogInfo( &target, kFSCatInfoNone, NULL, NULL, &spec, NULL ) == noErr ) {
						LStr255 name( spec.name );
						if( name.BeginsWith( beginsWith, ::strlen( beginsWith ) ) &&
							name.EndsWith( endsWith, ::strlen( endsWith ) ) ) {
							found = true;
							helpFile = target;
							}
						}
					}
			}
		if( theErr == errFSNoMoreItems ||
			theErr == afpAccessDenied )
			theErr = noErr;
		if( iterator != NULL )
			::FSCloseIterator( iterator );
		if( !found )
			theErr = -1;
		}

	/* We have something. Open it. We can ask Finder to figure out what to do with it */
	char path[1024];
	if( theErr == noErr )
		theErr = ::FSRefMakePath( &helpFile, (UInt8 *)path, 1024 );
	if( theErr == noErr ) {
		char buffer[1500];
		::sprintf( buffer, "osascript "
			"-e 'tell application \"Finder\"' "
			"-e 'open(POSIX file \"%s\")' "
			"-e 'end tell'", path );
		::system( buffer );
		}

	return( theErr );
	}

/*======================================================================================*/
