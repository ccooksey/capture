/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#pragma once
#ifndef CCaptureRecordSetup_h
#define CCaptureRecordSetup_h

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

/*--------------------------------------------------------------------------------------*/
class CCaptureRecordSetup :
	public LWindow,
	public LListener {

	public:

		enum { class_ID = 'RCsu' };
		static CCaptureRecordSetup *CreateFromStream( LStream *inStream );
		CCaptureRecordSetup( LStream *inStream );
		virtual ~CCaptureRecordSetup( void );

		static void fGetPreferenceAutomatic( SInt32 *automatic );
		static void fSetPreferenceAutomatic( SInt32 automatic );
		static void fGetPreferencesDestination( char *path, char *name );

		virtual	void FinishCreateSelf( void );
		virtual void ListenToMessage( MessageT inMessage, void *ioParam );

	protected:

		SInt32 mAuto;
		char mPath[1024];
		char mName[256];

		LCheckBoxGroupBox *mAutoGroupCheckbox;
		LEditText *mPathEditText;
		LPushButton *mChoosePathButton;
		LEditText *mNameEditText;
		LPushButton *mOKButton;
		LPushButton *mCancelButton;

	};

#endif
