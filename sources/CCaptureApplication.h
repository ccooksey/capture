/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#ifndef CCaptureApplication_h
#define CCaptureApplication_h
#pragma once

#if TARGET_RT_MAC_MACHO
#ifndef __CARBON__
#include <Carbon/Carbon.h>
#endif
#else
#include <MacTypes.h>
#endif

#include <LApplication.h>

#include "CCaptureWindow.h"

#define kPrefsVersion				3

enum {
	kSnapshotButtonModeSnapshot = 0,
	kSnapshotButtonModeRecord,
	kSnapshotButtonModeSnapshotAutoName,
	kSnapshotButtonModeRecordAutoName
	};

typedef struct sMonitor {
	CCaptureWindow *window;
	sDevice device;
	SInt32 input;
	struct sMonitor *next;
	} sMonitor, *sMonitorPtr;

#if kNTZPackageNumber == '3Cam'
enum {
	k3CamRunModeNormal = 0,
	k3CamRunModeFullScreen,
	k3CamRunModeVideoRecorder
	};
extern SInt32 g3CamRunMode;
#endif

class CCaptureApp :
	public LApplication,
	public LPeriodical,
	public LListener {

	public:

		CCaptureApp();
		virtual ~CCaptureApp();

		Boolean AllowSubRemoval( LCommander *inSub );
		virtual void FindCommandStatus( CommandT inCommand, Boolean &outEnabled, Boolean &outUsesMark, UInt16 &outMark, Str255 outName );
		virtual Boolean ObeyCommand( CommandT inCommand, void *ioParam = NULL );	
		virtual void ShowAboutBox( void );
		virtual void DoPreferences( void );
		virtual void HandleAppleEvent( const AppleEvent &inAppleEvent, AppleEvent &outAEReply, AEDesc &outResult, SInt32 inAENumber );
		virtual	void SpendTime( const EventRecord &inMacEvent );
		virtual void ListenToMessage( MessageT inMessage, void *ioParam );

		virtual void GetSubModelByName( DescType inModelID, Str255 inName, AEDesc &outToken ) const;

	protected:

		virtual void StartUp();
	
		void fRegisterClasses();

	private:

		#if kNTZPackageNumber == '3Cam'
		SInt32 fLogicalChoiceStartUpInMode( sDeviceListPtr list );
		SInt32 fLogicalChoiceSelectDevice( sDeviceListPtr list, sDevicePtr *device, SInt32 *input );
		SInt32 fLogicalChoiceGetInputMenuIndex( sDeviceListPtr list, SInt32 deviceIndex, SInt32 inputIndex );
		void fLogicalChoiceGetPreferredIndices( sDeviceListPtr list, SInt32 menuIndex, SInt32 *deviceIndex, SInt32 *inputIndex );
		SInt32 fLogicalChoiceSelectResolution( SInt32 *resolution );
		LStr255 fResolutionToString( SInt32 resolution );
		Boolean mLCTOptionPressed;
		#endif
		SInt32 fNewMonitor( SInt32 i, sDevicePtr device, SInt32 input, CCaptureWindow **window );
		void fDoAbout( void );
		SInt32 fConvertNameToSpec( Str255 name, FSSpec *spec );
		void fFindMonitorByName( Str255 windowName, CCaptureWindow **window );
		void fCloseAll( void );
		void fSaveClosedState( sDevicePtr device, SInt32 input, Boolean closed );
		void fLoadClosedState( sDevicePtr device, SInt32 input, Boolean *closed );
		Boolean fSerialNumberinPrefs( void );
		SInt32 fOpenHelpFile( const char *beginsWith, const char *endsWith );
		void fGetPreferences( void );

		sMonitor *mMonitorList;
		Boolean mDontSaveClosedState;
		Boolean mSerialized;
		UInt32 mEndTime;

		Boolean mPrefKeepAwake;
		Boolean mPrefBackgroundMute;
		UInt32 mNextSleepTickle;	// ticks
		Boolean mPrefAddClipsToITunes;

		SInt32 mPrefSnapshotButtonMode;

	};

#endif