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

#include "CCaptureWindow.h"
#include "CCaptureSnapshotSettings.h"
#include "CUPreferences.h"
#include "CaptureConstants.h"
#include "CUtilities2.h"

#include "NTZRelease.h"
#include "NTZLogging.h"
//#include "NTZComponents.h"
#include "NTZVDComponents.h"

/*--------------------------------------------------------------------------------------*/
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/*======================================================================================*/
CCaptureSnapshotSettings *CCaptureSnapshotSettings::CreateFromStream( LStream *inStream ) {

	return( new CCaptureSnapshotSettings( inStream ) );
	}

/*======================================================================================*/
CCaptureSnapshotSettings::CCaptureSnapshotSettings( LStream *inStream ) :
	LWindow( inStream ) {

	mExporter = NULL;
	mFormatOptions = NULL;
	mExporters = NULL;
	}

/*======================================================================================*/
CCaptureSnapshotSettings::~CCaptureSnapshotSettings( void ) {

	if( mExporters != NULL )
		::DisposePtr( (Ptr)mExporters );
	if( mFormatOptions != NULL )
		::QTDisposeAtomContainer( mFormatOptions );
	if( mExporter != NULL )
		::CloseComponent( mExporter );
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::FinishCreateSelf( void ) {
/* Fill in the blanks, start listening.													*/

	SInt32 i;

	gPreferences.fBackupPreferences();

	/* Get the controls */
	mFormatMenu = dynamic_cast<LPopupButton *>(FindPaneByID( 'MFrm' ));
	mOptionsStaticText = dynamic_cast<LStaticText *>(FindPaneByID( 'Form' ));
	mChooseOptionsButton = dynamic_cast<LPushButton *>(FindPaneByID( 'CFrm' ));
	mUseQuickTimeDialogCheckBox = dynamic_cast<LCheckBox *>(FindPaneByID( 'UQTd' ));
	mOKButton = dynamic_cast<LPushButton *>(FindPaneByID( 'OK  ' ));
	mCancelButton = dynamic_cast<LPushButton *>(FindPaneByID( 'CNCL' ));
	ThrowIf_( mFormatMenu == NULL || mOptionsStaticText == NULL ||
		mChooseOptionsButton == NULL || mUseQuickTimeDialogCheckBox == NULL ||
		mOKButton == NULL || mCancelButton == NULL );

	/* This checkbox will reenable the QuickTime Save As dialog. Frankly I have no		*/
	/* desire to allow this, and will hide the chackbox unless the option key is down.	*/
	/* That way, if someone complains that they want to save to a different format		*/
	/* when saving manually, they will have the option to do so.						*/
	if( !fOptionPressed() )
		mUseQuickTimeDialogCheckBox->Hide();

	/* Set up the format menu. This is outrageously hard. We need to search the list	*/
	/* of installed graphic exporters. Man.												*/
	mNumExporters = 0;
	fGetExporters( &mExporters, &mNumExporters );
	mFormatMenu->DeleteAllMenuItems();
	Boolean foundPreferredFormat = false;
	for( i = 0; i < mNumExporters; i++ ) {
		mFormatMenu->AppendMenu( mExporters[i].componentName );
		foundPreferredFormat = mExporters[i].componentSubType == mFormat;
		}

	/* Set the preferred format */
	fGetPreferencesExporterFormat( &mFormat );
	fSelectFormat( mFormat, true );

	/* Set the Use QuickTime dialog option */
	fGetPreferenceUseQuickTimeDialog( &mUseQuickTimeDialog );
	mUseQuickTimeDialogCheckBox->SetValue( mUseQuickTimeDialog );

	/* Start listening to the controls */
	mFormatMenu->AddListener( this );
	mChooseOptionsButton->AddListener( this );
	mUseQuickTimeDialogCheckBox->AddListener( this );
	mOKButton->AddListener( this );
	mCancelButton->AddListener( this );

	Show();
	}

/*======================================================================================*/
SInt32 CCaptureSnapshotSettings::fGetExporters( sExporterInfoPtr *exporters, SInt32 *numExporters ) {

	SInt32 i;
	SInt32 j;
	SInt32 theErr = noErr;

	*exporters = NULL;
	*numExporters = 0;

	ComponentDescription cd;
	cd.componentType = GraphicsExporterComponentType;
	cd.componentSubType = 0;
	cd.componentManufacturer = 0;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;

	SInt32 count = 0;
	if( theErr == noErr )
		count = ::CountComponents( &cd );

	if( theErr == noErr ) {
		*exporters = (sExporterInfoPtr)::NewPtr( sizeof( sExporterInfo ) * count );
		if( *exporters == NULL )
			theErr = memFullErr;
		}

	if( theErr == noErr ) {
		Handle name = ::NewHandle( 0 );
		SInt32 index = 0;
		Component component = NULL;
		while( (component = ::FindNextComponent( component, &cd )) != NULL ) {
			ComponentDescription cd;
			if( ::GetComponentInfo( component, &cd, name, NULL, NULL ) == noErr )
				if( cd.componentSubType != kBaseGraphicsExporterSubType ) {
					(*exporters)[*numExporters].componentSubType = cd.componentSubType;
					::BlockMoveData( *name, (*exporters)[*numExporters].componentName, **name + 1 );
					if( cd.componentSubType == 'jp2 ' ) // kQTFileTypeJPEG2000 )
						::BlockMoveData( "\pJPEG 2000", (*exporters)[*numExporters].componentName, 10 );
					*numExporters += 1;
					}
			}
		::DisposeHandle( name );
		}

	/* And sort them for good measure */
	if( theErr == noErr )
		for( i = 0; i < *numExporters - 1; i++ )
			for( j = 0; j < *numExporters - 1 - i; j++ )
				if( ::MacCompareString( (*exporters)[j].componentName,
					(*exporters)[j + 1].componentName, NULL ) > 0 ) {
					sExporterInfo t = (*exporters)[j + 1];
					(*exporters)[j + 1] = (*exporters)[j];
					(*exporters)[j] = t;
					}

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureSnapshotSettings::fFormatToIndex( OSType *format ) {
/* Note that if we don't find the desired format, the value of format will be altered	*/
/* to one that is available.															*/

	SInt32 i;

	SInt32 index = 0;

	/* Look for preferred exporter */
	Boolean found = false;
	for( i = 0; i < mNumExporters && !found; i++ )
		if( *format == mExporters[i].componentSubType ) {
			found = true;
			index = i;
			}

	/* If we can't find the preferred exporter, use JPEG instead */
	if( !found )
		for( i = 0; i < mNumExporters && !found; i++ )
			if( *format == kQTFileTypeJPEG ) {
				found = true;
				index = i;
				*format = kQTFileTypeJPEG;
				}

	/* Total worst case. If nothing found so far, use the first available */
	if( !found ) {
		index = 0;
		*format == mExporters[0].componentSubType;
		}

	return( index );
	}

/*======================================================================================*/
OSType CCaptureSnapshotSettings::fIndexToFormat( SInt32 index  ) {
/* We probably shouldn't assume that kQTFileTypeJPEG is always available, but in		*/
/* reality it will always be available.													*/

	return( mExporters[index].componentSubType );
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fSelectFormat( OSType format, Boolean updateMenu ) {
/* Call this anytime the format menu changes. It updates the menu item (if desired),	*/
/* opens the correct exporter (shutting the old one first if necessary), applies the	*/
/* preferred format options (if any), and updates the static text describing the		*/
/* options.																				*/

	/* Update the menu */
	SInt32 menuValue = fFormatToIndex( &mFormat );
	if( updateMenu )
		mFormatMenu->SetCurrentMenuItem( menuValue + 1 );

	/* Open exporter */
	if( mExporter != NULL )
		::CloseComponent( mExporter );
	mExporter = ::OpenDefaultComponent( GraphicsExporterComponentType, mFormat );

	/* Set the preferred format options */
	if( mFormatOptions != NULL )
		::QTDisposeAtomContainer( mFormatOptions );
	fGetPreferencesExporterOptions( mExporter, mFormat, &mFormatOptions );
	if( mExporter != NULL && mFormatOptions != NULL )
		::GraphicsExportSetSettingsFromAtomContainer( mExporter, mFormatOptions );

	/* Update the static text item */
	fUpdateFormatOptionsText();
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fUpdateFormatOptionsText( void ) {

	if( mExporter != NULL ) {
		Handle theText;
		if( ::GraphicsExportGetSettingsAsText( mExporter, &theText ) == noErr ) {
			::HLock( theText );
			mOptionsStaticText->SetText( *theText, ::GetHandleSize( theText ) );
			::DisposeHandle( theText );
			}
		}
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fSetPreferencesExporterFormat( OSType format ) {

	gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotExporterFormat, (SInt32)format );
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fGetPreferencesExporterFormat( OSType *format ) {

	*format = kQTFileTypeJPEG;
	gPreferences.fGetPreference( kPrefTagSnapshot, kPrefSnapshotExporterFormat, (SInt32 *)format );
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fSetPreferencesExporterOptions( OSType format, QTAtomContainer formatOptions ) {

	if( formatOptions != NULL ) {
		::QTLockContainer( formatOptions );
		SInt32 length = ::GetHandleSize( (Handle)formatOptions );
		Str31 name;
		fSInt32ToString( format, name );
		gPreferences.fSetPreference( kPrefTagExporterFormatOptions, name, *formatOptions, length );
		::QTUnlockContainer( formatOptions );
		}
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fGetPreferencesExporterOptions( GraphicsExportComponent exporter,
	OSType format, QTAtomContainer *formatOptions ) {

	SInt32 length;

	Str31 name;
	fSInt32ToString( format, name );
	if( gPreferences.fGetPreferenceDataSize( kPrefTagExporterFormatOptions, name, &length ) == noErr ) {
		*formatOptions = (QTAtomContainer)::NewHandle( length );
		if( *formatOptions != NULL ) {
			::HLock( (Handle)*formatOptions );
			gPreferences.fGetPreference( kPrefTagExporterFormatOptions, name, **formatOptions, &length );
			::HUnlock( (Handle)*formatOptions );
			}
		}
	else
		if( exporter != NULL ) {
			::GraphicsExportGetSettingsAsAtomContainer( exporter, formatOptions );
			::QTLockContainer( *formatOptions );
			length = ::GetHandleSize( (Handle)*formatOptions );
			gPreferences.fSetPreference( kPrefTagExporterFormatOptions, name, **formatOptions, length );
			::QTUnlockContainer( *formatOptions );
			}
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fSetPreferenceUseQuickTimeDialog( SInt32 useQuickTimeDialog ) {

	gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotUseQuickTimeDialog, useQuickTimeDialog );
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::fGetPreferenceUseQuickTimeDialog( SInt32 *useQuickTimeDialog ) {

	*useQuickTimeDialog = 0;
	gPreferences.fGetPreference( kPrefTagSnapshot, kPrefSnapshotUseQuickTimeDialog, (SInt32 *)useQuickTimeDialog );
	}

///*======================================================================================*/
//void CCaptureSnapshotSettings::fSetPreferencesImporterFormat( OSType format ) {
//
//	gPreferences.fSetPreference( kPrefTagSnapshot, kPrefSnapshotImporterFormat, (SInt32)format );
//	}
//
///*======================================================================================*/
//void CCaptureSnapshotSettings::fGetPreferencesImporterFormat( OSType *format ) {
//
//	*format = kQTFileTypePicture;
//	gPreferences.fGetPreference( kPrefTagSnapshot, kPrefSnapshotImporterFormat, (SInt32 *)format );
//	}
//
///*======================================================================================*/
//void CCaptureSnapshotSettings::fSetPreferencesImporterOptions( OSType format, QTAtomContainer formatOptions ) {
//
//	if( formatOptions != NULL ) {
//		::QTLockContainer( formatOptions );
//		SInt32 length = ::GetHandleSize( (Handle)formatOptions );
//		Str31 name;
//		fSInt32ToString( format, name );
//		gPreferences.fSetPreference( kPrefTagImporterFormatOptions, name, *formatOptions, length );
//		::QTUnlockContainer( formatOptions );
//		}
//	}
//
///*======================================================================================*/
//void CCaptureSnapshotSettings::fGetPreferencesImporterOptions( GraphicsImportComponent importer,
//	OSType format, QTAtomContainer *formatOptions ) {
//
//	SInt32 length;
//
//	Str31 name;
//	fSInt32ToString( format, name );
//	if( gPreferences.fGetPreferenceDataSize( kPrefTagImporterFormatOptions, name, &length ) == noErr ) {
//		*formatOptions = (QTAtomContainer)::NewHandle( length );
//		if( *formatOptions != NULL ) {
//			::HLock( (Handle)*formatOptions );
//			gPreferences.fGetPreference( kPrefTagImporterFormatOptions, name, **formatOptions, &length );
//			::HUnlock( (Handle)*formatOptions );
//			}
//		}
//	else
//		if( importer != NULL ) {
//			::GraphicsImportGetExportSettingsAsAtomContainer( importer, formatOptions );
//			::QTLockContainer( *formatOptions );
//			length = ::GetHandleSize( (Handle)*formatOptions );
//			gPreferences.fSetPreference( kPrefTagImporterFormatOptions, name, **formatOptions, length );
//			::QTUnlockContainer( *formatOptions );
//			}
//	}
//
/*======================================================================================*/
StringPtr CCaptureSnapshotSettings::fSInt32ToString( UInt32 value, Str31 text ) {

	*(UInt32 *)&text[1] = EndianU32_NtoB( value );
	text[0] = 4;

	return( text );
	}

/*======================================================================================*/
void CCaptureSnapshotSettings::ListenToMessage( MessageT inMessage, void *ioParam ) {

	#pragma unused( ioParam )

	FSRef ref;
	FSSpec spec;
	SInt32 dirID;
	SInt32 length;
	Boolean valid;
	SInt32 index;

	switch( inMessage ) {
		case 'MFrm':
			index = mFormatMenu->GetCurrentMenuItem() - 1;
			mFormat = fIndexToFormat( index );
			fSelectFormat( mFormat );
			break;
		case 'CFrm':
			if( mExporter != NULL )
				if( ::GraphicsExportRequestSettings( mExporter, NULL, NULL ) == noErr ) {
					if( mFormatOptions != NULL )
						::QTDisposeAtomContainer( mFormatOptions );
					::GraphicsExportGetSettingsAsAtomContainer( mExporter, &mFormatOptions );
					fUpdateFormatOptionsText();
					}
			break;
		case 'UQTd':
			mUseQuickTimeDialog = mUseQuickTimeDialogCheckBox->GetValue() ? 1 : 0;
			break;
		case 'OK  ':
			/* Validate */
			valid = true;
			if( valid ) {
				fSetPreferencesExporterFormat( mFormat );
				fSetPreferencesExporterOptions( mFormat, mFormatOptions );
				fSetPreferenceUseQuickTimeDialog( mUseQuickTimeDialog );
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
