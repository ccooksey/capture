/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include <stdio.h>
#include <string.h>

#include <UStandardDialogs.h>
#include <LStaticText.h>

#include "CCaptureSnapshotSetup.h"
#include "CUPreferences.h"
#include "CUtilities2.h"
#include "CaptureConstants.h"

#include "NTZRelease.h"
#include "NTZLogging.h"
//#include "NTZComponents.h"
#include "NTZVDComponents.h"

/*--------------------------------------------------------------------------------------*/
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/*======================================================================================*/
CCaptureSnapshotSetup *CCaptureSnapshotSetup::CreateFromStream( LStream *inStream ) {

	return( new CCaptureSnapshotSetup( inStream ) );
	}

/*======================================================================================*/
CCaptureSnapshotSetup::CCaptureSnapshotSetup( LStream *inStream ) :
	LWindow( inStream ) {
	}

/*======================================================================================*/
CCaptureSnapshotSetup::~CCaptureSnapshotSetup( void ) {
	}

/*======================================================================================*/
void CCaptureSnapshotSetup::FinishCreateSelf( void ) {
/* Fill in the blanks, start listening.													*/

	SInt32 i;

	gPreferences.fBackupPreferences();

	/* Get the controls */
	mAutoGroupCheckbox = dynamic_cast<LCheckBoxGroupBox *>(FindPaneByID( 'Anam' ));
	mPathEditText = dynamic_cast<LEditText *>(FindPaneByID( 'Path' ));
	mChoosePathButton = dynamic_cast<LPushButton *>(FindPaneByID( 'Choo' ));
	mNameEditText = dynamic_cast<LEditText *>(FindPaneByID( 'Name' ));
	mSettingsButton = dynamic_cast<LPushButton *>(FindPaneByID( 'Sett' ));
	mOKButton = dynamic_cast<LPushButton *>(FindPaneByID( 'OK  ' ));
	mCancelButton = dynamic_cast<LPushButton *>(FindPaneByID( 'CNCL' ));
	ThrowIf_( mPathEditText == NULL || mChoosePathButton == NULL ||
		mNameEditText == NULL || mSettingsButton == NULL ||
		mOKButton == NULL || mCancelButton == NULL );

	/* Set the destination values */
	if( mAutoGroupCheckbox != NULL ) {
		fGetPreferenceAutomatic( &mAuto );
		mAutoGroupCheckbox->SetValue( mAuto );
		}
	else
		mAuto = 1;
	fGetPreferencesDestination( mPath, mName );
	mPathEditText->SetText( mPath, ::strlen( mPath ) );
	mNameEditText->SetText( mName, ::strlen( mName ) );

	/* Start listening to the controls */
	if( mAutoGroupCheckbox != NULL )
		mAutoGroupCheckbox->AddListener( this );
	mChoosePathButton->AddListener( this );
	mSettingsButton->AddListener( this );
	mOKButton->AddListener( this );
	mCancelButton->AddListener( this );

	Show();
	}

/*======================================================================================*/
void CCaptureSnapshotSetup::fGetPreferenceAutomatic( SInt32 *automatic ) {

	#if kNTZPackageNumber == 'agsp'
	*automatic = 1;
	#else
	*automatic = 0;
	#endif
	gPreferences.fGetPreference( kPrefTagSnapshot, kPrefSnapshotAuto, automatic );
	}

/*======================================================================================*/
void CCaptureSnapshotSetup::fSetPreferenceAutomatic( SInt32 automatic ) {

	gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotAuto, automatic );
	}

/*======================================================================================*/
void CCaptureSnapshotSetup::fGetPreferencesDestination( char *path, char *name ) {

	FSRef ref;
	SInt32 length;

	path[0] = '/';
	path[1] = '0';
	length = PATH_MAX;
	#if kNTZPackageNumber == 'agsp'
	if( ::FSFindFolder( kUserDomain, kDocumentsFolderType, kDontCreateFolder, &ref ) == noErr ) {
		::FSRefMakePath( &ref, (UInt8 *)path, PATH_MAX );
		FSRef newRef;
		HFSUniStr255 fileName;
		CUtilities2::fMakeHFUnicodeName( kNTZPackageName, &fileName );
		::FSCreateDirectoryUnicode( &ref, fileName.length, fileName.unicode,
			kFSCatInfoNone, NULL, &newRef, NULL, NULL );
		::strcat( path, "/" );
		::strcat( path, kNTZPackageName );
		}
	#else
	if( ::FSFindFolder( kUserDomain, kDesktopFolderType, kDontCreateFolder, &ref ) == noErr )
		::FSRefMakePath( &ref, (UInt8 *)path, PATH_MAX );
	#endif
	gPreferences.fGetPreference( kPrefTagSnapshot, kPrefSnapshotPath, path, &length );

	::GetIndString( (StringPtr)name, kStrings, kStringsSnaphot );
	length = name[0];
	::BlockMoveData( &name[1], name, length );
	name[length] = 0;
	length = 256;
	gPreferences.fGetPreference( kPrefTagSnapshot, kPrefSnapshotName, name, &length );
	}

/*======================================================================================*/
void CCaptureSnapshotSetup::ListenToMessage( MessageT inMessage, void *ioParam ) {

	#pragma unused( ioParam )

	FSRef ref;
	FSSpec spec;
	SInt32 dirID;
	SInt32 length;
	Boolean valid;
	SInt32 index;

	switch( inMessage ) {
		case 'Anam':
			if( mAutoGroupCheckbox != NULL )
				mAuto = mAutoGroupCheckbox->GetValue();
			break;
		case 'Choo':
			if( UNavServicesDialogs::AskChooseFolder( spec, dirID ) ) {
				if( ::FSpMakeFSRef( &spec, &ref ) == noErr &&
					::FSRefMakePath( &ref, (UInt8 *)mPath, PATH_MAX ) == noErr )
					mPathEditText->SetText( mPath, ::strlen( mPath ) );
				else
					::SysBeep( 1 );
				mPathEditText->SelectAll();
				}
			break;
		case 'Sett':
			LCommander::GetTopCommander()->ObeyCommand( kCmdSnapshotSettings );
			break;
		case 'OK  ':
			/* Validate */
			valid = true;
			if( mAuto ) {
				if( valid ) {
					Boolean isDirectory;
					mPathEditText->GetText( mPath, PATH_MAX, &length );
					mPath[length] = 0;
					valid = ::FSPathMakeRef( (UInt8 *)mPath, &ref, &isDirectory ) == noErr && isDirectory;
					if( !valid ) {
						::SysBeep( 1 );
						mPathEditText->SelectAll();
						LCommander::SwitchTarget( mPathEditText );
						}
					}
				if( valid ) {
					mNameEditText->GetText( mName, 256, &length );
					mName[length] = 0;
					valid = ::strpbrk( mName, ":/\\" ) == NULL;
					if( !valid ) {
						::SysBeep( 1 );
						mNameEditText->SelectAll();
						LCommander::SwitchTarget( mNameEditText );
						}
					}
				}
			if( valid ) {
				if( mAutoGroupCheckbox != NULL )
					gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotAuto, mAuto );
				gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotPath, mPath, PATH_MAX );
				gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotName, mName, 256 );
				delete this;
				}
			break;
		case 'CNCL':
			gPreferences.fRestorePreferences();
			delete this;
			break;
		}
	}

/*======================================================================================*/
