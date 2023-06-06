/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#pragma once
#ifndef CCaptureSnapshotSetup_h
#define CCaptureSnapshotSetup_h

#if TARGET_RT_MAC_MACHO
#ifndef __CARBON__
#include <Carbon/Carbon.h>
#endif
#ifndef __QUICKTIMECOMPONENTS__
#include <QuickTime/QuickTimeComponents.h>
#endif
#else
#include <MacTypes.h>
#include <QuickTimeComponents.h>
#endif

#include <LEditText.h>
#include <LPushButton.h>
#include <LCheckBoxGroupBox.h>
#include <LStaticText.h>
#include <LPopupButton.h>

/*--------------------------------------------------------------------------------------*/
class CCaptureSnapshotSetup :
	public LWindow,
	public LListener {

	public:

		enum { class_ID = 'SSsu' };
		static CCaptureSnapshotSetup *CreateFromStream( LStream *inStream );
		CCaptureSnapshotSetup( LStream *inStream );
		virtual ~CCaptureSnapshotSetup( void );

		static void fGetPreferenceAutomatic( SInt32 *automatic );
		static void fSetPreferenceAutomatic( SInt32 automatic );
		static void fGetPreferencesDestination( char *path, char *name );

		virtual	void FinishCreateSelf( void );
		virtual void ListenToMessage( MessageT inMessage, void *ioParam );

	private:

		SInt32 mAuto;
		char mPath[1024];
		char mName[256];

		LCheckBoxGroupBox *mAutoGroupCheckbox;
		LEditText *mPathEditText;
		LPushButton *mChoosePathButton;
		LEditText *mNameEditText;
		LPushButton *mSettingsButton;
		LPushButton *mOKButton;
		LPushButton *mCancelButton;

	};

#endif

