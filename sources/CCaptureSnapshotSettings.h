/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#pragma once
#ifndef CCaptureSnapshotSettings_h
#define CCaptureSnapshotSettings_h

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
#include <LCheckBox.h>

typedef struct {
	OSType componentSubType;
	Str255 componentName;
	} sExporterInfo, *sExporterInfoPtr;

/*--------------------------------------------------------------------------------------*/
class CCaptureSnapshotSettings :
	public LWindow,
	public LListener {

	public:

		enum { class_ID = 'SSst' };
		static CCaptureSnapshotSettings *CreateFromStream( LStream *inStream );
		CCaptureSnapshotSettings( LStream *inStream );
		virtual ~CCaptureSnapshotSettings( void );

		static void fSetPreferencesExporterFormat( OSType format );
		static void fGetPreferencesExporterFormat( OSType *format );
		static void fSetPreferencesExporterOptions( OSType format, QTAtomContainer formatOptions );
		static void fGetPreferencesExporterOptions( GraphicsExportComponent exporter, OSType format, QTAtomContainer *format );

		static void fSetPreferenceUseQuickTimeDialog( SInt32 useQuickTimeDialog );
		static void fGetPreferenceUseQuickTimeDialog( SInt32 *useQuickTimeDialog );

		//static void fSetPreferencesImporterFormat( OSType format );
		//static void fGetPreferencesImporterFormat( OSType *format );
		//static void fSetPreferencesImporterOptions( OSType format, QTAtomContainer formatOptions );
		//static void fGetPreferencesImporterOptions( GraphicsImportComponent importer, OSType format, QTAtomContainer *formatOptions );

		static StringPtr fSInt32ToString( UInt32 value, Str31 text );

		virtual	void FinishCreateSelf( void );
		virtual void ListenToMessage( MessageT inMessage, void *ioParam );

	private:

		SInt32 fGetExporters( sExporterInfoPtr *exporters, SInt32 *numExporters );
		SInt32 fFormatToIndex( OSType *format );
		OSType fIndexToFormat( SInt32 index );
		void fSelectFormat( OSType format, Boolean updateMenu = false );
		void fUpdateFormatOptionsText( void );

		GraphicsExportComponent mExporter;

		OSType mFormat;
		QTAtomContainer mFormatOptions;
		SInt32 mUseQuickTimeDialog;

		LPopupButton *mFormatMenu;
		LStaticText *mOptionsStaticText;
		LPushButton *mChooseOptionsButton;
		LCheckBox *mUseQuickTimeDialogCheckBox;
		LPushButton *mOKButton;
		LPushButton *mCancelButton;
		SInt32 mNumExporters;
		sExporterInfoPtr mExporters;

	};

#endif

