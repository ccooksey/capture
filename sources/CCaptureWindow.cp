/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <UStandardDialogs.h>
#include <LStaticText.h>
#include <LBevelButton.h>
#include <LCheckBox.h>
#include <UDrawingState.h>

#include "CCaptureWindow.h"
#include "CCaptureApplication.h"
#include "CClickWindow.h"
#include "CUPreferences.h"
#include "CaptureConstants.h"
#include "CCaptureSnapshotSettings.h"
#include "CCaptureSnapshotSetup.h"
#include "CCaptureRecordSetup.h"
#include "CUtilities.h"
#include "CUtilities2.h"

#include "NTZRelease.h"
#include "NTZLogging.h"
#include "NTZVDComponents.h"
#if kNTZLicenseeNumber != 'eEm '
#include "NTZSnapshot.h"
#endif

/*--------------------------------------------------------------------------------------*/
#define kChannelUsageFlagsPreviewVideo		(seqGrabPreview | seqGrabAlwaysUseTimeBase)
#define kChannelUsageFlagsPreviewAudio		(seqGrabPreview | seqGrabAlwaysUseTimeBase)
#define kChannelUsageFlagsRecordVideo		(seqGrabRecord | seqGrabAlwaysUseTimeBase | seqGrabPlayDuringRecord)
#define kChannelUsageFlagsRecordAudio		(seqGrabRecord | seqGrabAlwaysUseTimeBase)
#define kSeqGrabDontPreAllocateFileSize		512
#define kTimeLapseTimeScale					30000

/*--------------------------------------------------------------------------------------*/
typedef struct {
	Ptr data;
	long size;
	} sSample;

/*--------------------------------------------------------------------------------------*/
static Boolean fKeyIsDown( short keyCode );

/*--------------------------------------------------------------------------------------*/
SndChannelPtr gClickSoundChannel = NULL;
Handle gClickSound = NULL;

/*======================================================================================*/
CCaptureWindow *CCaptureWindow::CreateFromStream( LStream *inStream ) {

	return( new CCaptureWindow( inStream ) );
	}

/*======================================================================================*/
CCaptureWindow::CCaptureWindow( LStream *inStream ) :
	LWindow( inStream ) {

	mSequenceGrabber = NULL;

//	mDisplayFrameRate = false;
//	mLetterBox = false;
//	mStartScan = 110;
//	mNumScans = 248;
//	mStartColumn = 40;
//	mNumColumns = 566;
//	mTickCount = 0;
//	mFrameCount = 0;
//	mFramesPerSecond = 0;
//	mAdjustTick = 0;

	mInPosition = false;

	/* Get and apply the window geometric preferences. These will almost certainly be	*/
	/* overridden by VDIG active rect bounds, which in turn will be overriden by any	*/
	/* preferred values.																*/
	mSelectedSize = kCmd320x240;
	mRegularSize = kCmd320x240;
	mCustomWidth = 320;
	mCustomHeight = 240;

	mRecordVideo = true;
	mRecordSound = true;
	mSoundActive = false;

	mAspectRatioWarning = false;

	mFullScreen = false;

	mSystemVersion = 0;
	if( ::Gestalt( gestaltSystemVersion, &mSystemVersion ) != noErr )
		mSystemVersion = 0;

	/* fStart/StopRecording() support */
	mRecordErr = noErr;
	mIsRecording = false;
	mDuration = 0;
	mStartTime = 0;
	mNextDurationCheck = 0;
	mPreviewStopped = false;
	::memset( &mRecordSpec, 0, sizeof( FSSpec ) );
	::memset( &mRecordSpecNoIndex, 0, sizeof( FSSpec ) );
	mRecordSpecDirID = 0;
	mUsageRecording = false;
	mSuppressUI = false;
	mClickWindow = NULL;
	mClickView = NULL;
	mTimePanel = NULL;
	mIsTimeLapse = false;
	::memset( &mTimeLapseData, 0, sizeof( sTimeLapseData ) );
	mTimeLapseNextTime = 0;
	mTimeLapseInterval = 0;
	mMovieResRefNum = 0;
	mMovie = 0;
	mTrack = 0;
	mMedia = 0;
	mImageDescription = NULL;
	mTLDecodeDurationPerSample = 0;
	mTimeLapseKeyFrameRateChanged = false;
	mTLSaveKeyFrameRate = 0;
	mTLExporter = NULL;

	mPreSuspensionPreviewSoundUsage = -1;
	mSnapshotButtonPressed = false;

	#if kNTZPackageNumber == '3Cam'
	mLCTLastCopyBits = 0;
	mLCTLastStatusUpdate = 0;
	#endif

	/* Get QuickTimeVersion */
	mQuickTimeVersion = 0x06000000;
	::Gestalt( gestaltQuickTimeVersion, &mQuickTimeVersion );
	}

/*======================================================================================*/
CCaptureWindow::~CCaptureWindow( void ) {

	if( mIsRecording )
		fStopRecording();

	if( mFullScreen ) {
		fEndFullScreen();
		fResizeWindow( mRegularSize );
		}

	/* Audio has to be stopped, or else SGDisposeChannel() can crash in the				*/
	/* Sound Manager.																	*/
	if( mSequenceGrabber != NULL ) {
		::SGPause( mSequenceGrabber, seqGrabPause );
		::SGStop( mSequenceGrabber );
		}

	/* OS 9 only */
	if( mSoundDeviceChanged ) {
		SInt32 refNum;
		if( ::SPBOpenDevice( "\pBuilt-in", siWritePermission, &refNum ) == noErr ) {
			::SPBSetDeviceInfo( refNum, siOSTypeInputSource, &mSoundDevice );
			::SPBCloseDevice( refNum );
			}
		}

	if( mVideoChannel != NULL )
		::SGDisposeChannel( mSequenceGrabber, mVideoChannel );
	if( mSoundChannel != NULL )
		::SGDisposeChannel( mSequenceGrabber, mSoundChannel );
	if( mSequenceGrabber != NULL )
		::CloseComponent( mSequenceGrabber );

	/* PowerPlant changes the window position in too many different ways to override	*/
	/* so we will just get it at quit.													*/
	if( mInPosition ) {
		Point topLeft;
		::SetPt( &topLeft, 0, 0 );
		PortToGlobalPoint( topLeft );
		gPreferences.fSetPreference( kPrefTagLocation, mPreferenceID, *(SInt32 *)&topLeft );
		}
	}

/*======================================================================================*/
#if kNTZPackageNumber == '3Cam'
void CCaptureWindow::FinishCreateSelf( void ) {

	mLCTGWorld = NULL;

	if( g3CamRunMode == k3CamRunModeFullScreen ) {
		mLCTQuitButton = dynamic_cast<LBevelButton *>( FindPaneByID( 'Quit' ) );
		mLCTSnapButton = dynamic_cast<LBevelButton *>( FindPaneByID( 'Snap' ) );
		mLCTQuitButton->AddListener( this );
		mLCTSnapButton->AddListener( this );
		}

	if( g3CamRunMode == k3CamRunModeVideoRecorder ) {
		mLCTRecordButton = dynamic_cast<LBevelButton *>( FindPaneByID( 'Reco' ) );
		mLCTStopButton = dynamic_cast<LBevelButton *>( FindPaneByID( 'Stop' ) );
		mLCTToolsButton = dynamic_cast<LBevelButton *>( FindPaneByID( 'Tool' ) );
		mLCTRecordButton->AddListener( this );
		mLCTStopButton->AddListener( this );
		mLCTToolsButton->AddListener( this );
		mLCTStopButton->Disable();
		mLCTVRStatusText = dynamic_cast<LStaticText *>( FindPaneByID( 'Stat' ) );
		mLCTVRStatusText->SetDescriptor( "\pStopped" );
		}
	}

#endif
/*======================================================================================*/
void CCaptureWindow::HandleAppleEvent( const AppleEvent &inAppleEvent, AppleEvent &outAEReply,
	AEDesc &outResult, long inAENumber ) {
/* The application will have opened the digitizer if it wasn't already. Just make sure	*/
/* the open event doesn't throw, and make sure the window is selected.					*/

	switch( inAENumber ) {

		case ae_Open:
			UDesktop::SelectDeskWindow( this );
			break;

		default:
			LWindow::HandleAppleEvent( inAppleEvent, outAEReply, outResult, inAENumber );
			break;
		}
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fNewDeviceListCopy( sDeviceListPtr *list, Boolean *serialized ) {

	SInt32 i;
	SInt32 theErr = noErr;

	*list = NULL;

	/* First unregister all the "undesirable" components, in a prepass. Ignore			*/
	/* any errors.																		*/
	if( theErr == noErr )
		fUnregisterUndesirables();

	ComponentDescription cd;
	if( theErr == noErr ) {
		cd.componentType = 'vdig';
		cd.componentSubType = 0;
		cd.componentManufacturer = 0;
		cd.componentFlags = 0;
		cd.componentFlagsMask = 0;
		}

	SInt32 count = 0;
	if( theErr == noErr )
		count = ::CountComponents( &cd );

	if( theErr == noErr ) {
		*list = (sDeviceListPtr)::NewPtrClear( sizeof( sDeviceList ) + (count - 1) * sizeof( sDevice ) );
		if( *list == NULL )
			theErr = memFullErr;
		}

	*serialized = false;
	if( theErr == noErr ) {
		Handle h = ::NewHandle( 0 );
		(*list)->count = 0;
		Component component = NULL;
		while( (component = ::FindNextComponent( component, &cd )) != NULL ) {
	     	ComponentInstance componentInstance;
			SInt32 anErr;

			#if APP_DIAGNOSTICS
			#warning "KAV diagnostics"
			::printf( "-------------------------------\n" );
			::printf( "Found a vdig component.\n" );
			ComponentDescription cdi;
			anErr = ::GetComponentInfo( component, &cdi, h, NULL, NULL );
			if( anErr == noErr )
				::printf( "Got component info: subType = %c%c%c%c, manufacturer = %c%c%c%c\n",
					(cdi.componentSubType >> 24) & 0xFF,
					(cdi.componentSubType >> 16) & 0xFF,
					(cdi.componentSubType >> 8) & 0xFF,
					(cdi.componentSubType >> 0) & 0xFF,
					(cdi.componentManufacturer >> 24) & 0xFF,
					(cdi.componentManufacturer >> 16) & 0xFF,
					(cdi.componentManufacturer >> 8) & 0xFF,
					(cdi.componentManufacturer >> 0) & 0xFF );
			else
				::printf( "Failed to GetComponentInfo: %d\n", anErr );
			::printf( "Attempting to open component\n" );
			#endif

			anErr = ::OpenAComponent( component, &componentInstance );
	     	if( anErr == noErr ) {
				ComponentDescription cd;
				anErr = ::GetComponentInfo( component, &cd, h, NULL, NULL );
				if( anErr == noErr ) {
					if( **h > 0 ) {
						sDevicePtr device = &(*list)->devices[(*list)->count];
						device->component = component;
						device->subType = cd.componentSubType;
						device->manufacturer = cd.componentManufacturer;
						if( **h > 63 )
							**h = 63;
						::memcpy( device->name, *h, **h + 1 );
						::memset( &device->name[**h + 1], 0, 63 - **h );
						(*list)->count++;

						/* See if the vdig likes to promote it's inputs to devices.		*/
						/* Apple's USB Video Class Video vdig does this.				*/
						/* If so, make a note of the input names. Note that this won't	*/
						/* cause VideoGlide to fully initialize because					*/
						/* VDGetDeviceNameAndFlags() will return an error.				*/
						device->inputCount = 0;
						Str255 deviceName;
						UInt32 flags;
						if( ::VDGetDeviceNameAndFlags( componentInstance, deviceName, &flags ) == noErr )
							if( (flags & vdDeviceFlagShowInputsAsDevices) != 0 ) {
								SInt16 inputCount;
								if( ::VDGetNumberOfInputs( componentInstance, &inputCount ) == noErr ) {
									Boolean protocolFailure = false;
									inputCount++;
									if( inputCount > kMaxInputNames )
										inputCount = kMaxInputNames;
									for( i = 0; i < inputCount && !protocolFailure; i++ ) {
										Str255 inputName;
										if( ::VDGetInputName( componentInstance, i, inputName ) == noErr )
											CUtilities::fPStrCopy( device->inputNames[i], inputName );
										else
											protocolFailure = true;
										}
									if( !protocolFailure )
										device->inputCount = inputCount;
									}
								}

						/* If USBVision device, see if it is InterView Lite or			*/
						/* serialized.													*/
						if( !*serialized ) {
							OSType baseManufacturer = (cd.componentManufacturer & 0xFFFFFF00) + ' ';
							if( baseManufacturer == 'efx ' ) {
								sSerializationInfo info;
								if( ::fNTZVDGetSerializationInfo( componentInstance, &info ) == noErr )
									*serialized = info.interviewLite || info.serialized;
								}
							}
						}

					#if APP_DIAGNOSTICS
					#warning "KAV diagnostics"
					else {
						::SetHandleSize( h, 256 );
						::BlockMoveData( "\pDevice unnamed -will not be presented!", *h, 25 );
						}

					char buffer[512];
					::memcpy( buffer, &(*h)[1], (*h)[0] );
					buffer[(*h)[0]] = 0;
					::printf( "Device name: %s, subType = %c%c%c%c, manufacturer = %c%c%c%c\n",
						buffer,
						(cd.componentSubType >> 24) & 0xFF,
						(cd.componentSubType >> 16) & 0xFF,
						(cd.componentSubType >> 8) & 0xFF,
						(cd.componentSubType >> 0) & 0xFF,
						(cd.componentManufacturer >> 24) & 0xFF,
						(cd.componentManufacturer >> 16) & 0xFF,
						(cd.componentManufacturer >> 8) & 0xFF,
						(cd.componentManufacturer >> 0) & 0xFF );
					#endif

					}

				#if APP_DIAGNOSTICS
				else
					::printf( "Failed to GetComponentInfo: %d\n", anErr );
				#endif

	     		::CloseComponent( componentInstance );
				}

			#if APP_DIAGNOSTICS
			else
				::printf( "Failed to OpenAComponent: %d\n", anErr );
			#endif

			}

		#if APP_DIAGNOSTICS
		::printf( "-------------------------------\n" );
		#endif

		::DisposeHandle( h );
		}

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fDisposeDeviceListCopy( sDeviceListPtr list ) {

	SInt32 theErr = noErr;

	::DisposePtr( (Ptr)list );

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fUnregisterUndesirables( void ) {
/* Unregister components that we want nothing to do with. We must unregister it			*/
/* completely otherwise QuickTime may attempt to use it while we aren't looking.		*/
/* This is the identity of the IOXpert driver for the Proscope microscope included in	*/
/* Apple's "science bundle". Anything prior to 1.0.1 hangs the capture application in	*/
/* OpenAComponent():																	*/
/*		name			Scalar USB Microscope											*/
/*		subType			Divi															*/
/*		manufacturer	Scal															*/
/* This is the identity of Apple's USB Video Class Video Component:						*/
/*		name			USB Video Class Video											*/
/*		subType			usbv															*/
/*		manufacturer	appl															*/

	SInt32 i;
	SInt32 theErr = noErr;
	sDevicePtr device;

	ComponentDescription cd;
	cd.componentType = 'vdig';
	cd.componentSubType = 0;
	cd.componentManufacturer = 0;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;

	/* Count all the registered vdigs in the system */
	SInt32 count = 0;
	if( theErr == noErr )
		count = ::CountComponents( &cd );

	/* Allocate enough space to record references to them all */
	sDeviceListPtr list = NULL;
	if( theErr == noErr ) {
		list = (sDeviceListPtr)::NewPtr( sizeof( sDeviceList ) + (count - 1) * sizeof( sDevice ) );
		if( list == NULL )
			theErr = memFullErr;
		}

	/* Make a list of all the undesirables */
	if( theErr == noErr ) {
		list->count = 0;
		Component component = NULL;
		while( (component = ::FindNextComponent( component, &cd )) != NULL ) {
			ComponentDescription cd;
			if( ::GetComponentInfo( component, &cd, NULL, NULL, NULL ) == noErr )
				if( cd.componentSubType == 'Divi' && cd.componentManufacturer == 'Scal' ) {
					FSRef folder;
					if( ::FSFindFolder( kLocalDomain, kComponentsFolderType,
						kDontCreateFolder, &folder ) == noErr ) {
						CFURLRef bundleURL = ::CFURLCreateFromFSRef( NULL, &folder );
						if( bundleURL != NULL ) {
							CFURLRef folderURL = ::CFURLCreateCopyAppendingPathComponent( NULL,
								bundleURL, CFSTR( "Scalar USB Microscope.component" ), true );
							if( folderURL != NULL ) {
								CFBundleRef bundle = ::CFBundleCreate( NULL, folderURL );
								if( bundle != NULL ) {
									if( ::CFBundleGetVersionNumber( bundle ) < 0x01018000 ) {
										device = &list->devices[list->count];
										device->component = component;
										list->count++;
										}
									::CFRelease( bundle );
									}
								::CFRelease( folderURL );
								}
							::CFRelease( bundleURL );
							}
						}
					}
			}
		}

	/* Remove undesirables in a post-pass, since UnregisterComponent() messes up the	*/
	/* list iteration.																	*/
	if( theErr == noErr )
		for( i = 0; i < list->count; i++ ) {
			device = &list->devices[i];
			::UnregisterComponent( device->component );
			}

	if( list != NULL )
		::DisposePtr( (Ptr)list );

	return( theErr );
	}

/*======================================================================================*/
Boolean CCaptureWindow::fGetPreferenceID( sDevicePtr device, SInt32 input, SInt32 *preferenceID ) {
/* Given a device id, locate the id used to store information for this device.			*/
/* If the device is not present, create a new preference.								*/

	/* This is a program error if this occurs */
	if( device->inputCount > 0 )
		if( input < 0 || input >= device->inputCount )
			return( false );

	sDeviceTriple triple;

	Boolean found = false;
	Boolean finished = false;
	SInt32 nextID = kPrefDataStartID;
	while( !found && !finished ) {

		/* Look for our shadow resource that contains a triple of device				*/
		/* identifying information. If no resource exists with the specified id, we		*/
		/* will create one after the search completes.									*/
		::memset( (void *)&triple, 0, sizeof( sDeviceTriple ) );
		if( !finished ) {
			SInt32 length;
			if( gPreferences.fGetPreferenceDataSize( kPrefTagChannelVDIGTripleData,
				nextID, &length ) != noErr )
				finished = true;
			else {
				length = sizeof( sDeviceTriple );
				finished = gPreferences.fGetPreference( kPrefTagChannelVDIGTripleData,
					nextID, (char *)&triple, &length ) != noErr;
				}
			}

		/* If we found a preference at the current id, see if it is for the device		*/
		/* and input passed in. If so, set the found variable. Note that devices that	*/
		/* promote inputs to device level will store a separate preference for every	*/
		/* device, and will store the specific inputName in the 0 slot so we can		*/
		/* find it.																		*/
		if( !finished ) {
			found = triple.subType == device->subType &&
				triple.manufacturer == device->manufacturer &&
				::memcmp( triple.name, device->name, triple.name[0] + 1 ) == 0;
			if( found )
				if( device->inputCount > 0 )
					found = ::memcmp( triple.inputName, device->inputNames[input],
						triple.inputName[0] + 1 ) == 0;
			}

		/* If not finished and not found, move to the next ID in sequence */
		if( !finished )
			if( !found )
				nextID++;
		}

	/* If not found, add the triple for the desired device / input */
	if( !found ) {
		::memset( (void *)&triple, 0, sizeof( sDeviceTriple ) );
		triple.subType = device->subType;
		triple.manufacturer = device->manufacturer;
		CUtilities::fPStrCopy( triple.name, device->name );
		if( device->inputCount > 0 )
			CUtilities::fPStrCopy( triple.inputName, device->inputNames[input] );
		found = gPreferences.fSetPreference( kPrefTagChannelVDIGTripleData, nextID,
			(char *)&triple, sizeof( sDeviceTriple ) ) == noErr;
		}

	if( found )
		*preferenceID = nextID;

	return( found );
	}

/*======================================================================================*/
OSStatus CCaptureWindow::fStartSequencer( SInt32 index, sDevicePtr device, SInt32 input ) {
/* Get the sequencer fully running.	This is called straight after the window is			*/
/* created. The window will be made visible in here.									*/
/* We are going to do most of our error prone initialization in here rather than in		*/
/* the constructor so that we can report the error.										*/

	SInt32 theErr = noErr;
	SInt32 result = noErr;
	ComponentDescription cd;
	Str255 deviceName;

	FocusDraw();

	mDevice = *device;
	mInput = input;

	/* Get the preference ID used to access information about this specific device.		*/
	/* Make a note of whether we have seen this device or not. If it's new, we'll		*/
	/* do some preconfiguring later on when more things are known.						*/
	Boolean newDevice = false;
	if( theErr == noErr )
		if( !fGetPreferenceID( &mDevice, mInput, &mPreferenceID ) )
			theErr = -1;
	if( theErr == noErr ) {
		SInt32 length;
		newDevice = gPreferences.fGetPreferenceDataSize( kPrefTagLocation,
			mPreferenceID, &length ) != noErr;
		}
	if( result == noErr && theErr != noErr ) {
		result = 1;
		fReportStartSequencerError( "kCaptureOpenErrorMessage1", theErr );
		}

	/* Find and open a sequence grabber */
	mSequenceGrabber = NULL;
	Component sequenceGrabber;
	if( theErr == noErr ) {
		cd.componentType = SeqGrabComponentType;
		cd.componentSubType = 0;
		cd.componentManufacturer = 'appl';
		cd.componentFlags = 0;
		cd.componentFlagsMask = 0;
		sequenceGrabber = ::FindNextComponent( NULL, &cd );
		if( sequenceGrabber == 0 )
			theErr = -2;
		}
	if( theErr == noErr ) {
		mSequenceGrabber = ::OpenComponent( sequenceGrabber );
		if( mSequenceGrabber == NULL )
			theErr = -3;
		}
	if( theErr == noErr )
		theErr = ::SGInitialize( mSequenceGrabber );
	if( theErr == noErr )
		theErr = ::SGSetGWorld( mSequenceGrabber, (CGrafPtr)GetMacPort(), NULL );
	if( result == noErr && theErr != noErr ) {
		result = 3;
		fReportStartSequencerError( "kCaptureOpenErrorMessage3", theErr );
		}

	/* Open video channel */
	mVideoChannel = NULL;
	if( theErr == noErr )
		theErr = ::SGNewChannel( mSequenceGrabber, VideoMediaType, &mVideoChannel );
	if( result == noErr && theErr != noErr ) {
		result = 4;
		fReportStartSequencerError( "kCaptureOpenErrorMessage4", theErr );
		}

	/* Open the sequence grabber panel component and restore the preferences it saved */
	if( theErr == noErr )
		fLoadSettings( kPrefTagVideoChannelData, mVideoChannel );

	/* fLoadSettings() has the potential to change both the device and the input		*/
	/* associated with the video channel. We don't allow that -change them back. Note	*/
	/* that the Sequence Grabber probably selected the wrong device as well when we		*/
	/* created the channel.																*/
	if( theErr == noErr )
		if( ::SGGetChannelDeviceAndInputNames( mVideoChannel, deviceName, NULL, NULL ) == noErr )
			if( device->inputCount == 0 ) {
				if( ::memcmp( mDevice.name, deviceName, deviceName[0] + 1 ) != 0 )
					theErr = ::SGSetChannelDevice( mVideoChannel, mDevice.name );
				}
			else {
				if( ::memcmp( mDevice.inputNames[mInput], deviceName, deviceName[0] + 1 ) != 0 ) {
					if( theErr == noErr )
						theErr = ::SGSetChannelDevice( mVideoChannel, mDevice.name );
					if( theErr == noErr )
						theErr = ::SGSetChannelDeviceInput( mVideoChannel, mInput );
					}
				}
	if( result == noErr && theErr != noErr ) {
		result = 9;
		fReportStartSequencerError( "kCaptureOpenErrorMessage9", theErr );
		}

	/* Get the active video frame size */
	if( theErr == noErr ) {
		Rect rect;
		if( ::SGGetSrcVideoBounds( mVideoChannel, &rect ) == noErr ) {
			OffsetRect( &rect, -rect.left, -rect.top );
			SInt32 size = fGetBestSize( &rect );
			mWidth = rect.right;
			mHeight = rect.bottom;
			mSelectedSize = size;
			mRegularSize = size;
			mCustomWidth = rect.right - rect.left;
			mCustomHeight = rect.bottom - rect.top;
			}
		}

	/* We have enough info now to set the window name, position and initial size. If	*/
	/* window is offscreen, move it back on.											*/
	if( theErr == noErr ) {
		Point topLeft;
		::SetPt( &topLeft, 20 + index * 20, 50 + index * 20 );
		gPreferences.fGetPreference( kPrefTagLocation, mPreferenceID, (SInt32 *)&topLeft );
		Boolean onScreen = false;
		GDHandle device = ::GetDeviceList();
		while( device != NULL && !onScreen ) {
			Rect rect;
			if( ::GetAvailableWindowPositioningBounds( device, &rect ) == noErr )
				onScreen = ::PtInRect( topLeft, &rect );
			device = ::GetNextDevice( device );
			}
		if( !onScreen )
			::SetPt( &topLeft, 20 + index * 20, 50 + index * 20 );
		gPreferences.fGetPreference( kPrefTagRegularSize, mPreferenceID, &mRegularSize );
		gPreferences.fGetPreference( kPrefTagCustomWidth, mPreferenceID, &mCustomWidth );
		gPreferences.fGetPreference( kPrefTagCustomHeight, mPreferenceID, &mCustomHeight );
		gPreferences.fGetPreference( kPrefTagSelectedSize, mPreferenceID, &mSelectedSize );
		if( mSelectedSize == kCmdFullScreen ||
			mSelectedSize == kCmdFullScreen16x9 )
			mSelectedSize = mRegularSize;
		if( mDevice.inputCount == 0 )
			SetDescriptor( mDevice.name );
		else
			SetDescriptor( mDevice.inputNames[mInput] );
		DoSetPosition( topLeft );
		mInPosition = true;
		fResizeWindow( mSelectedSize );
		}

	/* Set the digitizer usage: preview regular */
	if( theErr == noErr )
		theErr = ::SGSetChannelUsage( mVideoChannel, kChannelUsageFlagsPreviewVideo );
	if( result == noErr && theErr != noErr ) {
		result = 5;
		fReportStartSequencerError( "kCaptureOpenErrorMessage5", theErr );
		}

	/* Change the sound input selection to "Built-in". OS 9 only.						*/
	/* Note that this isn't a QuickTime thing. We are telling the machine to			*/
	/* temporarily take input from the mic jack instead of the CD or the internal		*/
	/* modem for example. This stops the user from having to fumble around like an		*/
	/* idiot every time they launch this app.											*/
	mSoundDeviceChanged = false;
	if( theErr == noErr ) {
		SInt32 systemVersion;
		if( ::Gestalt( gestaltSystemVersion, &systemVersion ) == noErr && systemVersion < 0x00001000 ) {
			SInt32 refNum;
			if( ::SPBOpenDevice( "\pBuilt-in", siWritePermission, &refNum ) == noErr ) {
				if( ::SPBGetDeviceInfo( refNum, siOSTypeInputSource, &mSoundDevice ) == noErr ) {
					OSType source = 'sinj';
					if( ::SPBSetDeviceInfo( refNum, siOSTypeInputSource, &source ) == noErr )
						mSoundDeviceChanged = true;
					}
				::SPBCloseDevice( refNum );
				}
			}
		}

	/* Open a sound channel and set it to some useful defaults. Note some machines		*/
	/* have absolutely no built-in sound input capabilities. mSoundChannel will be		*/
	/* NULL in that case. All errors from this process are ignored.						*/
	/* We don't use the sound channel until the window is activated. However, if this	*/
	/* this is the first time the device is opened, we must set the usage value to		*/
	/* what we want to save in the prefs, i.e. seqGrabPreview.							*/
	mSoundChannel = NULL;
	if( theErr == noErr ) {
		SInt32 anErr = noErr;
		if( anErr == noErr ) {
			anErr = ::SGNewChannel( mSequenceGrabber, SoundMediaType, &mSoundChannel );
			if( anErr != noErr && anErr != couldntGetRequiredComponent )
				fReportSoundError( "\pSGNewChannel", anErr );
			}
		/* This is necessary in Mac OS 10.5 otherwise you always get siDeviceBusyErr	*/
		/* when you try to start the preview, no matter how long you wait.				*/
		if( anErr == noErr )
			fSetSoundInputDriver( "\p" );
		if( anErr == noErr ) {
			if( ::memcmp( device->name, "\pUSBVision", device->name[0] + 1 ) == 0 ) {
				if( fSetSoundInputDriver( "\pUSBVision" ) != noErr )
					fSetSoundInputDriver( "\pBuilt-in Input" );
				}
			else
				if( fSetSoundInputDriver( device->name ) != noErr )
					if( fSetSoundInputDriver( "\pUnknown USB Audio Device" ) != noErr )
						if( fSetSoundInputDriver( "\pUSB 2861 Device" ) != noErr )
							if( fSetSoundInputDriver( "\piGrabber Device" ) != noErr )
								if( fSetSoundInputDriver( "\pi Grabber Device" ) != noErr )
									if( fSetSoundInputDriver( "\pUSB VIDBOX FW Audio" ) != noErr )
										fSetSoundInputDriver( "\pBuilt-in Input" );
			}
		if( anErr == noErr ) {
			anErr = ::SGSetChannelVolume( mSoundChannel, 0x7FFF );
			if( anErr != noErr )
				fReportSoundError( "\pSGSetChannelVolume", anErr );
			}
		if( anErr == noErr ) {
			if( anErr == noErr ) {
				anErr = ::SGSetSoundInputParameters( mSoundChannel, 16, 2, 'raw ' );
				if( anErr != noErr )
					fReportSoundError( "\pSGSetSoundInputParameters", anErr );
				}
			if( anErr == noErr ) {
				anErr = ::SGSetSoundInputRate( mSoundChannel, 0xAC440000 ); // 44100
				if( anErr != noErr )
					fReportSoundError( "\pSGSetSoundInputRate", anErr );
				}
			}
		if( anErr == noErr ) {
			anErr = ::SGSetChannelUsage( mSoundChannel, kChannelUsageFlagsPreviewAudio );
			if( anErr != noErr )
				fReportSoundError( "\pSGSetChannelUsage", anErr );
			}
		}
	if( result == noErr && theErr != noErr ) {
		result = 6;
		fReportStartSequencerError( "kCaptureOpenErrorMessage6", theErr );
		}

	/* Restore the audio settings from preferences. Note that the device last used is	*/
	/* restored before applying the preferred sound data.								*/
	if( theErr == noErr )
		if( mSoundChannel != NULL ) {
			fLoadSoundDriver();
			fLoadSettings( kPrefTagAudioChannelData, mSoundChannel );
			}

	/* Save the video and sound settings that have been established up to this point,	*/
	/* whether by default, via preferences or by force.									*/
	if( theErr == noErr ) {
		fSaveSettings( kPrefTagVideoChannelData, mVideoChannel );
		if( mSoundChannel != NULL ) {
			fSaveSoundDriver();
			fSaveSettings( kPrefTagAudioChannelData, mSoundChannel );
			}
		}

	/* Disable the sound until all windows are opened. fLoadSettings() must be called	*/
	/* before doing this so that we know what the user audio play-through preferences	*/
	/* are.																				*/
	if( theErr == noErr )
		fStopSoundPreview();

	/* Show the window, otherwise SGStartPreview() will fail sometimes. We are going	*/
	/* to take a chance with the 3Cam software and show later to avoid all the wild		*/
	/* flickering.																		*/
	if( theErr == noErr )
		#if kNTZPackageNumber == '3Cam'
		if( g3CamRunMode == k3CamRunModeNormal )
		#endif
		Show();

	/* Start the preview */
	Boolean previewStarted = false;
	if( theErr == noErr ) {
		theErr = ::SGStartPreview( mSequenceGrabber );
		/* I have seen occassions where I can wait for a moment and get the sound input	*/
		/* device to work where previously it wouldn't.									*.
		/* siDeviceBusyErr == -227														*/
		SInt32 count = 0;
		while( theErr == siDeviceBusyErr && count++ < 5 ) {
			::usleep( 500000 );
			theErr = ::SGStartPreview( mSequenceGrabber );
			}
		previewStarted = theErr == noErr;
		}
	if( result == noErr && theErr != noErr ) {
		result = 7;
		fReportStartSequencerError( "kCaptureOpenErrorMessage7", theErr );
		}

	/* Get the vdig instance in use. We need the digitizer for all kinds of things		*/
	/* ibcluding large snapshot and full screen support.								*/
	mVideoDigitizer = NULL;
	if( theErr == noErr ) {
		mVideoDigitizer = ::SGGetVideoDigitizerComponent( mVideoChannel );
		if( mVideoDigitizer == NULL )
			theErr = -4;
		}
	if( result == noErr && theErr != noErr ) {
		result = 8;
		fReportStartSequencerError( "kCaptureOpenErrorMessage8", theErr );
		}

	/* See if the digitizer is being driven by an EchoFX driver */
	mNTZInterfaceAvailable = false;
	mNTZDeviceType = kNTZDeviceTypeUnknown;
	if( theErr == noErr )
		fNTZVDCheckInterface( mVideoDigitizer, &mNTZInterfaceAvailable, &mNTZDeviceType );

	/* If its one of our devices set up the snapshot callback. Note that we don't		*/
	/* actually want to handle the NT100x devices because they process the snapshot		*/
	/* button entirely within the VDIG.													*/
	if( theErr == noErr )
		if( mNTZInterfaceAvailable )
			if( mNTZDeviceType != kNTZDeviceType100x )
				::fNTZVDSetSnapshotCallback( mVideoDigitizer, fSnapshotCallback, (SInt32)this );

	/* Now preconfigure settings on any new devices that we find */
	if( theErr == noErr )
		if( newDevice )
			fPreconfigureNewDevice( device );

	/* For whatever reason, preview sound will sync vastly better if we wait a			*/
	/* moment, then pause and restart the preview. That is what	this is in				*/
	/* aid of. We are telling SpendTime() to do this for us one time.					*/
	mWantVolumeAtTime = 0;
	if( theErr == noErr ) {
		mWantVolumeAtTime = ::TickCount() + 60;
		::SGGetChannelVolume( mSoundChannel, &mWantVolumeAt );
		::SGSetChannelVolume( mSoundChannel, 0x0000 );
		}

	#if kNTZPackageNumber == 'agsp'
	/* Yet more weird cases. If this is a SOPRO handheld dental cam, and the size is	*/
	/* initially set to 640 x 480, it will not generate a video signal. However, if we	*/
	/* resize it to 320 x 240 then back, it will start working.							*/
	if( theErr == noErr ) {
		if( device->manufacturer == 'AGr ' ) {
			SInt32 saveSize = mSelectedSize;
			fResizeWindow( kCmdHalfSize );
			fResizeWindow( saveSize );
			}
		}
	#endif

	/* Clean up on error */
	if( theErr != noErr ) {
		fStopSoundPreview();
		::usleep( 25000 );
		if( mSequenceGrabber != NULL )
			::SGStop( mSequenceGrabber );
		if( mSoundChannel != NULL ) {
			fStopSoundPreview();
			::SGDisposeChannel( mSequenceGrabber, mSoundChannel );
			mSoundChannel = NULL;
			}
		/* OS 9 only */
		if( mSoundDeviceChanged ) {
			SInt32 refNum;
			if( ::SPBOpenDevice( "\pBuilt-in", siWritePermission, &refNum ) == noErr ) {
				::SPBSetDeviceInfo( refNum, siOSTypeInputSource, &mSoundDevice );
				::SPBCloseDevice( refNum );
				mSoundDeviceChanged = false;
				}
			}
		if( mVideoChannel != NULL ) {
			::SGDisposeChannel( mSequenceGrabber, mVideoChannel );
			mVideoChannel = NULL;
			}
		if( mSequenceGrabber != NULL ) {
			::CloseComponent( mSequenceGrabber );
			mSequenceGrabber = NULL;
			}
		}

	/* Remember when we started. Don't allow snapshots in the first couple seconds.		*/
	/* This should alleviate the false snapshots positives some devices give on start	*/
	/* up. It won't work if the serial number dialog comes up, but ah well.				*/
	if( theErr == noErr )
		mTicksSinceStart = ::TickCount();

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fGetBestSize( const Rect *rect ) {
/* Look at the devices preferred rect. Pick a size that is close. Note that we want to	*/
/* go with the standard size that is closest to the devices preferred rect, not the		*/
/* preferred rect itself, although in reality devices will almost certainly be			*/
/* returning standard sizes.															*/

	SInt32 size;
	SInt32 width = rect->right - rect->left;
	SInt32 height = rect->bottom - rect->top;

	if( width >= 320 && width <= 360 && height >= 230 && height < 250 )
		size = kCmd320x240;
	else if( width >= 320 && width <= 360 && height >= 280 && height < 300 )
		size = kCmdPAL360x288;
	else if( width >= 640 && width <= 720 && height == 480 )
		size = kCmd640x480;
	else if( width >= 640 && width <= 720 && height == 576 )
		size = kCmdPAL720x576;
	else
		size = kCmdFullSize;

	return( size );
	}

/*======================================================================================*/
void CCaptureWindow::fSnapshotCallback( SInt32 refCon ) {

	CCaptureWindow *captureWindow = dynamic_cast<CCaptureWindow *>( (CCaptureWindow *)refCon );
	if( captureWindow != NULL )
		captureWindow->fSnapshotCallback();
	}

/*======================================================================================*/
void CCaptureWindow::fSnapshotCallback( void ) {

	mSnapshotButtonPressed = true;
	}

/*======================================================================================*/
void CCaptureWindow::fReportStartSequencerError( const char *errorMessageKey, SInt32 theErr ) {

	LStr255 description;
	CUtilities2::fGetStringASCII( "Capture", errorMessageKey, description );

	StDialogHandler dialog( PPob_StartSequencerError, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	LStaticText *message = (LStaticText *)window->FindPaneByID( 'text' );
	ThrowIfNULL_( message );

	Str255 errNumString;
	::NumToString( theErr, errNumString );
	LStr255 messageText;
	message->GetDescriptor( messageText );

	if( mDevice.inputCount == 0 )
		messageText.Replace( messageText.Find( "\p^d" ), 2, mDevice.name );
	else
		messageText.Replace( messageText.Find( "\p^d" ), 2, mDevice.inputNames[mInput] );
	messageText.Replace( messageText.Find( "\p^m" ), 2, description );
	messageText.Replace( messageText.Find( "\p^e" ), 2, errNumString );
	message->SetDescriptor( messageText );

	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {
			case msg_OK:
				finished = true;
				break;
			}
		}
	}

/*======================================================================================*/
void CCaptureWindow::fReportSoundError( ConstStr255Param quickTimeCall, SInt32 theErr ) {

	StDialogHandler dialog( PPob_SoundChannelError, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	LStaticText *message = (LStaticText *)window->FindPaneByID( 'text' );
	ThrowIfNULL_( message );

	Str255 errNumString;
	::NumToString( theErr, errNumString );
	LStr255 messageText;
	message->GetDescriptor( messageText );
	messageText.Replace( messageText.Find( "\p^q" ), 2, quickTimeCall );
	messageText.Replace( messageText.Find( "\p^e" ), 2, errNumString );
	message->SetDescriptor( messageText );

	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {
			case msg_OK:
				finished = true;
				break;
			}
		}
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fSGSetChannelUsage( SGChannel channel, long usage ) {
/* It may be necessary to pause or stop the sequence before setting the usage to		*/
/* prevent that Sound Manager crash we see so often. SGSetChannelUsage() discards		*/
/* memory that the sound manager is using if usage is 0.								*/

	SInt32 theErr = noErr;

	if( theErr == noErr ) {
		Boolean paused = ::SGPause( mSequenceGrabber, seqGrabPause ) == noErr;
		theErr = ::SGSetChannelUsage( channel, usage );
		if( paused )
			::SGPause( mSequenceGrabber, seqGrabUnpause );
		}

	return( theErr );
	}

/*======================================================================================*/
void CCaptureWindow::fResizeWindow( SInt32 inSize ) {

	Rect rect;
	GetPortBounds( GetMacPort(), &rect );
	InvalPortRect( &rect );

	SInt32 width;
	SInt32 height;
	switch( inSize ) {
		case kCmdQuarterSize:
			width = mWidth / 4;
			height = mHeight / 4;
			break;
		case kCmdHalfSize:
			width = mWidth / 2;
			height = mHeight / 2;
			break;
		case kCmdFullSize:
			width = mWidth;
			height = mHeight;
			break;
		case kCmdDoubleSize:
			width = mWidth * 2;
			height = mHeight * 2;
			break;
		case kCmd80x60:
			width = 80;
			height = 60;
			break;
		case kCmd160x120:
			width = 160;
			height = 120;
			break;
		case kCmd320x240:
			width = 320;
			height = 240;
			break;
		case kCmd640x480:
			width = 640;
			height = 480;
			break;
		case kCmdPAL360x288:
			width = 360;
			height = 288;
			break;
		case kCmdPAL720x576:
			width = 720;
			height = 576;
			break;
		case kCmdCustomSize:
			width = mCustomWidth;
			height = mCustomHeight;
			break;
		case kCmdFullScreen:
		case kCmdFullScreen16x9:
			mMultiplier = 2.0;

			/* We are kind of messed up now. If we see CIF resolution, modify the		*/
			/* resolution so that the aspect ratio is correct, i.e. 4:3					*/
			mFSWidth = mWidth;
			mFSHeight = mHeight;
			if( mWidth == 352 && mHeight == 240 ) {
				mFSWidth = 320;
				mFSHeight = 240;
				}
			if( mWidth == 352 && mHeight == 288 ) {
				mFSWidth = 352;
				mFSHeight = 264;
				}

			GDHandle screen = ::GetMainDevice();
			if( screen != NULL ) {
				Rect rect = (*screen)->gdRect;
				float hMultiplier = (rect.right - rect.left) / (float)mFSWidth;
				float vMultiplier = (rect.bottom - rect.top) / (float)mFSHeight;
				mMultiplier = hMultiplier;
				if( mMultiplier > vMultiplier )
					mMultiplier = vMultiplier;

				/* Handle 16 x 9 request */
				if( inSize == kCmdFullScreen16x9 ) {
					if( mMultiplier * 12.0 / 9.0 * mFSWidth > (rect.right - rect.left) )
						mMultiplier = (rect.right - rect.left) / (float)mFSWidth;
					else
						mMultiplier *= 12.0 / 9.0;
					}
				}

			width = (SInt32)(mFSWidth * mMultiplier + 0.5);
			height = (SInt32)(mFSHeight * mMultiplier + 0.5);
			break;
		}
	mWindowWidth = width;
	mWindowHeight = height;
	ResizeWindowTo( width, height );
	fResizeSequencer( inSize );
	mSelectedSize = inSize;
	gPreferences.fSetPreference( kPrefTagSelectedSize, mPreferenceID, mSelectedSize );

	/* This is not helpful in PAL systems */
	#if 0
	/* Decide if we are going to display an aspect warning ratio during SpendTime() */
	mAspectRatioWarning = false;
	if( mSelectedSize != kCmdFullScreen &&
		mSelectedSize != kCmdFullScreen16x9 )
		if( width * 3 / 4 != height ) {
			SInt32 warningOff = 0;
			if( gPreferences.fGetPreference( kPrefTagARWarningOff, mPreferenceID,
				&warningOff ) == noErr && warningOff == 0 ||
				fOptionPressed() )
				mAspectRatioWarning = true;
			}
	#endif
	}

/*======================================================================================*/
void CCaptureWindow::fAspectRatioWarning( void ) {
/* If the aspect ratio of the frame is not 4:3 then provide a warning about the			*/
/* possible consequences. The decision to call this is made at the end of the previous	*/
/* function. Howeever, we defer calling it until the first call to SpendTime().			*/

	StDialogHandler dialog( PPob_WarningAspectRatio, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	LStaticText *message = (LStaticText *)window->FindPaneByID( 'text' );
	LCheckBox *checkbox = (LCheckBox *)window->FindPaneByID( 'dont' );
	ThrowIfNULL_( message );
	ThrowIfNULL_( checkbox );

	LStr255 messageText;
	message->GetDescriptor( messageText );
	if( mDevice.inputCount == 0 )
		messageText.Replace( messageText.Find( "\p^0" ), 2, mDevice.name );
	else
		messageText.Replace( messageText.Find( "\p^0" ), 2, mDevice.inputNames[mInput] );
	message->SetDescriptor( messageText );

	SInt32 warningOff = 0;
	gPreferences.fGetPreference( kPrefTagARWarningOff, mPreferenceID, &warningOff );
	checkbox->SetValue( warningOff );

	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {
			case msg_OK:
				gPreferences.fSetPreference( kPrefTagARWarningOff,
					mPreferenceID, checkbox->GetValue() );
				finished = true;
				break;
			}
		}
	}

/*======================================================================================*/
OSStatus CCaptureWindow::fResizeSequencer( SInt32 inSize ) {

	OSErr theErr = noErr;

	/* Stop the preview */
	Boolean previewStopped = false;
	if( theErr == noErr ) {
		theErr = ::SGStop( mSequenceGrabber );
		previewStopped = theErr == noErr;
		theErr = noErr;
		}

	if( theErr == noErr ) {
		Rect rect;
		GetPortBounds( GetMacPort(), &rect );
		if( inSize == kCmdFullScreen ||
			inSize == kCmdFullScreen16x9 ) {
			/* Center the digitizer rect */
			SInt32 width = (SInt32)(mFSWidth * mMultiplier + 0.5);
			SInt32 height = (SInt32)(mFSHeight * mMultiplier + 0.5);
			SInt32 hDelta = (rect.right - rect.left - width) / 2;
			SInt32 vDelta = (rect.bottom - rect.top - height) / 2;
			::SetRect( &rect, 0, 0, width, height );
			::OffsetRect( &rect, hDelta, vDelta );
			}
		mCaptureWidth = rect.right - rect.left;
		mCaptureHeight = rect.bottom - rect.top;
		theErr = ::SGSetChannelBounds( mVideoChannel, &rect );
		}

	/* Start the preview */
	if( theErr == noErr )
		if( previewStopped )
			theErr = ::SGStartPreview( mSequenceGrabber );

	return( theErr );
	}

/*======================================================================================*/
OSStatus CCaptureWindow::fBeginFullScreen( SInt32 inSize ) {

	SInt32 theErr = noErr;
	Rect rect;
	::SetRect( &rect, 0, 0, mWidth, mHeight );

	if( !mFullScreen ) {

		BroadcastMessage( 'paus', this );

		StGrafPortSaver portSaver( GetMacPort() );

		/* Stop the preview */
		Boolean previewStopped = false;
		if( theErr == noErr ) {
			theErr = ::SGStop( mSequenceGrabber );
			previewStopped = theErr == noErr;
			theErr = noErr;
			}

		/* Get the video digitizer */
		if( theErr == noErr )
			if( mVideoDigitizer == NULL )
				theErr = -1;

		/* If this is a Nogatech device, tell it to switch to component video output.	*/
		/* QuickTime is not smart enough to tell it automatically.						*/
		mSaveCompressor = 0;
		if( theErr == noErr )
			if( mNTZDeviceType == kNTZDeviceType100x ) {
				::fNTZVDGetDefaultCompressor( mVideoDigitizer, &mSaveCompressor );
				::fNTZVDSetDefaultCompressor( mVideoDigitizer, k422YpCbCr8CodecType );
				}

		/* Set the digitizer usage: preview full screen */
		mVideoUsageChanged = false;
		if( theErr == noErr )
			theErr = ::SGGetChannelUsage( mVideoChannel, &mVideoUsage );
		if( theErr == noErr ) {
			theErr = fSGSetChannelUsage( mVideoChannel, kChannelUsageFlagsPreviewVideo );
			mVideoUsageChanged = theErr == noErr;
			}

		/* Change the sound channel usage to preview */
		mSoundUsageChanged = false;
		if( mSoundChannel != NULL ) {
			if( theErr == noErr )
				theErr = ::SGGetChannelUsage( mSoundChannel, &mSoundUsage );
			if( theErr == noErr ) {
				SInt32 usage = kChannelUsageFlagsPreviewAudio & ~seqGrabPreview | mSoundDuringPreview;
				theErr = fSGSetChannelUsage( mSoundChannel, usage );
				mSoundUsageChanged = theErr == noErr;
				}
			}

		/* Enter full screen mode */
		SInt32 flags = fullScreenHideCursor;
		#if kNTZPackageNumber == '3Cam'
		if( g3CamRunMode == k3CamRunModeFullScreen )
			flags = 0;
		#endif
		mFullScreenEntered = false;
		SInt16 desiredWidth;
		SInt16 desiredHeight;
		SInt16 screenWidth;
		SInt16 screenHeight;
		if( theErr == noErr ) {
			screenWidth = desiredWidth = (SInt16)(mFSWidth * mMultiplier + 0.5);
			screenHeight = desiredHeight = (SInt16)(mFSHeight * mMultiplier + 0.5);
			theErr = ::BeginFullScreen( &mRestoreState, NULL, &screenWidth,
				&screenHeight, NULL, NULL, flags );
			mFullScreenEntered = theErr == noErr;
			}

		/* Resize the window (no need to save state) */
		SInt16 finalWidth;
		SInt16 finalHeight;
		if( theErr == noErr ) {
			finalWidth = CUtilities::fMax( screenWidth, desiredWidth );
			finalHeight = CUtilities::fMax( screenHeight, desiredHeight );
			mWindowWidth = finalWidth;
			mWindowHeight = finalHeight;
			ResizeWindowTo( finalWidth, finalHeight );
			fResizeSequencer( inSize );
			}

		/* Save the monitor position and move it to the top left corner of the screen */
		mWindowMoved = false;
		if( theErr == noErr ) {
			::SetPt( &mTopLeft, 0, 0 );
			PortToGlobalPoint( mTopLeft );
			MoveWindowTo( (screenWidth - finalWidth) / 2, (screenHeight - finalHeight) / 2 );
			mWindowMoved = true;
			}

		/* Start the preview. Note that the format selected will be the one chosen in	*/
		/* the Video Settings dialog, unless no compressor component for that format is	*/
		/* installed. In which case the default ('IVno') will be used instead.			*/
		if( theErr == noErr )
			if( previewStopped )
				theErr = ::SGStartPreview( mSequenceGrabber );

		/* Now that the preview has been started, we may call SGSetVideoCompressorType.	*/
		/* However -be warned that the code is picky, and will only work if a 2vuy		*/
		/* compressor is available. Note that Final Cut installs such a compressor, and	*/
		/* so this will work in that case. Be sure to ignore the error if any.			*/
		mSaveCompressorType = 0;
		if( theErr == noErr )
			theErr = ::SGGetVideoCompressorType( mVideoChannel, &mSaveCompressorType );
		if( theErr == noErr )
			::SGSetVideoCompressorType( mVideoChannel, k422YpCbCr8CodecType );

		if( theErr != noErr ) {
			if( mSaveCompressorType != 0 )
				::SGSetVideoCompressorType( mVideoChannel, mSaveCompressorType );
			if( mWindowMoved )
				DoSetPosition( mTopLeft );
			if( mFullScreenEntered )
				::EndFullScreen( mRestoreState, 0 );
			if( mSoundUsageChanged )
				fSGSetChannelUsage( mSoundChannel, mSoundUsage );
			if( mVideoUsageChanged )
				fSGSetChannelUsage( mVideoChannel, mVideoUsage );
			if( mVideoDigitizer != NULL )
				if( mNTZDeviceType == kNTZDeviceType100x )
					::fNTZVDSetDefaultCompressor( mVideoDigitizer, mSaveCompressor );
			}

		mFullScreen = theErr == noErr;
		}

	return( theErr );
	}

/*======================================================================================*/
OSStatus CCaptureWindow::fEndFullScreen( void ) {

	SInt32 theErr = noErr;
	Rect rect;
	::SetRect( &rect, 0, 0, mWidth, mHeight );

	if( mFullScreen ) {

		StGrafPortSaver portSaver( GetMacPort() );

		/* Stop the preview */
		Boolean previewStopped = false;
		if( theErr == noErr ) {
			theErr = ::SGStop( mSequenceGrabber );
			previewStopped = theErr == noErr;
			theErr = noErr;
			}

		if( mSaveCompressorType != 0 )
			::SGSetVideoCompressorType( mVideoChannel, mSaveCompressorType );
		if( mWindowMoved )
			DoSetPosition( mTopLeft );
		if( mFullScreenEntered )
			::EndFullScreen( mRestoreState, 0 );
		if( mSoundUsageChanged )
			fSGSetChannelUsage( mSoundChannel, mSoundUsage );
		if( mVideoUsageChanged )
			fSGSetChannelUsage( mVideoChannel, mVideoUsage );
		if( mVideoDigitizer != NULL )
			if( mNTZDeviceType == kNTZDeviceType100x )
				::fNTZVDSetDefaultCompressor( mVideoDigitizer, mSaveCompressor );

		/* Restore the video settings from preferences. Note that this will actually	*/
		/* open the sequence grabber panel component and restore the preferences it		*/
		/* saved.																		*/
		fLoadSettings( kPrefTagVideoChannelData, mVideoChannel );

		/* Start the preview */
		if( theErr == noErr )
			if( previewStopped )
				theErr = ::SGStartPreview( mSequenceGrabber );

		BroadcastMessage( 'resu', this );

		mFullScreen = false;
		}

	return( theErr );
	}

/*======================================================================================*/
void CCaptureWindow::ActivateSelf( void ) {

	/* Stop all audio everywhere, except this window */
	BroadcastMessage( 'aStp', this );

	/* Start audio in this window. Note that we won't be recording. If we are,			*/
	/* ActivateSelf() can't be called.													*/
	fStartSoundPreview();

	/* Set the "Record Video", "Record Audio" options. These are per digitizer options */
	SInt32 recordVideo = mRecordVideo ? 1 : 0;
	gPreferences.fGetPreference( kPrefTagRecordVideo, mPreferenceID, &recordVideo );
	mRecordVideo = recordVideo != 0;
	SInt32 recordSound = mRecordSound ? 1 : 0;
	gPreferences.fGetPreference( kPrefTagRecordSound, mPreferenceID, &recordSound );
	mRecordSound = recordSound != 0;
	SetUpdateCommandStatus( true );
	}

/*======================================================================================*/
void CCaptureWindow::Suspend( void ) {
/* Unfortunately I can't mute a recording while it is going (cantDoThatInCurrentMode)	*/
/* so we can only mute preview windows. Only one preview window will actually be		*/
/* playing, but that's ok.																*/

	if( !mIsRecording ) {
		mPreSuspensionPreviewSoundUsage = -1;
		SInt32 backgroundMute = 1;
		gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscBackgroundMute, &backgroundMute );
		if( backgroundMute != 0 )
			if( ::SGGetChannelUsage( mSoundChannel, &mPreSuspensionPreviewSoundUsage ) == noErr )
				fStopSoundPreview();
		}
	}

/*======================================================================================*/
void CCaptureWindow::Resume( void ) {
/* Restore the preview window's sound play-through setting. We can't rely on			*/
/* ActivateSelf() to do this because the Preferences or About Dialog might be the		*/
/* active window and thus no preview window would be activated upon resumption and the	*/
/* one that should resume playing won't.												*/

	if( !mIsRecording )
		if( mPreSuspensionPreviewSoundUsage != -1 ) {
			SInt32 backgroundMute = 1;
			gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscBackgroundMute, &backgroundMute );
			if( backgroundMute != 0 ) {
				fSGSetChannelUsage( mSoundChannel, mPreSuspensionPreviewSoundUsage );
				mPreSuspensionPreviewSoundUsage = -1;
				}
			}
	}

/*======================================================================================*/
void CCaptureWindow::DrawSelf( void ) {

	#if kNTZPackageNumber == '3Cam'
	if( g3CamRunMode == k3CamRunModeVideoRecorder )
		return;
	#endif

	Rect rect;
	CalcLocalFrameRect( rect );
	RGBColor saveBackColor;
	::GetBackColor( &saveBackColor );
	::BackColor( blackColor );
	::EraseRect( &rect );
	::RGBBackColor( &saveBackColor );
	}

/*======================================================================================*/
Boolean CCaptureWindow::HandleKeyPress( const EventRecord &inKeyEvent ) {

	Boolean keyHandled = false;

	#if kNTZPackageNumber == '3Cam'
	if( (inKeyEvent.modifiers & cmdKey) != 0 ) {
		UInt8 charCode = inKeyEvent.message & charCodeMask;
		switch( g3CamRunMode ) {
			case k3CamRunModeFullScreen:
				if( charCode == 'c' || charCode == 'C' ) {
					mLCTSnapButton->SimulateHotSpotClick( 0 );
					keyHandled = true;
					}
				break;
			case k3CamRunModeVideoRecorder:
				if( charCode == 'r' || charCode == 'R' ) {
					mLCTRecordButton->SimulateHotSpotClick( 0 );
					keyHandled = true;
					}
				if( charCode == 's' || charCode == 'S' ) {
					mLCTStopButton->SimulateHotSpotClick( 0 );
					keyHandled = true;
					}
				if( charCode == 'a' || charCode == 'A' ||
					charCode == 't' || charCode == 'T' ) {
					mLCTToolsButton->SimulateHotSpotClick( 0 );
					keyHandled = true;
					}
				break;
			}
		}
	#endif

	if( !keyHandled )
		if( mFullScreen ) {
			UInt8 charCode = inKeyEvent.message & charCodeMask;
			if( charCode == 0x1B ||	// esc
				charCode == ' ' ) {	// space
				#if kNTZPackageNumber == '3Cam'
				if( g3CamRunMode == k3CamRunModeFullScreen )
					mLCTQuitButton->SimulateHotSpotClick( 0 );
				else {
				#endif
				ObeyCommand( mSelectedSize, NULL );
				SetUpdateCommandStatus( true );
				keyHandled = true;
				#if kNTZPackageNumber == '3Cam'
				}
				#endif
				}
			}

	if( !keyHandled )
		keyHandled = LCommander::HandleKeyPress( inKeyEvent );

	return( keyHandled );
	}

/*======================================================================================*/
Boolean CCaptureWindow::ObeyCommand( CommandT inCommand, void *ioParam ) {

	Boolean cmdHandled = true;

	switch( inCommand ) {

		#if EXPIRES || EDIT_REGISTERS
		case kCmdEditRegisters:
			fEditRegisters();
			break;
		#endif

		case kCmdQuarterSize:
		case kCmdHalfSize:
		case kCmdFullSize:
		case kCmdDoubleSize:
		case kCmd80x60:
		case kCmd160x120:
		case kCmd320x240:
		case kCmd640x480:
		case kCmdPAL360x288:
		case kCmdPAL720x576:
			if( mFullScreen )
				fEndFullScreen();
			fResizeWindow( inCommand );
			mRegularSize = mSelectedSize;
			gPreferences.fSetPreference( kPrefTagRegularSize, mPreferenceID, mRegularSize );
			break;

		case kCmdCustomSize:
			if( mFullScreen )
				fEndFullScreen();
			if( fGetCustomSize( &mCustomWidth, &mCustomHeight ) ) {
				fResizeWindow( inCommand );
				mRegularSize = mSelectedSize;
				gPreferences.fSetPreference( kPrefTagRegularSize, mPreferenceID, mRegularSize );
				gPreferences.fSetPreference( kPrefTagCustomWidth, mPreferenceID, mCustomWidth );
				gPreferences.fSetPreference( kPrefTagCustomHeight, mPreferenceID, mCustomHeight );
				}
			break;

		case kCmdFullScreen:
		case kCmdFullScreen16x9:
			if( mFullScreen &&
				inCommand != mSelectedSize ) {
				fEndFullScreen();
				fResizeWindow( mRegularSize );
				}
			if( !mFullScreen ) {
				fResizeWindow( inCommand );
				fBeginFullScreen( inCommand );
				}
			else {
				fEndFullScreen();
				fResizeWindow( mRegularSize );
				}
			break;

		case kCmdVideoSettings:
			fVideoSettings();
			break;

		case kCmdSoundSettings:
			fSoundSettings();
			break;

		case kCmdRecordVideo:
			mRecordVideo = !mRecordVideo;
			gPreferences.fSetPreference( kPrefTagRecordVideo, mPreferenceID, mRecordVideo ? 1 : 0 );
			break;

		case kCmdRecordSound:
			mRecordSound = !mRecordSound;
			gPreferences.fSetPreference( kPrefTagRecordSound, mPreferenceID, mRecordSound ? 1 : 0 );
			break;

		case kCmdRecord:
			fStartRecording( 0, NULL, NULL, false );
			break;

		case kCmdRecordDuration: {
			UInt32 hours = 0;
			UInt32 minutes = 5;
			UInt32 seconds = 0;
			gPreferences.fGetPreference( kPrefTagHours, mPreferenceID, (SInt32 *)&hours );
			gPreferences.fGetPreference( kPrefTagMinutes, mPreferenceID, (SInt32 *)&minutes );
			gPreferences.fGetPreference( kPrefTagSeconds, mPreferenceID, (SInt32 *)&seconds );
			if( fGetDuration( &hours, &minutes, &seconds ) ) {
				gPreferences.fSetPreference( kPrefTagHours, mPreferenceID, hours );
				gPreferences.fSetPreference( kPrefTagMinutes, mPreferenceID, minutes );
				gPreferences.fSetPreference( kPrefTagSeconds, mPreferenceID, seconds );
				fStartRecording( hours * 3600 + minutes * 60 + seconds, NULL, NULL, false );
				}
			break; }

		case kCmdSnapshotTimeLapse:
		case kCmdRecordTimeLapse: {
			Boolean isSnapshot = inCommand == kCmdSnapshotTimeLapse;
			sTimeLapseData timeLapse = { 0 };
			timeLapse.intervalValue = (isSnapshot ? 30 : 1) * 65536; // 30 or 1 second between frames
			timeLapse.hasDuration = 1;
			timeLapse.durationValue = 1 * 65536;					 // 1 hour
			timeLapse.durationUnit = 2;
			timeLapse.outputFrameRate = 30 * 65536;					 // 30 fps
			SInt32 prefsTag = isSnapshot ? kPrefTagTimeLapseSnapshot : kPrefTagTimeLapseRecord;
			SInt32 length = sizeof( sTimeLapseData );
			gPreferences.fGetPreference( prefsTag, mPreferenceID, (char *)&timeLapse, &length );
			timeLapse.outputMode = isSnapshot ? kOutputModeSnapshot : kOutputModeMovie;
			if( fGetTimeLapse( &timeLapse ) ) {
				gPreferences.fSetPreference( prefsTag, mPreferenceID, (char *)&timeLapse, sizeof( sTimeLapseData ) );
				fStartRecording( 0, NULL, &timeLapse, false );
				}
			break; }

		case kCmdSnapshotSmall:
			fSnapshotSmall( NULL, false );
			break;

		case kCmdSnapshotLarge:
			#if kNTZLicenseeNumber != 'eEm '
			fSnapshotLarge( NULL, false );
			#endif
			break;

		default:
			cmdHandled = LWindow::ObeyCommand( inCommand, ioParam );
			break;
		}

	return( cmdHandled );
	}

/*======================================================================================*/
Boolean CCaptureWindow::fGetDuration( UInt32 *outHours, UInt32 *outMinutes, UInt32 *outSeconds ) {

	Boolean gotValues = false;
	UInt32 hours;
	UInt32 minutes;
	UInt32 seconds;

	StDialogHandler dialog( PPob_Duration, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	LEditText *paneHours = (LEditText *)window->FindPaneByID( 'Hour' );
	LEditText *paneMinutes = (LEditText *)window->FindPaneByID( 'Minu' );
	LEditText *paneSeconds = (LEditText *)window->FindPaneByID( 'Seco' );
	ThrowIfNULL_( paneHours );
	ThrowIfNULL_( paneMinutes );
	ThrowIfNULL_( paneSeconds );

	paneHours->SetValue( *outHours );
	paneMinutes->SetValue( *outMinutes );
	paneSeconds->SetValue( *outSeconds );

	if( *outHours > 0 ) {
		paneHours->SelectAll();
		window->SetLatentSub( paneHours );
		}
	else
		if( *outMinutes > 0 ) {
			paneMinutes->SelectAll();
			window->SetLatentSub( paneMinutes );
			}
		else {
			paneSeconds->SelectAll();
			window->SetLatentSub( paneSeconds );
			}

	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {
			case msg_OK:
				hours = paneHours->GetValue();
				minutes = paneMinutes->GetValue();
				seconds = paneSeconds->GetValue();
				if( hours < 0 || hours > 1000 ) {
					paneHours->SelectAll();
					LCommander::SwitchTarget( paneHours );
					::ParamText( "\p0", "\p1000", "\p", "\p" );
					UModalAlerts::StopAlert( ALRT_NumberBad );
					}
				else
					if( minutes < 0 || minutes > 1000 ) {
						paneMinutes->SelectAll();
						LCommander::SwitchTarget( paneMinutes );
						::ParamText( "\p0", "\p1000", "\p", "\p" );
						UModalAlerts::StopAlert( ALRT_NumberBad );
						}
					else
						if( seconds < 0 || seconds > 1000 ) {
							paneSeconds->SelectAll();
							LCommander::SwitchTarget( paneSeconds );
							::ParamText( "\p0", "\p1000", "\p", "\p" );
							UModalAlerts::StopAlert( ALRT_NumberBad );
							}
						else {
							*outHours = hours;
							*outMinutes = minutes;
							*outSeconds = seconds;
							gotValues = true;
							finished = true;
							}
				break;
			case msg_Cancel:
				finished = true;
				break;
			}
		}

	return( gotValues );
	}

/*======================================================================================*/
Boolean CCaptureWindow::fGetCustomSize( SInt32 *customWidth, SInt32 *customHeight ) {

	Boolean gotValues = false;
	SInt32 width;
	SInt32 height;

	StDialogHandler dialog( PPob_CustomSize, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	LEditText *paneWidth = (LEditText *)window->FindPaneByID( 'Widt' );
	LEditText *paneHeight = (LEditText *)window->FindPaneByID( 'Heig' );
	ThrowIfNULL_( paneWidth );
	ThrowIfNULL_( paneHeight );

	paneWidth->SetValue( *customWidth );
	paneWidth->SelectAll();
	window->SetLatentSub( paneWidth );
	paneHeight->SetValue( *customHeight );

	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {
			case msg_OK:
				width = paneWidth->GetValue();
				height = paneHeight->GetValue();
				if( width < 64 || width > 32000 ) {
					paneWidth->SelectAll();
					LCommander::SwitchTarget( paneWidth );
					::ParamText( "\p64", "\p32000", "\p", "\p" );
					UModalAlerts::StopAlert( ALRT_NumberBad );
					}
				else
					if( height < 48 || height > 32000 ) {
						paneHeight->SelectAll();
						LCommander::SwitchTarget( paneHeight );
						::ParamText( "\p48", "\p32000", "\p", "\p" );
						UModalAlerts::StopAlert( ALRT_NumberBad );
						}
					else {
						*customWidth = width;
						*customHeight = height;
						gotValues = true;
						finished = true;
						}
				break;
			case msg_Cancel:
				finished = true;
				break;
			}
		}

	return( gotValues );
	}

/*======================================================================================*/
void CCaptureWindow::FindCommandStatus( CommandT inCommand, Boolean &outEnabled,
	Boolean &outUsesMark, UInt16 &outMark, Str255 outName ) {
/* Determine the status of a Command for the purposes of menu updating.					*/

	switch( inCommand ) {

		#if EXPIRES || EDIT_REGISTERS
		case kCmdEditRegisters:
			outEnabled = mNTZInterfaceAvailable;
			break;
		#endif

		case kCmdQuarterSize:
		case kCmdHalfSize:
		case kCmdFullSize:
		case kCmdDoubleSize:
		case kCmd80x60:
		case kCmd160x120:
		case kCmd320x240:
		case kCmd640x480:
		case kCmdPAL360x288:
		case kCmdPAL720x576:
		case kCmdCustomSize:
		case kCmdFullScreen:
		case kCmdFullScreen16x9:
			outUsesMark = true;
			outMark = noMark;
			if( inCommand == mSelectedSize )
				outMark = checkMark;
			outEnabled = true;
			break;

		case kCmdVideoSettings:
			outEnabled =
				mSelectedSize != kCmdFullScreen &&
				mSelectedSize != kCmdFullScreen16x9;
			break;

		case kCmdSoundSettings:
			outEnabled =
				mSelectedSize != kCmdFullScreen &&
				mSelectedSize != kCmdFullScreen16x9 &&
				mSoundChannel != NULL;
			break;

		case kCmdRecordVideo:
			outUsesMark = true;
			outMark = noMark;
			if( mRecordVideo )
				outMark = checkMark;
			outEnabled = true;
			break;

		case kCmdRecordSound:
			outUsesMark = true;
			outMark = noMark;
			if( mRecordSound && mSoundChannel != NULL )
				outMark = checkMark;
			outEnabled = mSoundChannel != NULL;
			break;

		case kCmdRecord:
		case kCmdRecordDuration:
			outEnabled =
				mSelectedSize != kCmdFullScreen &&
				mSelectedSize != kCmdFullScreen16x9 &&
				(mRecordVideo || mRecordSound && mSoundChannel != NULL);
			break;
		case kCmdRecordTimeLapse:
			outEnabled =
				mSelectedSize != kCmdFullScreen &&
				mSelectedSize != kCmdFullScreen16x9 &&
				(mRecordVideo || mRecordSound && mSoundChannel != NULL) &&
				CFM_AddressIsResolved_( InstallEventLoopTimer );
			break;

		case kCmdSnapshotSmall:
			outEnabled = true;
			break;
		case kCmdSnapshotLarge:
			outEnabled = mNTZDeviceType == kNTZDeviceType100x;
			break;
		case kCmdSnapshotTimeLapse:
			outEnabled =
				mSelectedSize != kCmdFullScreen &&
				mSelectedSize != kCmdFullScreen16x9 &&
				CFM_AddressIsResolved_( InstallEventLoopTimer );
			break;

		case kCmdDisplayFrameRate:
			outEnabled =
				mSelectedSize == kCmdFullScreen ||
				mSelectedSize == kCmdFullScreen16x9;
			break;

		default:
			LWindow::FindCommandStatus( inCommand, outEnabled, outUsesMark, outMark,
				outName );
			break;
		}
	}

/*======================================================================================*/
void CCaptureWindow::SpendTime( const EventRecord &inMacEvent ) {

	#pragma unused( inMacEvent )

	SInt32 theErr = noErr;

	if( mWantVolumeAtTime != 0 )
		if( ::TickCount() > mWantVolumeAtTime ) {
			mWantVolumeAtTime = 0;
			::SGPause( mSequenceGrabber, seqGrabPause );
			::SGSetChannelVolume( mSoundChannel, mWantVolumeAt );
			::SGPause( mSequenceGrabber, seqGrabUnpause );
			}

	if( mSequenceGrabber != NULL )
		#if kNTZPackageNumber == '3Cam'
		switch( g3CamRunMode ) {

			case k3CamRunModeNormal:
				theErr = ::SGIdle( mSequenceGrabber );
				break;

			case k3CamRunModeFullScreen: {
				/* Obscure the mouse if it has been still long enough (5 seconds) */
				static UInt32 gLastMouseMove;
				static Point gLastMouse;
				if( gLastMouseMove == 0 ) {
					gLastMouseMove = ::TickCount();
					::GetMouse( &gLastMouse );
					::LocalToGlobal( &gLastMouse );
					ObscureCursor();
					}
				else {
					Point mouse;
					::GetMouse( &mouse );
					UInt32 now = ::TickCount();
					::LocalToGlobal( &mouse );
					if( ::EqualPt( mouse, gLastMouse ) ) {
						if( now - gLastMouseMove > 300 )
							::ObscureCursor();
						}
					else {
						gLastMouseMove = now;
						gLastMouse = mouse;
						}
					}

				RgnHandle oldClip = ::NewRgn();
				RgnHandle newClip = ::NewRgn();
				RgnHandle quitRgn = ::NewRgn();
				RgnHandle snapRgn = ::NewRgn();

				Rect quitRect;
				Rect snapRect;
				mLCTQuitButton->CalcPortFrameRect( quitRect );
				mLCTSnapButton->CalcPortFrameRect( snapRect );
				::RectRgn( quitRgn, &quitRect );
				::RectRgn( snapRgn, &snapRect );

				CGrafPtr gWorld;
				::SGGetGWorld( mSequenceGrabber, &gWorld, NULL );
				::GetPortVisibleRegion( gWorld, oldClip );

				::DiffRgn( oldClip, quitRgn, newClip );
				::DiffRgn( newClip, snapRgn, newClip );
				::SetPortVisibleRegion( gWorld, newClip );

				theErr = ::SGIdle( mSequenceGrabber );

				::SetPortVisibleRegion( gWorld, oldClip );

				::DisposeRgn( oldClip );
				::DisposeRgn( newClip );
				::DisposeRgn( quitRgn );
				::DisposeRgn( snapRgn );
				break;
				}

			case k3CamRunModeVideoRecorder: {

				theErr = ::SGIdle( mSequenceGrabber );

				UInt32 now = ::TickCount();

				/* Check status once per second */
				if( mIsRecording )
					if( mLCTLastStatusUpdate < now - 60 ) {

						/* Update status text */
						mLCTLastStatusUpdate = now;
						LStr255 text;
						mLCTVRStatusText->GetDescriptor( text );
						if( text.Length() == 0 )
							mLCTVRStatusText->SetDescriptor( "\pRecording" );
						else
							mLCTVRStatusText->SetDescriptor( "\p" );

						/* Check disk space */
						if( mLCTReserveSpace != 0 ) {
							UInt64 freeSpace;
							if( CUtilities::fGetFreeDiskSpace( mRecordSpec.vRefNum, &freeSpace ) == noErr )
								if( freeSpace < mLCTReserveSpace ) {
									ListenToMessage( 'Stop', NULL );
									StDialogHandler dialog( PPob_LogicalChoiceDiskSpaceExhausted, this );
									while( dialog.DoDialog() != msg_OK ) {}
									}
							}
						}

				/* Update preview window */
				if( mLCTGWorld != NULL ) {
					FocusDraw(); // Absolutely required -shifts origin back to 0, 0?
					GrafPtr windowPort = GetMacPort();
					StGrafPortSaver portSaver( windowPort );
					StColorState::Normalize();
					Rect r;
					::SetRect( &r, 0, 0, 320, 240 );
					Boolean update = !mIsRecording;
					if( mIsRecording ) {
						if( mLCTLastCopyBits < now - 20 ) {
							mLCTLastCopyBits = now;
							update = true;
							}
						if( mLCTLastStatusUpdate < now - 60 ) {
							mLCTLastStatusUpdate = now;
							LStr255 text;
							mLCTVRStatusText->GetDescriptor( text );
							if( text.Length() == 0 )
								mLCTVRStatusText->SetDescriptor( "\pRecording" );
							else
								mLCTVRStatusText->SetDescriptor( "\p" );
							}
						}
					if( update )
						mLCTGWorld->CopyImage( windowPort, r );
					}

				break;
				}
			}
		#else
		theErr = ::SGIdle( mSequenceGrabber );
		#endif

	/* This is not helpful in PAL systems. Apparently its not helpful anywhere :-) */
	#if 0
	/* If desired, display the aspect ratio warning. We wait until now to do it so that	*/
	/* we can have something on the screen to which the user can refer.					*/
	if( !mIsRecording )
		if( mAspectRatioWarning ) {
			mAspectRatioWarning = false;
			fAspectRatioWarning();
			}
	#endif

	if( mIsRecording ) {

		UInt32 tickCount = ::TickCount();

		/* Look for errors that should stop the recording */
		if( mRecordErr == noErr ) {
			mRecordErr = theErr;
			if( mRecordErr != noErr )
				fStopRecording();
			}

		/* See if this is a timed session */
		if( mDuration > 0 )
			if( mNextDurationCheck < tickCount ) {
				mNextDurationCheck = tickCount + 60;
				UInt32 now;
				::GetDateTime( &now );
				SInt32 remaining = mStartTime + mDuration - (SInt32)now;
				if( remaining <= 0 ) {
					fUpdateTimePanel( 0 );
					fStopRecording();
					}
				else
					fUpdateTimePanel( (UInt32)remaining );
				}
		}

	/* Process any snapshot button clicks (definitely don't want to do any of this		*/
	/* inside SGIdle()!)																*/
	if( mSnapshotButtonPressed ) {
		mSnapshotButtonPressed = false;
		SInt32 snapshotButtonMode = kSnapshotButtonModeSnapshot;
		gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscSnapshotButtonMode, &snapshotButtonMode );
		switch( snapshotButtonMode ) {
			case kSnapshotButtonModeSnapshotAutoName:
			case kSnapshotButtonModeSnapshot:
				if( !mIsRecording )
					if( ::TickCount() - mTicksSinceStart > 120 )
						fSnapshotSmall( NULL, snapshotButtonMode == kSnapshotButtonModeSnapshotAutoName );
				break;
			case kSnapshotButtonModeRecordAutoName:
			case kSnapshotButtonModeRecord:
				if( !mFullScreen ) {
					if( !mIsRecording )
						fStartRecording( 0, NULL, NULL, snapshotButtonMode == kSnapshotButtonModeRecordAutoName );
					else
						fStopRecording();
					}
				break;
			}
		}
	}

/*======================================================================================*/
void CCaptureWindow::ListenToMessage( MessageT inMessage, void *ioParam ) {

	#pragma unused( ioParam )

	switch( inMessage ) {

		/* Click window sends these when it suspends and resumes */
		case 'Susp':
			if( fIsRecording() )
				fStopRecording();
			break;

		case 'Resu':
			if( fIsRecording() )
				fStopRecording();
			break;

		case 'Clik':
			if( fIsRecording() )
				fStopRecording();
			break;

		#if kNTZPackageNumber == '3Cam'

		case kMsgLogicalChoiceFSSetup: {

			/* Don't do this if the monitor is already in full screen. It no longer can	*/
			/* be, but what's a world without cruft?									*/
			if( !mFullScreen )
				ObeyCommand( kCmdFullScreen, NULL );

			/* We don't Show() in fStartSequencer(). Do it now */
			Show();
			break;
			}

		case 'Quit':
			LCommander::GetTopCommander()->ObeyCommand( cmd_Quit, NULL );
			break;

		case 'Snap':
			fSnapshotSmallLCTFSCopy();
			LCommander::GetTopCommander()->ObeyCommand( cmd_Quit, NULL );
			break;

		case kMsgLogicalChoiceVRSetup: {

			/* Set window to full size so that the sequencer is set up to the right		*/
			/* size. Then set it to the LCT specified size.								*/
			mLCTVRResolution = (SInt32)ioParam;
			mCustomWidth = mLCTVRResolution >> 16;
			mCustomHeight = mLCTVRResolution & 0xFFFF;
			mRegularSize = kCmdCustomSize;
			gPreferences.fSetPreference( kPrefTagRegularSize, mPreferenceID, mRegularSize );
			gPreferences.fSetPreference( kPrefTagCustomWidth, mPreferenceID, mCustomWidth );
			gPreferences.fSetPreference( kPrefTagCustomHeight, mPreferenceID, mCustomHeight );
			fResizeWindow( kCmdCustomSize );
			mWindowWidth = 320;
			mWindowHeight = 286;
			ResizeWindowTo( 320, 286 );
			SetDescriptor( "\p3Cam Video Recorder" );

			/* Redirect the output of SGIdle() to an offscreen GWorld (this is nuts) */
			Rect rect;
			SetRect( &rect, 0, 0, mCustomWidth, mCustomHeight );
			mLCTGWorld = new LGWorld( rect, 32 );
			if( mLCTGWorld != NULL ) {
				GrafPtr windowPort = mLCTGWorld->GetMacGWorld();
				StGrafPortSaver portSaver( windowPort );
				StColorState::Normalize();
				::SGSetGWorld( mSequenceGrabber, windowPort, NULL );
				}

			/* We don't Show() in fStartSequencer(). Do it now */
			Show();
			break;
			}

		case 'Reco':
			if( !fIsRecording() ) {
				fStartRecording( 0, NULL, NULL, false );
				Activate(); // Not sure why we need this. We do need it though. Why weren't we reactivated?
				if( fIsRecording() ) {
					mLCTVRStatusText->SetTextTraitsID( kTextTraitGeneva9Red );
					mLCTVRStatusText->SetDescriptor( "\pRecording" );
					mLCTRecordButton->Disable();
					mLCTStopButton->Enable();
					mLCTToolsButton->Disable();
					}
				}
			break;

		case 'Stop':
			if( fIsRecording() ) {
				mSuppressUI = true;
				fStopRecording();
				mLCTVRStatusText->SetTextTraitsID( kTextTraitGeneva9 );
				mLCTVRStatusText->SetDescriptor( "\pStopped" );
				mLCTRecordButton->Enable();
				mLCTStopButton->Disable();
				mLCTToolsButton->Enable();
				LCommander::GetTopCommander()->ObeyCommand( cmd_Quit, NULL );
				}
			break;

		case 'Tool':
			if( !fIsRecording() )
				fVideoSettings();
			break;

		#endif
		}
	}

/*======================================================================================*/
void CCaptureWindow::fVideoSettings( void ) {

	SInt32 theErr = noErr;

	FocusDraw();

	BroadcastMessage( 'paus', this );

	if( theErr == noErr ) {
		UDesktop::Deactivate();
		theErr = ::SGSettingsDialog( mSequenceGrabber, mVideoChannel, 0, NULL, 0, NULL, 0 );
		UDesktop::Activate();
		}

	/* Save the settings */
	if( theErr == noErr )
		fSaveSettings( kPrefTagVideoChannelData, mVideoChannel );

	/* Find out what the full-size of the digitizer is. We can then resize the window	*/
	/* appropriately.																	*/
	#if kNTZPackageNumber == '3Cam'
	if( g3CamRunMode == k3CamRunModeNormal ) {
	#endif
	Rect rect;
	if( theErr == noErr )
		theErr = ::SGGetSrcVideoBounds( mVideoChannel, &rect );
	if( theErr == noErr ) {
		OffsetRect( &rect, -rect.left, -rect.top );
		mWidth = rect.right;
		mHeight = rect.bottom;
		fResizeWindow( mSelectedSize );
		}
	#if kNTZPackageNumber == '3Cam'
	}
	#endif

	BroadcastMessage( 'resu', this );
	}

/*======================================================================================*/
void CCaptureWindow::fSoundSettings( void ) {

	SInt32 theErr = noErr;

	BroadcastMessage( 'paus', this );

	/* This is necessary to stop the sound settings panel from using the settings		*/
	/* belonging to a different digitizer!												*/
	if( mSoundChannel != NULL ) {
		fLoadSoundDriver();
		fLoadSettings( kPrefTagAudioChannelData, mSoundChannel );
		}

	if( theErr == noErr ) {
		UDesktop::Deactivate();
		theErr = ::SGSettingsDialog( mSequenceGrabber, mSoundChannel, 0, NULL, 0, NULL, 0 );
		UDesktop::Activate();
		}

	/* Save the settings */
	if( theErr == noErr ) {
		fSaveSettings( kPrefTagAudioChannelData, mSoundChannel );
		fSaveSoundDriver();
		}

	BroadcastMessage( 'resu', this );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fGetFileSpec( const FSSpec *inSpec, Boolean automatic,
	Boolean isTimeLapse, Boolean isSnapshot, FSSpec *outSpec, FSSpec *outSpecNoIndex,
	OSType *outFileType, OSType *outFileCreator ) {

	SInt32 i;
	SInt32 theErr = noErr;

	/* Initialize the output parameters */
	::memset( outSpec, 0, sizeof( FSSpec ) );
	::memset( outSpecNoIndex, 0, sizeof( FSSpec ) );
	*outFileType = 0;
	*outFileCreator = 0;

	/* Get all the file meta information */
	LStr255 name;
	LStr255 suffix;
	LStr255 extension;
	GetDescriptor( name );
	if( !isSnapshot ) {
		suffix = LStr255( kStrings, kStringsMovieSuffix );
		extension = "mov";
		*outFileType = 'MooV';
		*outFileCreator = 'TVOD';
		}
	else {
		suffix = LStr255( kStrings, kStringsSnapshotsSuffix );
		theErr = fGetSnapshotExporterFileMetaData( extension, *outFileType, *outFileCreator );
		}

	if( theErr == noErr )

		if( inSpec != NULL )

			/* AppleScript can provide the spec, in which case inSpec will be non-NULL */
			*outSpec = *inSpec;

		else

			if( automatic != 0 ) {

				/* Naming is automatic. Generate a Spec */
				char path[1024];
				char name[256];
				if( theErr == noErr ) {
					if( !isSnapshot )
						CCaptureRecordSetup::fGetPreferencesDestination( path, name );
					else
						CCaptureSnapshotSetup::fGetPreferencesDestination( path, name );
					theErr = fGetAutomaticFileDestination( path, name,
						isTimeLapse && isSnapshot ? NULL : extension.TextPtr(),
						extension.Length(), isTimeLapse && isSnapshot, outSpec );
					}
				if( theErr == fnfErr ) {
					ResIDT resID = isSnapshot ? PPob_SnapshotAutoFolderNotFound : PPob_RecordAutoFolderNotFound;
					StDialogHandler dialog( resID, this );
					while( dialog.DoDialog() != msg_OK ) {}
					}

				}
			else {

				/* Present standard Save As dialog. Ask user for spec */
				try {

					/* Generate a default filename */
					LStr255 defaultName = name + suffix;
					defaultName.Remove( 32, 255 );
					if( !isTimeLapse || !isSnapshot ) {
						defaultName.Remove( 32 - 1 - extension.Length(), 255 );
						defaultName += LStr255( "." ) + extension;
						}

					/* Ask user for spec */
					PP_StandardDialogs::LFileDesignator designator;
					designator.SetFileType( *outFileType );
					designator.SetFileCreator( *outFileCreator );
					if( designator.AskDesignateFile( defaultName ) )
						designator.GetFileSpec( *outSpec );
					else
						ThrowIfOSErr_( fnfErr );
					}
				catch( PP_PowerPlant::LException err ) {
					theErr = err.GetErrorCode();
					if( theErr != fnfErr ) {
						LStr255 string( theErr );
						::ParamText( string, "\p", "\p", "\p" );
						UModalAlerts::StopAlert( ALRT_CouldNotSave );
						/* Don't tell the user that the recording stopped. He			*/
						/* already knows something went wrong.							*/
						mSuppressUI = true;
						}
					}

				}

	/* Delete the existing file if any, unless we are doing automatic, or timelapse		*/
	/* snapshot. Automatic always save to unique filenames. TimeLapse snapshot always	*/
	/* saves to a unique folder (regardless of the state of automatic).					*/
	if( theErr == noErr )
//		if( !automatic && (!isTimeLapse || !isSnapshot) ) {
		if( !automatic ) {
			theErr = ::FSpDelete( outSpec );
			if( theErr == fnfErr )
				theErr = noErr;
			if( theErr == fBsyErr ) {
				UModalAlerts::StopAlert( ALRT_CouldNotSaveFileBusy );
				/* Don't tell the user that the recording stopped. He already			*/
				/* knows something went wrong.											*/
				mSuppressUI = true;
				}
			}

	/* Create spec for shadow save destination using a unique name and a ".noindex"		*/
	/* suffix. This is the place we will really save the file so that Spotlight			*/
	/* won't try to index the incoming data. We will also reserve the original			*/
	/* file. Clean up tmp files everytime.												*/
	/* This is only needed for movie files.												*/
	if( theErr == noErr )
		if( !isSnapshot ) {
			SInt32 index = 9;
			Boolean found = false;
			while( index >= 0 ) {
				Str255 tmpString;
				::sprintf( (char *)tmpString, "\017.tmp%03d.noindex", index-- );
				FSSpec tmpSpec;
				SInt32 anErr = ::FSMakeFSSpec( outSpec->vRefNum, outSpec->parID, tmpString, &tmpSpec );
				if( anErr == fnfErr ) {
					*outSpecNoIndex = tmpSpec;
					found = true;
					}
				else
					if( anErr == noErr )
						if( ::FSpDelete( &tmpSpec ) == noErr ) {
							*outSpecNoIndex = tmpSpec;
							found = true;
							}
				}
			if( !found )
				theErr = dupFNErr;
			}

	return( theErr );
	}

/*======================================================================================*/
void CCaptureWindow::fStartRecording( UInt32 duration, FSSpec *inSpec,
	sTimeLapseData *timeLapse, Boolean forceAutomatic ) {
/* If fileName is NULL, have user specify the file name.								*/
/* If fileName is not NULL, we will somewhat crudely assume that we are being run from	*/
/* an AppleScript. That being the case, make sure that the user is not bothered by		*/
/* dialogs.																				*/
/* duration is in seconds.																*/
/* inSpec auto																			*/
/*	NULL	F		present std dialog													*/
/*	NULL	T		use autoname spec / save											*/
/*	!NULL	F		use applescript spec / save											*/
/*  !NULL	T		use applescript spec / save											*/

	SInt32 theErr = noErr;

	Boolean isSnapshot = false;
	if( timeLapse != NULL )
		isSnapshot = timeLapse->outputMode == kOutputModeSnapshot;

	SInt32 automatic;
	if( theErr == noErr )
		if( forceAutomatic )
			automatic = 1;
		else
			if( !isSnapshot )
				CCaptureRecordSetup::fGetPreferenceAutomatic( &automatic );
			else
				CCaptureSnapshotSetup::fGetPreferenceAutomatic( &automatic );

	/* Initialize the recording parameters */
	mRecordErr = noErr;
	mIsRecording = false;
	mDuration = duration;
	mStartTime = 0;
	mNextDurationCheck = 0;
	mPreviewStopped = false;
	::memset( &mRecordSpec, 0, sizeof( FSSpec ) );
	::memset( &mRecordSpecNoIndex, 0, sizeof( FSSpec ) );
	mRecordSpecDirID = 0;
	mUsageRecording = false;
	mSuppressUI = inSpec != NULL || automatic != 0;
	mClickWindow = NULL;
	mClickView = NULL;
	mTimePanel = NULL;
	mIsTimeLapse = timeLapse != NULL;
	::memset( &mTimeLapseData, 0, sizeof( sTimeLapseData ) );
	mDecompressionSession = 0;
	mImageDescription = NULL;
	if( mIsTimeLapse ) {
		mTimeLapseData = *timeLapse;
		mTimeLapseNextTime = 0;
		mTimeLapseInterval = mTimeLapseData.intervalValue / 65536.0 *
			(mTimeLapseData.intervalUnit == 0 ? 1 :
			mTimeLapseData.intervalUnit == 1 ? 60 : 3600) * 60.0;
		mMovieResRefNum = 0;
		mMovie = 0;
		mTrack = 0;
		mMedia = 0;
		mTLDecodeDurationPerSample = 0;
		mTimeLapseKeyFrameRateChanged = false;
		mTLSaveKeyFrameRate = 0;
		mTLExporter = NULL;
		}

	/* Allow timelapse to override the duration parameter if desired */
	if( mIsTimeLapse )
		if( mTimeLapseData.hasDuration != 0 )
			mDuration = (UInt32)(mTimeLapseData.durationValue / 65536.0 *
				(mTimeLapseData.durationUnit == 0 ? 1 :
				mTimeLapseData.durationUnit == 1 ? 60 :
				mTimeLapseData.durationUnit == 2 ? 3600 : 86400) + 0.5);

	/* Stop all other movie streams */
	BroadcastMessage( 'paus', this );

	/* Stop the preview */
	if( theErr == noErr ) {
		theErr = ::SGStop( mSequenceGrabber );
		mPreviewStopped = theErr == noErr;
		}

	/* Find where to save the file */
	OSType fileType;
	OSType fileCreator;
	if( theErr == noErr )
		theErr = fGetFileSpec( inSpec, automatic, mIsTimeLapse, isSnapshot, &mRecordSpec,
			&mRecordSpecNoIndex, &fileType, &fileCreator );

	/* Create the destination place holder */
	if( theErr == noErr )
		if( !isSnapshot )
			theErr = ::FSpCreate( &mRecordSpec, '????', '????', smSystemScript );
		else {
			SInt32 dirID;
			if( theErr == noErr )
				theErr = ::FSpDirCreate( &mRecordSpec, smSystemScript, &mRecordSpecDirID );
			}

	#if kNTZPackageNumber == '3Cam'
	if( theErr == noErr ) {
		mLCTReserveSpace = 0;
		CUtilities::fGetFreeDiskSpace( mRecordSpec.vRefNum, &mLCTReserveSpace );
		mLCTReserveSpace = (UInt64)(mLCTReserveSpace * 0.1);
		}
	#endif

	/* Create the noindex shadow file to which we initially save the movie. Its			*/
	/* presence is required for snapshot mode even though we never save anything to it.	*/
	/* We need it to get the SGProc firing.												*/
	if( theErr == noErr )
		if( !mIsTimeLapse )
			theErr = ::CreateMovieFile( &mRecordSpecNoIndex, fileCreator,
				smSystemScript, createMovieFileDontOpenFile |
				createMovieFileDontCreateResFile, NULL, &mMovie );
		else
			if( !isSnapshot ) {
				if( theErr == noErr )
					theErr = ::CreateMovieFile( &mRecordSpecNoIndex, fileCreator,
						smSystemScript,
						createMovieFileDontCreateResFile,
						&mMovieResRefNum, &mMovie );
				if( theErr == noErr ) {
					Rect rect;
					GetPortBounds( GetMacPort(), &rect );
					mTrack = ::NewMovieTrack( mMovie,
						(rect.right - rect.left) * 65536,
						(rect.bottom - rect.top) * 65536,
						kNoVolume );
					if( mTrack == 0 )
						theErr = ::GetMoviesError();
					}
				if( theErr == noErr ) {
					mMedia = ::NewTrackMedia( mTrack, VideoMediaType, kTimeLapseTimeScale, NULL, 0 );
					if( mMedia == 0 )
						theErr = ::GetMoviesError();
					}
				if( theErr == noErr )
					theErr = ::BeginMediaEdits( mMedia );
				}

	/* Tell the sequence grabber where to save frames, if at all */
	if( theErr == noErr )
		if( !mIsTimeLapse ) {
			/* Associate no index shadow file with the sequence grabber */
			theErr = ::SGSetDataOutput( mSequenceGrabber, &mRecordSpecNoIndex,
				seqGrabToDisk | seqGrabAppendToFile | kSeqGrabDontPreAllocateFileSize );
			}
		else {
			/* Tell the sequence grabber to use the SGDataProc instead */
			if( theErr == noErr )
				theErr = ::SGSetDataOutput( mSequenceGrabber, NULL, seqGrabDontMakeMovie );
			if( theErr == noErr )
				theErr = ::SGSetDataProc( mSequenceGrabber, CCaptureWindow::fSGDataProcTimeLapse, (long)this );
			}

	#if kNTZPackageNumber == '3Cam'
	if( g3CamRunMode == k3CamRunModeNormal ) {
	#endif
	/* Create the "Click here to terminate" window. It will be handled in				*/
	/* the main event loop.																*/
	if( theErr == noErr ) {
		ResIDT dialogID = isSnapshot ? PPob_ClickToStopSnapshots : PPob_ClickToStopMovie;
		mClickWindow = (CClickWindow *)CClickWindow::CreateWindow( dialogID, this );
		ThrowIfNULL_( mClickWindow );
		mClickView = dynamic_cast<CClickView *>( mClickWindow->FindPaneByID( 'Clik' ) );
		ThrowIfNULL_( mClickView );
		LBroadcaster *broadcaster = dynamic_cast<LBroadcaster *>( mClickView );
		ThrowIfNULL_( broadcaster );
		broadcaster->AddListener( this );
		if( mDuration > 0 ) {
			mClickWindow->ResizeWindowBy( 0, 30 );
			mTimePanel = dynamic_cast<LStaticText *>( mClickWindow->FindPaneByID( 'time' ) );
			ThrowIfNULL_( mTimePanel );
			fUpdateTimePanel( mDuration );
			}
		mClickWindow->Show();
		}
	#if kNTZPackageNumber == '3Cam'
	}
	#endif

	/* Put the Sequence Grabber in record mode */
	if( theErr == noErr ) {
		if( mVideoChannel != NULL ) {
			::SGGetChannelUsage( mVideoChannel, &mVideoUsage );
			SInt32 usage = kChannelUsageFlagsRecordVideo;
			if( !mRecordVideo )
				usage &= ~seqGrabRecord;
			::SGSetChannelUsage( mVideoChannel, usage );
			}
		if( mSoundChannel != NULL ) {
			::SGGetChannelUsage( mSoundChannel, &mSoundUsage );
			SInt32 usage = kChannelUsageFlagsRecordAudio & ~seqGrabPlayDuringRecord | mSoundDuringRecord;
			if( !mRecordSound || mIsTimeLapse )
				usage &= ~seqGrabRecord;
			::SGSetChannelUsage( mSoundChannel, usage );
			}
		mUsageRecording = true;
		}

	/* If time lapse, change the keyframe rate to 0. This is a limitation of			*/
	/* our implementation -we can only save keyframes to the movie. This				*/
	/* largely comes about because not all digitizers have compressors for				*/
	/* their native format. Notably USBVision.											*/
	if( theErr == noErr )
		if( mIsTimeLapse ) {
			if( theErr == noErr )
				theErr = ::SGGetVideoCompressor( mVideoChannel, &mTLSaveDepth, &mTLSaveCompressor,
					&mTLSaveSpatialQuality, &mTLSaveTemporalQuality, &mTLSaveKeyFrameRate );
			if( theErr == noErr ) {
				theErr = ::SGSetVideoCompressor( mVideoChannel, mTLSaveDepth, mTLSaveCompressor,
					mTLSaveSpatialQuality, mTLSaveTemporalQuality, 0 );
				mTimeLapseKeyFrameRateChanged = theErr == noErr;
				}
			if( theErr == noErr )
				mTLDecodeDurationPerSample = (65536 * kTimeLapseTimeScale +
					mTimeLapseData.outputFrameRate / 2) / mTimeLapseData.outputFrameRate;
			}

	/* If timelapse snapshot, open a graphics exporter for the data */
	if( theErr == noErr )
		if( isSnapshot ) {

			/* Get the preferred format */
			OSType format = kQTFileTypeJPEG;
			if( theErr == noErr )
				CCaptureSnapshotSettings::fGetPreferencesExporterFormat( &format );

			/* Open exporter */
			if( theErr == noErr ) {
				mTLExporter = ::OpenDefaultComponent( GraphicsExporterComponentType, format );
				if( mTLExporter == NULL )
					theErr = -1;
				}

			/* Set the preferred format options */
			QTAtomContainer formatOptions;
			if( theErr == noErr ) {
				CCaptureSnapshotSettings::fGetPreferencesExporterOptions( mTLExporter, format, &formatOptions );
				if( formatOptions == NULL )
					theErr = -1;
				}
			if( theErr == noErr ) {
				theErr = ::GraphicsExportSetSettingsFromAtomContainer( mTLExporter, formatOptions );
				::QTDisposeAtomContainer( formatOptions );
				}
			}

	/* Attempt to recover the preview area obscured by dialogs */
	if( theErr == noErr ) {
		#if TARGET_OS_WIN32
		::UpdatePort( gMonitor );
		#endif
		::SGUpdate( mSequenceGrabber, 0 );
		}

	/* Whole new thing. Let's set a dataproc, tell QuickTime NOT to draw any frames		*/
	/* and handle everything ourselves. Hope this works.								*/
	if( theErr == noErr )
		if( !mIsTimeLapse ) {
			SInt32 usage;
			if( theErr == noErr )
				theErr = ::SGGetChannelUsage( mVideoChannel, &usage );
			if( theErr == noErr )
				theErr = ::SGSetChannelUsage( mVideoChannel, usage & ~seqGrabPlayDuringRecord );
			if( theErr == noErr )
				theErr = ::SGSetDataProc( mSequenceGrabber, CCaptureWindow::fSGDataProcRecord, (long)this );
			}

	/* Stuff from mailing list. This rocks. It changes the sequence grabber's			*/
	/* timebase to that of the audio track. The vdig can use the timebase to			*/
	/* figure out precisely when the audio track started, and can sync the				*/
	/* video frames to that.															*/
	if( theErr == noErr )
		if( mSoundChannel != NULL && mRecordSound ) {
			TimeBase sgTimeBase = NULL;
			TimeBase soundTimeBase = NULL;
			if( ::SGPrepare( mSequenceGrabber, true, true ) == noErr &&
				::SGGetTimeBase( mSequenceGrabber, &sgTimeBase ) == noErr &&
				::SGGetChannelTimeBase( mSoundChannel, &soundTimeBase ) == noErr &&
				soundTimeBase != NULL )
				::SetTimeBaseMasterClock( sgTimeBase, (Component)::GetTimeBaseMasterClock( soundTimeBase ), NULL );
			}

	/* Start the Sequence Grabber recording */
	if( theErr == noErr ) {
		theErr = ::SGStartRecord( mSequenceGrabber );
		mIsRecording = theErr == noErr;
		}
	if( theErr == noErr )
		::GetDateTime( &mStartTime );

	if( theErr != noErr ) {
		mRecordErr = theErr;
		mSuppressUI = true;
		fStopRecording();
		}
	}

/*======================================================================================*/
Boolean CCaptureWindow::fIsRecording( void ) {

	return( mIsRecording );
	}

/*======================================================================================*/
pascal OSErr CCaptureWindow::fSGDataProcRecord( SGChannel c, Ptr p, long len, long *offset,
	long chRefCon, TimeValue time, short writeType, long refCon ) {

	CCaptureWindow *captureWindow = dynamic_cast<CCaptureWindow *>( (CCaptureWindow *)refCon );
	return( captureWindow->fSGDataProcRecord( c, p, len, offset, chRefCon, time, writeType ) );
	}

/*======================================================================================*/
OSErr CCaptureWindow::fSGDataProcRecord( SGChannel c, Ptr p, long len, long *offset,
	long chRefCon, TimeValue time, short writeType ) {

	#pragma unused( writeType )

	ComponentResult theErr = noErr;

	/* If the data is for the video channel, display the frame. Unlike					*/
	/* fSGDataProcTimeLapse() we might in fact see audio samples. Ignore them.			*/
	if( c == mVideoChannel ) {

		/* The first time we enter the callback, set up decompression session for		*/
		/* playback to the window during the recording session.							*/
		/* Reset the frame count.														*/
		/* Get the time scale.															*/
		/* Retrieve the video channel's sample description. The channel returns			*/
		/* a sample description that is appropriate to the type of data being			*/
		/* captured.																	*/
		/* Set up a decompression session.												*/
		if( theErr == noErr )
			if( mDecompressionSession == 0 ) {

				/* Set up timing parameters */
				mFrameCount = 0;
				if( theErr == noErr ) {
					theErr = ::SGGetChannelTimeScale( c, &mTimeScale );
					if( theErr != noErr )
						fprintf( stderr, "SGGetChannelTimeScale failed (%d)", theErr );
					}

				/* Get our image description. It won't change during recording */
				mImageDescription = (ImageDescriptionHandle)::NewHandle( 0 );
				theErr = ::SGGetChannelSampleDescription( c, (Handle)mImageDescription );
				if( theErr != noErr ) {
					fprintf( stderr, "SGGetChannelSampleDescription failed (%d)", theErr );
					::DisposeHandle( (Handle)mImageDescription );
					mImageDescription = NULL;
					}

				/* Start the decompression sequence */
				if( theErr == noErr ) {
					theErr = ::DecompressSequenceBeginS( &mDecompressionSession,
						mImageDescription, p, len, (CGrafPtr)GetMacPort(), NULL, NULL,
						NULL, srcCopy, NULL, codecFlagUseScreenBuffer, codecNormalQuality,
						bestSpeedCodec );
					if( theErr != noErr )
						fprintf( stderr, "fCreateDecompressionSession failed (%d)", theErr );
					}
				}

		/* Decompress the sample data we have been handed straight to the screen (but	*/
		/* asynchronously).																*/
		if( theErr == noErr )
			if( mDecompressionSession != 0 ) {
				CodecFlags outFlags;
				ICMFrameTimeRecord frameTime = {{ 0 }};
				frameTime.recordSize = sizeof( ICMFrameTimeRecord );
				*(TimeValue64 *)&frameTime.value = time;
				frameTime.scale = mTimeScale;
				frameTime.rate = fixed1;
				frameTime.frameNumber = ++mFrameCount;
				frameTime.flags = icmFrameTimeIsNonScheduledDisplayTime;
				theErr = ::DecompressSequenceFrameWhen( mDecompressionSession, p, len,
					0, &outFlags, (ICMCompletionProcRecordPtr)-1, NULL );
				}
		}

	return( theErr );
	}

/*======================================================================================*/
pascal OSErr CCaptureWindow::fSGDataProcTimeLapse( SGChannel c, Ptr p, long len, long *offset,
	long chRefCon, TimeValue time, short writeType, long refCon ) {

	CCaptureWindow *captureWindow = dynamic_cast<CCaptureWindow *>( (CCaptureWindow *)refCon );
	return( captureWindow->fSGDataProcTimeLapse( c, p, len, offset, chRefCon, time, writeType ) );
	}

/*======================================================================================*/
OSErr CCaptureWindow::fSGDataProcTimeLapse( SGChannel c, Ptr p, long len, long *offset,
	long chRefCon, TimeValue time, short writeType ) {
/* We expect to see only video frames in this function because we have turned audio		*/
/* recording off during timelapse recording.											*/

	SInt32 theErr = noErr;

	Boolean isSnapshot = mTimeLapseData.outputMode == kOutputModeSnapshot;

	/* There is some initialization best done here after we see the first frame. If		*/
	/* we are initializing, don't save this frame, but wait for the next call to		*/
	/* this proc, just in case we have been handed a difference frame this time			*/
	/* around.																			*/
	if( mTimeLapseNextTime == 0 ) {

		/* Get our image description. It won't change during recording */
		mImageDescription = (ImageDescriptionHandle)::NewHandle( 0 );
		theErr = ::SGGetChannelSampleDescription( c, (Handle)mImageDescription );
		if( theErr != noErr ) {
			fprintf( stderr, "SGGetChannelSampleDescription failed (%d)", theErr );
			::DisposeHandle( (Handle)mImageDescription );
			mImageDescription = NULL;
			}

		/* Don't try to capture anything until initialization succeeds */
		if( theErr == noErr )
			mTimeLapseNextTime = ::TickCount();
		}

	/* Once initialization has succeeded, we can function as usual */
	else {

		/* If we are passed our snapshot time, take a snapshot and set the next			*/
		/* snapshot time.																*/
		if( ::TickCount() > mTimeLapseNextTime ) {

			if( !isSnapshot ) {
				if( mSystemVersion >= 0x00001030 )
					theErr = ::AddMediaSample2( mMedia, (UInt8 *)p, len,
						mTLDecodeDurationPerSample, 0,
						(SampleDescriptionHandle)mImageDescription, 1, 0, NULL );
				else {
					Handle h;
					if( theErr == noErr ) {
						h = ::NewHandle( len );
						theErr = ::MemError();
						}
					if( theErr == noErr ) {
						::BlockMoveData( p, *h, len );
						theErr = ::AddMediaSample( mMedia, h, 0, len,
							mTLDecodeDurationPerSample,
							(SampleDescriptionHandle)mImageDescription, 1, 0, NULL );
						::DisposeHandle( h );
						}
					}
				}
			else {

				/* Generate a spec */
				FSSpec spec;
				FSRef ref;
				char path[1024];
				char name[256];
				LStr255 extension;
				OSType fileType;
				OSType fileCreator;
				if( theErr == noErr )
					theErr = fGetSnapshotExporterFileMetaData( extension, fileType, fileCreator );
				if( theErr == noErr )
					theErr = ::FSpMakeFSRef( &mRecordSpec, &ref );
				if( theErr == noErr )
					theErr == ::FSRefMakePath( &ref, (UInt8 *)path, 1024 );
				if( theErr == noErr ) {
					LString::CopyPStr( mRecordSpec.name, (StringPtr)name, 256 );
					LString::PToCStr( (StringPtr)name );
					theErr = fGetAutomaticFileDestination( path, name,
						extension.TextPtr(), extension.Length(), mIsTimeLapse, &spec );
					}

				/* Set destination file spec */
				if( theErr == noErr )
					theErr = ::GraphicsExportSetOutputFile( mTLExporter, &spec );

				/* Set destination file type and creator */
				if( theErr == noErr )
					theErr = ::GraphicsExportSetOutputFileTypeAndCreator( mTLExporter, fileType, fileCreator );

				/* Set the image data and decriptor */
				if( theErr == noErr )
					theErr = ::GraphicsExportSetInputPtr( mTLExporter, p, len, mImageDescription );

				/* Save it */
				if( theErr == noErr ) {
					UInt32 length;
					theErr = ::GraphicsExportDoExport( mTLExporter, &length );
					}
				}

			mTimeLapseNextTime += mTimeLapseInterval;
			}

		}

	return( noErr );
	}

/*======================================================================================*/
void CCaptureWindow::fStopRecording( void ) {
/* This has gotten a bit more complicated since the addition of time lapse. Let's just	*/
/* deal with that separately.															*/

	SInt32 theErr = noErr;

	Boolean isSnapshot = mIsTimeLapse && mTimeLapseData.outputMode == kOutputModeSnapshot;

	theErr = mRecordErr;

	/* Determine the output mode */
	SInt32 outputMode = kOutputModeMovie;
	if( mIsTimeLapse &&
		mTimeLapseData.outputMode == kOutputModeSnapshot )
		outputMode = kOutputModeSnapshot;

	/* Need to clear mIsRecording otherwise SpendTime() will try to do things it		*/
	/* should not.																		*/
	if( mIsRecording ) {
		::SGStop( mSequenceGrabber );
		mIsRecording = false;
		}

	/* Dispose of the decompression session used during recording. Turn off the			*/
	/* recording playback proc.															*/
	if( !mIsTimeLapse ) {
		if( mDecompressionSession != 0 ) {
			::CDSequenceEnd( mDecompressionSession );
			mDecompressionSession = 0;
			}
		::SGSetDataProc( mSequenceGrabber, NULL, (long)this );
		}

	/* Turn off time lapse task if any */
	if( mIsTimeLapse )
		::SGSetDataProc( mSequenceGrabber, NULL, (long)this );

	/* Recording and time lapse both use an image description. Don't need it anymore */
	if( mImageDescription != NULL )
		::DisposeHandle( (Handle)mImageDescription );

	/* Exchange the no index file and the destination file in the file system catalog.	*/
	/* Delete the no index file. Don't do this for snapshot time lapse.					*/
	if( !isSnapshot ) {

		/* If building movie with SGDataProc, close it first */
		if( mIsTimeLapse ) {
			SInt32 anErr;
			if( mMedia != 0 )
				anErr = ::EndMediaEdits( mMedia );
			if( mTrack != 0 )
				anErr = ::InsertMediaIntoTrack( mTrack, 0, 0, ::GetMediaDuration( mMedia ), 0x00010000 );
			if( mMovie != 0 && mMovieResRefNum != 0 ) {
				SInt16 resID = movieInDataForkResID;
				anErr = ::AddMovieResource( mMovie, mMovieResRefNum, &resID, NULL );
				}
			if( mMovieResRefNum != 0 )
				anErr = ::CloseMovieFile( mMovieResRefNum );
			anErr = noErr;
			}

		/* Now swap the movies */
		if( mRecordSpec.name[0] != 0 &&
			mRecordSpecNoIndex.name[0] != 0 ) {
			FInfo info;
			FInfo infoNoIndex;
			if( ::FSpGetFInfo( &mRecordSpec, &info ) == noErr &&
				::FSpGetFInfo( &mRecordSpecNoIndex, &infoNoIndex ) == noErr &&
				info.fdType == '????' ) {
				::FSpSetFInfo( &mRecordSpec, &infoNoIndex );
				if( ::FSpExchangeFiles( &mRecordSpecNoIndex, &mRecordSpec ) != noErr ) {
					/* Many volume formats do not support FSpExchangeFiles(). Delete	*/
					/* the target and rename the original.								*/
					FSSpec dstFolder;
					if( CUtilities::fGetParentFolder( &mRecordSpec, &dstFolder ) == noErr ) {
						::FSpDelete( &mRecordSpec );
						::FSpCatMove( &mRecordSpecNoIndex, &dstFolder );
						mRecordSpecNoIndex.parID = mRecordSpec.parID;
						::FSpRename( &mRecordSpecNoIndex, mRecordSpec.name );
						}
					}
				else
					::FSpDelete( &mRecordSpecNoIndex );

				/* Support AVID / Pinnacle requested feature. Add the clip to iTunes	*/
				/* if desired.															*/
				SInt32 addClipsToITunes = 0;
				gPreferences.fGetPreference( kPrefTagMisc, kPrefMiscAddClipsToITunes, &addClipsToITunes );
				if( addClipsToITunes != 0 )
					fAddClipToITunes( &mRecordSpec );
				}
			}
		}

	/* Dispose of the "Click here to terminate" window */
	if( mClickWindow != NULL )
		delete mClickWindow;

	/* Close the graphics exporter for timelapse snapshot */
	if( mIsTimeLapse )
		if( isSnapshot )
			if( mTLExporter != NULL )
				::CloseComponent( mTLExporter );

	/* If timelapse, restore the key frame rate. The other values weren't changed */
	if( mIsTimeLapse )
		if( mTimeLapseKeyFrameRateChanged )
			::SGSetVideoCompressor( mVideoChannel, mTLSaveDepth, mTLSaveCompressor,
				mTLSaveSpatialQuality, mTLSaveTemporalQuality, mTLSaveKeyFrameRate );

	/* Put the Sequence Grabber back in preview mode */
	if( mUsageRecording ) {
		if( mVideoChannel != NULL )
			::SGSetChannelUsage( mVideoChannel, mVideoUsage );
		if( mSoundChannel != NULL )
			::SGSetChannelUsage( mSoundChannel, mSoundUsage );
		}

	if( mPreviewStopped )
		theErr = ::SGStartPreview( mSequenceGrabber );

	BroadcastMessage( 'resu', this );

	/* If reasonable to do so, let the user know that recording or timelapse snapshots	*/
	/* has stopped.																		*/
	if( !mSuppressUI ) {
		ResIDT dialogID = isSnapshot ? PPob_SnapshotsProblem : PPob_RecordingProblem;
		switch( theErr ) {
			case noErr:
				dialogID = isSnapshot ? PPob_SnapshotsTaken : PPob_MovieRecorded;
				break;
			case dskFulErr:
				dialogID = isSnapshot ? PPob_SnapshotsDiskFull : PPob_RecordingDiskFull;
				break;
			}
		StDialogHandler dialog( dialogID, this );
		if( dialogID == PPob_RecordingProblem ||
			dialogID == PPob_SnapshotsProblem ) {
			LStaticText *text = dynamic_cast<LStaticText *>( dialog.GetDialog()->FindPaneByID( 'ERRn' ) );
			ThrowIfNULL_( text );
			text->SetDescriptor( LStr255( theErr ) );
			}
		if( dialogID == PPob_MovieRecorded ) {
			SInt32 systemVersion;
			if( ::Gestalt( gestaltSystemVersion, &systemVersion ) == noErr && systemVersion < 0x00001020 ) {
				LPushButton *button = dynamic_cast<LPushButton *>( dialog.GetDialog()->FindPaneByID( 'open' ) );
				if( button != NULL )
					button->Hide();
				}
			}
		Boolean finished = false;
		while( !finished ) {
			MessageT message = dialog.DoDialog();
			switch( message ) {
				case msg_OK:
					finished = true;
					break;
				case 'open':
					fOpenMovie( &mRecordSpec );
					finished = true;
					break;
				}
			}
		}

	/* Copied straight from constructor */
	mRecordErr = noErr;
	mIsRecording = false;
	mDuration = 0;
	mStartTime = 0;
	mNextDurationCheck = 0;
	mPreviewStopped = false;
	::memset( &mRecordSpec, 0, sizeof( FSSpec ) );
	::memset( &mRecordSpecNoIndex, 0, sizeof( FSSpec ) );
	mRecordSpecDirID = 0;
	mUsageRecording = false;
	mSuppressUI = false;
	mClickWindow = NULL;
	mClickView = NULL;
	mTimePanel = NULL;
	mIsTimeLapse = false;
	::memset( &mTimeLapseData, 0, sizeof( sTimeLapseData ) );
	mTimeLapseNextTime = 0;
	mTimeLapseInterval = 0;
	mMovieResRefNum = 0;
	mMovie = 0;
	mTrack = 0;
	mMedia = 0;
	mImageDescription = NULL;
	mTLDecodeDurationPerSample = 0;
	mTimeLapseKeyFrameRateChanged = false;
	mTLSaveKeyFrameRate = 0;
	mTLExporter = NULL;
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fAddClipToITunes( const FSSpec *movie ) {

	SInt32 theErr = noErr;

	FSRef movieRef;
	if( theErr == noErr )
		theErr = ::FSpMakeFSRef( movie, &movieRef );

	char path[1024];
	if( theErr == noErr )
		theErr = ::FSRefMakePath( &movieRef, (UInt8 *)path, 1024 );

	if( theErr == noErr ) {
		char buffer[1500];
		::sprintf( buffer, "osascript "
			"-e 'tell application \"iTunes\"' "
			"-e 'add(POSIX file \"%s\")' "
			"-e 'end tell'", path );
		::system( buffer );
		}

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fLoadSettings( SInt32 inPrefTag, SGChannel channel ) {

	SInt32 theErr = noErr;

#if 0
	/* Use unique settings for each of the possible video inputs and formats. We will	*/
	/* only do this for video for now, but we could do it for audio as well.			*/
	SInt32 index = 0;
	if( theErr == noErr )
		if( inPrefTag == kPrefTagVideoChannelData )
			if( mVideoDigitizer != NULL ) {
				SInt16 numInputs;
				if( ::VDGetNumberOfInputs( mVideoDigitizer, &numInputs ) == noErr ) {
					numInputs++;
					SInt16 input;
					if( ::VDGetInput( mVideoDigitizer, &numInputs ) == noErr ) {
						Rect rect;
						if( ::SGGetSrcVideoBounds( mVideoChannel, &rect ) == noErr ) {
							SInt16 standard =
							}
						}
					}
				}

#endif

	/* Load the settings data */
	SInt32 length;
	if( theErr == noErr )
		theErr = gPreferences.fGetPreferenceDataSize( inPrefTag, mPreferenceID, &length );
	Handle h = NULL;
	if( theErr == noErr ) {
		h = ::NewHandle( length );
		if( h == NULL )
			theErr = memFullErr;
		}
	if( theErr == noErr ) {
		::HLock( h );
		theErr = gPreferences.fGetPreference( inPrefTag, mPreferenceID, *h, &length );
		::HUnlock( h );
		}

	/* Apply the settings data */
	UserData userData = NULL;
	if( theErr == noErr )
		theErr = ::NewUserDataFromHandle( h, &userData );
	if( theErr == noErr )
		theErr = ::SGSetChannelSettings( mSequenceGrabber, channel, userData, 0 );

	/* Special case for audio settings. This picks up any differences between the		*/
	/* default sound-playthrough mode and the saved mode.								*/
	if( theErr == noErr )
		if( inPrefTag == kPrefTagAudioChannelData ) {
			SInt32 soundUsage;
			::SGGetChannelUsage( mSoundChannel, &soundUsage );
			mSoundDuringPreview = soundUsage & seqGrabPreview;
			mSoundDuringRecord = soundUsage & seqGrabPlayDuringRecord;
			}

	/* Clean up */
	if( userData != NULL )
		::DisposeUserData( userData );
	if( h != NULL )
		::DisposeHandle( h );

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fSaveSettings( SInt32 inPrefTag, SGChannel channel ) {

	SInt32 theErr = noErr;

	/* Get the settings data */
	UserData userData = NULL;
	if( theErr == noErr )
		theErr = ::SGGetChannelSettings( mSequenceGrabber, channel, &userData, 0 );

	/* Special case for audio settings. This picks up any changes the user made			*/
	/* to the sound play-through mode using the sound settings dialog.					*/
	if( theErr == noErr )
		if( inPrefTag == kPrefTagAudioChannelData ) {
			SInt32 soundUsage;
			::SGGetChannelUsage( mSoundChannel, &soundUsage );
			mSoundDuringPreview = soundUsage & seqGrabPreview;
			mSoundDuringRecord = soundUsage & seqGrabPlayDuringRecord;
			}

	Handle h = NULL;
	if( theErr == noErr ) {
		h = ::NewHandle( 512 );
		if( h == NULL )
			theErr = memFullErr;
		}
	if( theErr == noErr )
		theErr = ::PutUserDataIntoHandle( userData, h );
	if( theErr == noErr ) {
		::HLock( h );
		gPreferences.fSetPreference( inPrefTag, mPreferenceID, *h, ::GetHandleSize( h ) );
		::HUnlock( h );
		}
	if( h != NULL )
		::DisposeHandle( h );
	if( userData != NULL )
		::DisposeUserData( userData );

	return( theErr );
	}

/*======================================================================================*/
void CCaptureWindow::fPreconfigureNewDevice( sDevicePtr device ) {
/* Called whenever a new device might be present. Preconfigure certain video channel	*/
/* options in specific devices.	the device must basically be up and running at this		*/
/* point.																				*/

	SInt32 saveUsage;
	Boolean saveVideoSettings = false;

	switch( mNTZDeviceType ) {

		case kNTZDeviceType100x:
			/* Set the recording key frame rate to 15 */
			saveVideoSettings = ::SGSetVideoCompressor( mVideoChannel, 0, 0,
				codecNormalQuality, codecNormalQuality, 15 ) == noErr;
			break;

		case kNTZDeviceType28xx:
			/* Set the recording compressor to JPEG */
			saveVideoSettings = ::SGSetVideoCompressorType( mVideoChannel,
				kJPEGCodecType ) == noErr;
			break;

		default:
			/* Look for Apple's USB Video Class Video driver. It sets the default		*/
			/* compressor to "Y'CbCr 4:2:2 -yuyv". But when a recording is made to		*/
			/* that format, the size is always 640 x 480 for some reason. So switch to	*/
			/* jpeg so that we can record in whatever size we like.						*/
			if( device->subType == 'usbv' &&
				device->manufacturer == 'appl' )
				saveVideoSettings = ::SGSetVideoCompressorType( mVideoChannel,
					kJPEGCodecType ) == noErr;
			break;
		}

	if( saveVideoSettings )
		fSaveSettings( kPrefTagVideoChannelData, mVideoChannel );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fSetSoundInputDriver( ConstStr255Param name ) {

	SInt32 theErr = noErr;

	if( mSoundChannel != NULL ) {
		Boolean done = false;
		SInt32 refNum = ::SGGetSoundInputDriver( mSoundChannel );
		if( refNum != 0 ) {
			Str255 current;
			if( ::SPBGetDeviceInfo( refNum, siDeviceName, current ) == noErr )
				done = ::memcmp( name, current, name[0] + 1 ) == 0;
			}
		if( !done ) {
			theErr = ::SGSetSoundInputDriver( mSoundChannel, name );
			done = true;
			}
		}

	return( theErr );
	}

/*======================================================================================*/
void CCaptureWindow::fLoadSoundDriver( void ) {
/* Uses preferences to select the sound driver for this device.							*/

	if( mSoundChannel != NULL ) {
		SInt32 length;
		Str255 name;
		if( gPreferences.fGetPreferenceDataSize( kPrefTagAudioDevice, mPreferenceID, &length ) == noErr &&
			gPreferences.fGetPreference( kPrefTagAudioDevice, mPreferenceID, (char *)name, &length ) == noErr )
			fSetSoundInputDriver( name );
		}
	}

/*======================================================================================*/
void CCaptureWindow::fSaveSoundDriver( void ) {
/* Uses preferences to save the sound driver for this device.							*/

	if( mSoundChannel != NULL ) {
		Str255 name;
		SInt32 refNum = ::SGGetSoundInputDriver( mSoundChannel );
		if( ::SPBGetDeviceInfo( refNum, siDeviceName, name ) == noErr )
			gPreferences.fSetPreference( kPrefTagAudioDevice, mPreferenceID, (char *)name, 256 );
		}
	}

/*======================================================================================*/
void CCaptureWindow::fSnapshotSmall( FSSpec *spec, Boolean forceAutomatic ) {

	SInt32 theErr = noErr;

	if( mSequenceGrabber != NULL ) {

		PicHandle picture = NULL;
		if( theErr == noErr ) {
			FocusDraw();
			::SGPause( mSequenceGrabber, seqGrabPause );
			::SGGrabPict( mSequenceGrabber, &picture, NULL, 0, grabPictOffScreen | grabPictCurrentImage );
			::SGPause( mSequenceGrabber, seqGrabUnpause );
			if( picture == NULL )
				::SGGrabPict( mSequenceGrabber, &picture, NULL, 0, grabPictOffScreen );
			if( picture == NULL )
				theErr = -1;
			}

		Boolean mute = true;
		#if kNTZPackageNumber == 'agsp'
		mute = false;
		#endif
		if( theErr == noErr )
			theErr = fSnapshotSave( picture, spec, forceAutomatic, mute );

		if( picture != NULL )
			::KillPicture( picture );
		}
	}

#if kNTZPackageNumber == '3Cam'
/*======================================================================================*/
void CCaptureWindow::fSnapshotSmallLCTFSCopy( void ) {
/* Place current frame on the clipboard, present the required dialog and quit.			*/

	SInt32 i;
	SInt32 theErr = noErr;

	if( mSequenceGrabber != NULL ) {

		PicHandle picture = NULL;
		if( theErr == noErr ) {
			FocusDraw();

			Rect rect;
			::SetRect( &rect, 0, 0, mWidth, mHeight );
			::SGStop( mSequenceGrabber );
			mCaptureWidth = rect.right - rect.left;
			mCaptureHeight = rect.bottom - rect.top;
			::SGSetChannelBounds( mVideoChannel, &rect );
			::SGStartPreview( mSequenceGrabber );

			/* Some devices need a little time to get back on their feet. Some in fact	*/
			/* require extra time to settle in their exposure levels -the Apple iSight	*/
			/* is a good example of this. One second is probably barely enough for this	*/
			/* to happen :-(															*/
			RgnHandle clipRgn = ::NewRgn();
			RgnHandle rgn = ::NewRgn();
			CGrafPtr gWorld;
			::SGGetGWorld( mSequenceGrabber, &gWorld, NULL );
			::GetPortVisibleRegion( gWorld, clipRgn );
			::SetPortVisibleRegion( gWorld, rgn );
			for( i = 0; i < 100; i++ ) {
				::usleep( 10000 );
				::SGIdle( mSequenceGrabber );
				}
			::SetPortVisibleRegion( gWorld, clipRgn );
			::DisposeRgn( clipRgn );
			::DisposeRgn( rgn );

			::SGPause( mSequenceGrabber, seqGrabPause );
			::SGGrabPict( mSequenceGrabber, &picture, NULL, 0, grabPictOffScreen | grabPictCurrentImage );
			if( picture == NULL )
				::SGGrabPict( mSequenceGrabber, &picture, NULL, 0, grabPictOffScreen );
			::SGPause( mSequenceGrabber, seqGrabUnpause );

			if( picture == NULL ) {
				::SysBeep( 1 );
				theErr = -1;
				}
			}

		if( theErr == noErr )
			UScrap::SetData( 'PICT', (Handle)picture, true );

		if( picture != NULL )
			::KillPicture( picture );

		if( theErr == noErr ) {
			StopIdling(); // No need for any more idle activity, including obscuring the cursor
			StDialogHandler dialog( PPob_LogicalChoicePictureOnClip, this );
			while( dialog.DoDialog() != msg_OK ) {}
			fEndFullScreen();
			}

		if( theErr != noErr )
			::SGPause( mSequenceGrabber, seqGrabUnpause );
		}
	}

/*======================================================================================*/
Boolean CCaptureWindow::AttemptQuit( long inSaveOption ) {
/* Another beautiful hack for LCT.														*/

	if( g3CamRunMode == k3CamRunModeVideoRecorder ) {
		if( mIsRecording ) {
			mSuppressUI = true;
			fStopRecording();
			::FSpDelete( &mRecordSpec );
			}
		}

	return( LWindow::AttemptQuit( inSaveOption ) );
	}

/*======================================================================================*/
void CCaptureWindow::AttemptClose( void ) {
/* Another beautiful hack for LCT.														*/
/* Note that I added k3CamRunModeFullScreen because of a bug Nic at KAV found where		*/
/* full screen still showed the title bar. Now closing with the title bar will quit		*/
/* the app through here as well. The bug has been corrected, so g3CamRunMode can't be	*/
/* k3CamRunModeFullScreen in theory, but there you go.									*/

	if( g3CamRunMode == k3CamRunModeFullScreen ||
		g3CamRunMode == k3CamRunModeVideoRecorder )
		LCommander::GetTopCommander()->ObeyCommand( cmd_Quit, NULL );

	LWindow::AttemptClose();
	}

#endif
#if kNTZLicenseeNumber != 'eEm '
/*======================================================================================*/
void CCaptureWindow::fSnapshotLarge( FSSpec *spec, Boolean forceAutomatic ) {
/* Only available with Nogatech devices.												*/

	SInt32 theErr = noErr;

	if( mVideoDigitizer != NULL ) {

		/* Since AppleEvents can get us in here, we need to check that the device		*/
		/* supports the functionality instead of just relying on the fact that the		*/
		/* Large Snapshot menu item is disabled for non-Nogatech devices.				*/
		if( theErr == noErr )
			if( mNTZDeviceType != kNTZDeviceType100x )
				theErr = -3;

		/* Create a GWorld in which to store the picture (big enough for PAL) */
		CGrafPtr world = NULL;
		if( theErr == noErr ) {
			Rect rect;
			::SetRect( &rect, 0, 0, 720, 288 * 2 );
			theErr = ::NewGWorld( &world, 32, &rect, NULL, NULL, 0 );
			}

		/* Take a snapshot */
		Rect bounds;
		if( theErr == noErr )
			theErr = ::fSnapshotTake( mVideoDigitizer, world, &bounds );

		/* Create a PICT handle */
		PicHandle picture = NULL;
		if( theErr == noErr ) {
			CGrafPtr saveWorld;
			GDHandle saveDevice;
			::GetGWorld( &saveWorld, &saveDevice );
			::SetGWorld( world, NULL );
			PixMapHandle map = ::GetGWorldPixMap( world );
			UInt8 state = ::GetPixelsState( map );
			::LockPixels( map );
			::ClipRect( &bounds );
			OpenCPicParams ocpp;
			ocpp.srcRect = bounds;
			ocpp.hRes = 72 << 16;
			ocpp.vRes = 72 << 16;
			ocpp.version = -2;
			ocpp.reserved1 = 0;
			ocpp.reserved2 = 0;
			picture = ::OpenCPicture( &ocpp );
			if( picture != NULL ) {
				::CopyBits( (BitMap *)*map, (BitMap *)*map, &bounds, &bounds, srcCopy, 0 );
				::ClosePicture();
				}
			else
				theErr = -2;
			::SetPixelsState( map, state );
			::SetGWorld( saveWorld, saveDevice );
			}

		/* Save the picture */
		Boolean mute = true;
		#if kNTZPackageNumber == 'agsp'
		mute = false;
		#endif
		if( theErr == noErr )
			theErr = fSnapshotSave( picture, spec, forceAutomatic, mute );

		if( picture != NULL )
			::KillPicture( picture );
		if( world != NULL )
			::DisposeGWorld( world );
		}
	}

#endif
/*======================================================================================*/
SInt32 CCaptureWindow::fSnapshotSave( PicHandle picture, const FSSpec *inSpec,
	Boolean forceAutomatic, Boolean mute ) {
/* inSpec  auto    useQT																*/
/*	NULL	F		F		present std dialog											*/
/*	NULL	F		T		present QT dialog get spec / save							*/
/*	NULL	T		F		use autoname spec / save									*/
/*	NULL	T		T		use autoname spec / save									*/
/*	!NULL	F		F		use applescript spec / save									*/
/*	!NULL	F		T		use applescript spec / save									*/
/*  !NULL	T		F		use applescript spec / save									*/
/*	!NULL	T		T		use applescript spec / save									*/

	SInt32 theErr = noErr;

	/* Play a cute sound, if available */
	if( !mute ) {
		if( gClickSound == NULL ) {
			gClickSound = ::GetResource( 'snd ', 17000 );
			::DetachResource( gClickSound );
			}
		if( gClickSound != NULL ) {
			if( gClickSoundChannel == NULL )
				if( ::SndNewChannel( &gClickSoundChannel, sampledSynth, initMono, 0 ) != noErr )
					if( gClickSoundChannel != NULL ) {
						::SndDisposeChannel( gClickSoundChannel, true );
						gClickSoundChannel = NULL;
						}
			if( gClickSoundChannel != NULL ) {
				::HLock( gClickSound );
				::SndPlay( gClickSoundChannel, (SndListHandle)gClickSound, true );
				::HUnlock( gClickSound );
				}
			}
		}

	SInt32 automatic;
	if( theErr == noErr )
		if( forceAutomatic )
			automatic = 1;
		else
			CCaptureSnapshotSetup::fGetPreferenceAutomatic( &automatic );

	SInt32 useQuickTimeDialog;
	if( theErr == noErr )
		CCaptureSnapshotSettings::fGetPreferenceUseQuickTimeDialog( &useQuickTimeDialog );

	/* Take care of the QuickTime dialog special case. Would like to deprecate this */
	if( theErr == noErr )
		if( inSpec == NULL &&
			automatic == 0 &&
			useQuickTimeDialog != 0 ) {

			/* Let QT handle everything, including saving the file. Note that	*/
			/* spec will not be set here so we won't try to save the file		*/
			/* again later on.													*/

			/* Open the PICT graphics importer */
			GraphicsImportComponent importer = NULL;
			if( theErr == noErr ) {
				importer = ::OpenDefaultComponent( GraphicsImporterComponentType, kQTFileTypePicture );
				if( importer == NULL )
					theErr = -1;
				}

			/* Convert the picture handle into a PICT file (still in a handle)	*/
			/* by adding a 512-byte header to the start.						*/
			Handle handle = NULL;
			if( theErr == noErr ) {
				handle = ::NewHandleClear( 512 );
				if( handle == NULL )
					theErr = memFullErr;
				}
			if( theErr == noErr )
				theErr = ::HandAndHand( (Handle)picture, handle );

			/* Attach the pict to it */
			if( theErr == noErr )
				theErr = ::GraphicsImportSetDataHandle( importer, handle );

	// This is pointless. The settings are ignored by GraphicsImportDoExportImageFileDialog()
	//		/* Apply user settings. if any */
	//		OSType format = kQTFileTypePicture;
	//		if( theErr == noErr ) {
	//			QTAtomContainer formatOptions;
	//			CCaptureSnapshotSettings::fGetPreferencesImporterFormat( &format );
	//			CCaptureSnapshotSettings::fGetPreferencesImporterOptions( importer, format, &formatOptions );
	//			if( formatOptions != NULL ) {
	//				::GraphicsImportSetExportSettingsFromAtomContainer( importer, formatOptions );
	//				::QTDisposeAtomContainer( formatOptions );
	//				}
	//			}

			/* Save it */
			OSType format;
			if( theErr == noErr ) {
				FSSpec nameSpec;
				LStr255 name;
				GetDescriptor( name );
				LString::CopyPStr( name, nameSpec.name, sizeof( StrFileName ) );
				UDesktop::Deactivate();
				theErr = ::GraphicsImportDoExportImageFileDialog( importer, &nameSpec, NULL, NULL, &format, NULL, NULL );
				UDesktop::Activate();
				}

	// Equally pointless
	//		/* Save user settings */
	//		if( theErr == noErr ) {
	//			QTAtomContainer formatOptions;
	//			if( ::GraphicsImportGetExportSettingsAsAtomContainer( importer, &formatOptions ) == noErr ) {
	//				CCaptureSnapshotSettings::fSetPreferencesImporterFormat( format );
	//				CCaptureSnapshotSettings::fSetPreferencesImporterOptions( format, formatOptions );
	//				::QTDisposeAtomContainer( formatOptions );
	//				}
	//			}

			if( handle != NULL )
				::DisposeHandle( handle );
			if( importer != NULL )
				::CloseComponent( importer );

			return( theErr );
			}

	/* Find where to save the file */
	FSSpec spec;
	FSSpec specNoIndex;
	OSType fileType;
	OSType fileCreator;
	if( theErr == noErr ) {
		theErr = fGetFileSpec( inSpec, automatic, false, true, &spec, &specNoIndex, &fileType, &fileCreator );
		Activate();
		}

	/* Save the file */
	if( theErr == noErr ) {

		/* Get the preferred format */
		OSType format = kQTFileTypeJPEG;
		if( theErr == noErr )
			CCaptureSnapshotSettings::fGetPreferencesExporterFormat( &format );

		/* Open exporter */
		GraphicsExportComponent exporter = NULL;
		if( theErr == noErr ) {
			exporter = ::OpenDefaultComponent( GraphicsExporterComponentType, format );
			if( exporter == NULL )
				theErr = -1;
			}

		/* Set the preferred format options */
		QTAtomContainer formatOptions;
		if( theErr == noErr ) {
			CCaptureSnapshotSettings::fGetPreferencesExporterOptions( exporter, format, &formatOptions );
			if( formatOptions == NULL )
				theErr = -1;
			}
		if( theErr == noErr ) {
			theErr = ::GraphicsExportSetSettingsFromAtomContainer( exporter, formatOptions );
			::QTDisposeAtomContainer( formatOptions );
			}

		/* Set destination file spec */
		if( theErr == noErr )
			theErr = ::GraphicsExportSetOutputFile( exporter, &spec );

		/* Set destination file type and creator */
		if( theErr == noErr )
			theErr = ::GraphicsExportSetOutputFileTypeAndCreator( exporter, fileType, fileCreator );

		/* Set the picture */
		if( theErr == noErr )
			theErr = ::GraphicsExportSetInputPicture( exporter, picture );

		/* Save it */
		if( theErr == noErr ) {
			UInt32 length;
			theErr = ::GraphicsExportDoExport( exporter, &length );
			}

		if( exporter != NULL )
			::CloseComponent( exporter );

		}

	return( theErr );
	}

/*======================================================================================*/
SInt32 CCaptureWindow::fGetSnapshotExporterFileMetaData( LStr255 &extension, OSType &fileType, OSType &fileCreator ) {

	SInt32 theErr = noErr;

	extension = "";
	fileType = 0;
	fileCreator = 0;

	/* Get the preferred format */
	OSType format = kQTFileTypeJPEG;
	if( theErr == noErr )
		CCaptureSnapshotSettings::fGetPreferencesExporterFormat( &format );

	/* Open exporter */
	GraphicsExportComponent exporter = NULL;
	if( theErr == noErr ) {
		exporter = ::OpenDefaultComponent( GraphicsExporterComponentType, format );
		if( exporter == NULL )
			theErr = -1;
		}

	OSType extensionOSType;
	if( theErr == noErr )
		theErr = ::GraphicsExportGetDefaultFileNameExtension( exporter, &extensionOSType );
	if( theErr == noErr ) {
		extensionOSType = EndianU32_BtoN( extensionOSType );
		SInt32 index = 0;
		while( index < 4 && ::isgraph( ((char *)&extensionOSType)[index] ) ) {
			extension += (char)::tolower( ((char *)&extensionOSType)[index] );
			index++;
			}
		}

	if( theErr == noErr )
		theErr = ::GraphicsExportGetDefaultFileTypeAndCreator( exporter, &fileType, &fileCreator );

	if( exporter != NULL )
		::CloseComponent( exporter );

	return( theErr );
	}

///*======================================================================================*/
//void CCaptureWindow::fGetImporterExtension( GraphicsImportComponent importer, OSType format, char *extension ) {
///* Insanely complicated way of finding the appropriate file extension for the current	*/
///* importer export format. No longer used -this did not fix the problem it was			*/
///* intended to address.																	*/
//
//	SInt32 i;
//
//	extension[0] = 0;
//
//	QTAtomContainer typeList;
//	if( ::GraphicsImportGetExportImageTypeList( importer, &typeList ) == noErr ) {
//		SInt32 numTypes = ::QTCountChildrenOfType( typeList,
//			kParentAtomIsContainer, kGraphicsExportGroup );
//		Boolean found = false;
//		for( i = 1; i <= numTypes && !found; i++ ) {
//			QTAtom groupAtom = ::QTFindChildByIndex( typeList,
//				kParentAtomIsContainer, kGraphicsExportGroup, i, NULL );
//			if( groupAtom != 0 ) {
//				QTAtom fileTypeAtom = ::QTFindChildByIndex( typeList,
//					groupAtom, kGraphicsExportFileType, 1, NULL );
//				if( fileTypeAtom != 0 ) {
//					OSType fileType;
//					if( ::QTCopyAtomDataToPtr( typeList, fileTypeAtom, false,
//						sizeof( OSType ), &fileType, NULL ) == noErr ) {
//						fileType = EndianU32_BtoN( fileType );
//						found = fileType == format;
//						if( found ) {
//							QTAtom fileExtnAtom = ::QTFindChildByIndex( typeList,
//								groupAtom, kGraphicsExportExtension, 1, NULL );
//							if( fileExtnAtom != 0 ) {
//								OSType fileExtn = 0;
//								if( ::QTCopyAtomDataToPtr( typeList, fileExtnAtom, true,
//									sizeof( OSType ), &fileExtn, NULL ) == noErr ) {
//									fileExtn = EndianU32_BtoN( fileExtn );
//									SInt32 index = 0;
//									this has endian issues
//									while( index < 4 && ::isgraph( ((char *)&fileExtn)[index] ) ) {
//										extension[index] = ::tolower( ((char *)&fileExtn)[index] );
//										index++;
//										}
//									extension[index] = 0;
//									}
//								}
//							}
//						}
//					}
//				}
//			}
//		::QTDisposeAtomContainer( typeList );
//		}
//	}
//
/*======================================================================================*/
SInt32 CCaptureWindow::fGetAutomaticFileDestination( char *path, char *name,
	char *extension, SInt32 extensionLength, Boolean reset, FSSpec *spec ) {

	SInt32 theErr = noErr;

	FSRef dir;
	if( theErr == noErr ) {
		Boolean isDirectory;
		theErr = ::FSPathMakeRef( (UInt8 *)path, &dir, &isDirectory );
		if( theErr == noErr )
			if( !isDirectory )
				theErr = fnfErr;
		}
	char newName[256];
	if( theErr == noErr ) {
		FSRef ref;
		char separator = ':';
		SInt32 systemVersion;
		if( ::Gestalt( gestaltSystemVersion, &systemVersion ) == noErr &&
			systemVersion >= 0x00001000 )
			separator = '/';
		SInt32 length = ::strlen( path );
		path[length] = separator;
		path[length + 1] = 0;
		Boolean found = false;
		static UInt32 gNumber = 0;
		if( reset )
			gNumber = 0;
		while( !found && gNumber < 9999 ) {
			char buffer[1024];
			char shortName[256];
			::strcpy( shortName, name );
			shortName[31 - 6 - extensionLength] = 0;
			::sprintf( &newName[1], "%s_%.4d", shortName, gNumber++ );
			if( extension != NULL && extensionLength > 0 ) {
				::strcat( &newName[1], "." );
				::strncat( &newName[1], extension, extensionLength );
				}
			::sprintf( buffer, "%s%s", path, &newName[1] );
			found = ::FSPathMakeRef( (UInt8 *)buffer, &ref, NULL ) == fnfErr;
			}
		if( !found )
			theErr = -1;
		}
	FSCatalogInfo info;
	if( theErr == noErr )
		theErr = ::FSGetCatalogInfo( &dir, kFSCatInfoNodeID, &info, NULL, spec, NULL );

	if( theErr == noErr ) {
		newName[0] = ::strlen( &newName[1] );
		::FSMakeFSSpec( spec->vRefNum, info.nodeID, (StringPtr)newName, spec );
		}

	return( theErr );
	}

/*======================================================================================*/
void CCaptureWindow::fUpdateTimePanel( UInt32 timeRemaining ) {

	UInt32 hours = timeRemaining / 3600;
	UInt32 minutes = (timeRemaining - hours * 3600) / 60;
	UInt32 seconds = timeRemaining - hours * 3600 - minutes * 60;
	char buffer[256];
	::sprintf( &buffer[1], "%02d:%02d:%02d", hours, minutes, seconds );
	buffer[0] = ::strlen( &buffer[1] );
	mTimePanel->SetDescriptor( (StringPtr)buffer );
	mClickWindow->Refresh();
	}

/*======================================================================================*/
void CCaptureWindow::fPause( Boolean pause, CCaptureWindow *who ) {
/* This is called by the application object when a monitor tells it that all activity	*/
/* everywhere must temporarily be paused or resumed. This should stop the audio device	*/
/* busy errors and ensure maximum recording performance.								*/

	if( pause ) {
		::SGPause( mSequenceGrabber, seqGrabPause );
		if( who != this )
			::SGStop( mSequenceGrabber );
		}
	else {
		if( who != this )
			::SGStartPreview( mSequenceGrabber );
		::SGPause( mSequenceGrabber, seqGrabUnpause );
		}
	}

/*======================================================================================*/
void CCaptureWindow::fActivateSound( void ) {
/* Called on every window either at startup after all windows have been created, or as	*/
/* each new window is opened. Once the sound has been activated, it stays that way		*/
/* until the window is closed.															*/

	mSoundActive = true;
	}

/*======================================================================================*/
void CCaptureWindow::fStopSoundPreview( void ) {
/* This is called by the application object when a monitor tells it that other monitors	*/
/* need to be silent. This will generally occur during a monitor's ActivateSelf()		*/
/* method.																				*/

	if( mSoundChannel != NULL )
		if( mSoundActive )
			fSGSetChannelUsage( mSoundChannel, 0 );
	}

/*======================================================================================*/
void CCaptureWindow::fStartSoundPreview( void ) {
/* Called from ActivateSelf() when a window is activated. By definition the window must	*/
/* be in preview mode.																	*/

	if( mSoundChannel != NULL )
		if( mSoundActive ) {
			SInt32 usage = kChannelUsageFlagsPreviewAudio & ~seqGrabPreview | mSoundDuringPreview;
			fSGSetChannelUsage( mSoundChannel, usage );
			}
	}

/*======================================================================================*/
Boolean fOptionPressed( void ) {

	return( ::fKeyIsDown( 0x3A ) );
	}

/*======================================================================================*/
static Boolean fKeyIsDown( short keyCode ) {
/* Looks at the key map and returns true if the specified key is down.					*/
/* See page 191 of IM V for key codes.													*/

	KeyMap theKeys;

	::GetKeys( theKeys );

	return( ((unsigned char *)theKeys)[keyCode >> 3] & (1 << (keyCode & 7)) );
	}

#if EXPIRES || EDIT_REGISTERS
/*======================================================================================*/
void CCaptureWindow::fEditRegisters( void ) {

	SInt32 i;

	SInt32 d = finalStage - kNTZVersionStage;

	StDialogHandler dialog( PPob_EditRegisters, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	for( i = 0; i < 256; i++ )
		fEditRegistersRead( window, i );

	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {
			case msg_OK:
				finished = true;
				break;
			case 'Appl':
				LEditText *editText = dynamic_cast<LEditText *>( LCommander::GetTarget() );
				if( editText != NULL ) {
					SInt32 index = editText->GetPaneID() - 256;
					fEditRegistersWrite( window, index );
					fEditRegistersRead( window, index, true );
					}
				break;

			}
		}
	}

/*======================================================================================*/
void CCaptureWindow::fEditRegistersRead( LWindow *window, SInt32 index, Boolean select ) {

	Str255 buffer;

	::sprintf( (char *)&buffer[1], "00" );
	buffer[0] = ::strlen( (char *)&buffer[1] );
	UInt8 value;
	SInt32 registerType = kDeviceTypeRegisters;
	if( fOptionPressed() )
		registerType = kDeviceTypeVideoDecoder;
	if( ::fNTZVDDeviceRead( mVideoDigitizer, registerType, index, 1, &value ) == noErr )
		::sprintf( (char *)&buffer[1], "%02X", value );

	LEditText *editText = (LEditText *)window->FindPaneByID( 256 + index );
	editText->SetDescriptor( buffer );

	if( select ) {
		ControlEditTextSelectionRec pb;
		pb.selStart = 0;
		pb.selEnd = 32767;
		editText->SetSelection( pb );
		}
	}

/*======================================================================================*/
void CCaptureWindow::fEditRegistersWrite( LWindow *window, SInt32 index ) {

	SInt32 i;
	Str255 buffer;

	LEditText *editText = (LEditText *)window->FindPaneByID( 256 + index );
	editText->GetDescriptor( buffer );

	while( buffer[0] > 0 && isspace( (char)buffer[1] ) ) {
		::BlockMoveData( &buffer[2], &buffer[1], buffer[0] - 1 );
		buffer[0]--;
		}
	if( !(buffer[1] == '0' && buffer[2] == 'x') && buffer[0] < 254 ) {
		::BlockMoveData( &buffer[1], &buffer[3], buffer[0] );
		buffer[1] = '0';
		buffer[2] = 'x';
		buffer[0] += 2;
		}

	UInt8 value = 0;
	Boolean valid = true;
	for( i = 3; i <= buffer[0] && valid; i++ ) {
		if( isalpha( (char)buffer[i] ) )
			buffer[i] = toupper( (char)buffer[i] );
		Boolean isNumber = buffer[i] >= '0' && buffer[i] <= '9';
		Boolean isAlpha = buffer[i] >= 'A' && buffer[i] <= 'F';
		valid = isNumber || isAlpha;
		if( valid ) {
			value *= 16;
			value += buffer[i] - (isNumber ? '0' : 'A' - 10);
			}
		}

	if( !valid ) {
		::SysBeep( 1 );
		fEditRegistersRead( window, index, true );
		}
	else
		::fNTZVDDeviceWrite( mVideoDigitizer, kDeviceTypeRegisters, index, 1, &value );
	}

#endif
/*======================================================================================*/
void CCaptureWindow::fOpenMovie( const FSSpec *spec ) {
/* Open the movie in QuickTime Player.													*/

	FSRef ref;
	if( ::FSpMakeFSRef( spec, &ref ) == noErr ) {
		CFURLRef url = ::CFURLCreateFromFSRef( NULL, &ref );
		if( url != NULL )
			::LSOpenCFURLRef( url, NULL );
		}
	}

/*======================================================================================*/
Boolean CCaptureWindow::fGetTimeLapse( sTimeLapseData *timeLapse ) {
/* On entry, timelapse will be the preferred timelapse values. On affirmative exit,		*/
/* timeLapse will be set to new values.													*/

	Boolean gotInterval;
	Boolean gotDuration;
	Boolean gotOutputFrameRate;
	Boolean gotValues = false;

	Boolean isSnapshot = timeLapse->outputMode == kOutputModeSnapshot;

	/* Duplicate the input settings */
	sTimeLapseData lTimeLapse = *timeLapse;

	ResIDT dialogID = isSnapshot ? PPob_SnapshotTimeLapse : PPob_RecordTimeLapse;
	StDialogHandler dialog( dialogID, this );
	LWindow *window = dialog.GetDialog();
	ThrowIfNULL_( window );

	sTimeLapseDialogElements dialogElements;
	dialogElements.intervalValue = dynamic_cast<LEditText *>( window->FindPaneByID( 'eVaI' ) );
	dialogElements.intervalUnit = dynamic_cast<LPopupButton *>( window->FindPaneByID( 'pUnI' ) );
	dialogElements.hasDuration = dynamic_cast<LCheckBox *>( window->FindPaneByID( 'cLim' ) );
	dialogElements.durationValue = dynamic_cast<LEditText *>( window->FindPaneByID( 'eVaD' ) );
	dialogElements.durationUnit = dynamic_cast<LPopupButton *>( window->FindPaneByID( 'pUnD' ) );
	ThrowIfNULL_( dialogElements.intervalValue );
	ThrowIfNULL_( dialogElements.intervalUnit );
	ThrowIfNULL_( dialogElements.hasDuration );
	ThrowIfNULL_( dialogElements.durationValue );
	ThrowIfNULL_( dialogElements.durationUnit );
	if( !isSnapshot ) {
		dialogElements.outputFrameRate = dynamic_cast<LEditText *>( window->FindPaneByID( 'Ofps' ) );
		ThrowIfNULL_( dialogElements.outputFrameRate );
		}
	dialogElements.outputDuration = dynamic_cast<LStaticText *>( window->FindPaneByID( 'Odur' ) );
	ThrowIfNULL_( dialogElements.outputDuration );

	/* Set each field according to prefs */
	fTimeLapseSetFields( &dialogElements, &lTimeLapse );
	fTimeLapseUpdateOutputDuration( &dialogElements, &lTimeLapse );
	fTimeLapseUpdateMenus( &dialogElements, &lTimeLapse );

	/* Set focus */
	dialogElements.intervalValue->SelectAll();
	window->SetLatentSub( dialogElements.intervalValue );

	/* Process the dialog */
	window->Show();
	Boolean finished = false;
	while( !finished ) {
		MessageT message = dialog.DoDialog();
		switch( message ) {

			case 'Stng':
				if( !isSnapshot )
					ObeyCommand( kCmdVideoSettings, NULL );
				else
					ObeyCommand( kCmdSnapshotSettings, NULL );
				break;

			case msg_OK: {
				fTimeLapseGetFields( &dialogElements, &lTimeLapse );

				gotInterval = false;
				gotDuration = lTimeLapse.hasDuration == 0;
				gotOutputFrameRate = isSnapshot;
				double intervalValue = lTimeLapse.intervalValue / 65536.0;
				double durationValue = lTimeLapse.durationValue / 65536.0;
				double outputFrameRate = lTimeLapse.outputFrameRate / 65536.0;

				/* Validate interval */
				if( intervalValue < 1.0 || intervalValue > 1000.0 ) {
					double interval = lTimeLapse.intervalValue *
						(lTimeLapse.intervalUnit == 0 ? 1 :
						lTimeLapse.intervalUnit == 1 ? 60 : 3600);
					dialogElements.intervalValue->SelectAll();
					LCommander::SwitchTarget( dialogElements.intervalValue );
					::ParamText( "\p1.0", "\p1000.0", "\p", "\p" );
					UModalAlerts::StopAlert( ALRT_NumberBad );
					}
				else
					gotInterval = true;

				if( gotInterval )
					if( lTimeLapse.hasDuration != 0 )
						if( durationValue < 1.0 || durationValue > 1000.0 ) {
							double duration = durationValue *
								(lTimeLapse.durationUnit == 0 ? 1 :
								lTimeLapse.durationUnit == 1 ? 60 :
								lTimeLapse.durationUnit == 2 ? 3600 : 86400);
							dialogElements.durationValue->SelectAll();
							LCommander::SwitchTarget( dialogElements.durationValue );
							::ParamText( "\p1.0", "\p1000.0", "\p", "\p" );
							UModalAlerts::StopAlert( ALRT_NumberBad );
							}
						else
							gotDuration = true;

				if( gotInterval && gotDuration )
					if( !isSnapshot )
						if( outputFrameRate < 0.01 || outputFrameRate > 100.0 ) {
							dialogElements.outputFrameRate->SelectAll();
							LCommander::SwitchTarget( dialogElements.outputFrameRate );
							::ParamText( "\p0.01", "\p100.0", "\p", "\p" );
							UModalAlerts::StopAlert( ALRT_NumberBad );
							}
						else
							gotOutputFrameRate = true;

				if( gotInterval && gotDuration && gotOutputFrameRate ) {
					gotValues = true;
					finished = true;
					}
				break; }

			case msg_Cancel:
				finished = true;
				break;

			case 'eVaI': case 'pUnI': case 'cLim':
			case 'eVaD': case 'pUnD': case 'Ofps':
				fTimeLapseGetFields( &dialogElements, &lTimeLapse );
				fTimeLapseUpdateOutputDuration( &dialogElements, &lTimeLapse );
				fTimeLapseUpdateMenus( &dialogElements, &lTimeLapse );
				break;
			}
		}

	/* Report new settings, if changed */
	if( gotValues )
		*timeLapse = lTimeLapse;

	return( gotValues );
	}

/*======================================================================================*/
void CCaptureWindow::fTimeLapseSetFields( sTimeLapseDialogElements *dialogElements,
	sTimeLapseData *timeLapse ) {

	LStr255 string;

	Boolean isSnapshot = timeLapse->outputMode == kOutputModeSnapshot;

	fFloatToString( timeLapse->intervalValue / 65536.0, string );
	dialogElements->intervalValue->SetDescriptor( string );
	dialogElements->intervalUnit->SetValue( timeLapse->intervalUnit + 1 );

	dialogElements->hasDuration->SetValue( timeLapse->hasDuration == 0 ? 0 : 1 );
	fFloatToString( timeLapse->durationValue / 65536.0, string );
	dialogElements->durationValue->SetDescriptor( string );
	dialogElements->durationUnit->SetValue( timeLapse->durationUnit + 1 );

	if( !isSnapshot ) {
		fFloatToString( timeLapse->outputFrameRate / 65536.0, string );
		dialogElements->outputFrameRate->SetDescriptor( string );
		}
	}

/*======================================================================================*/
void CCaptureWindow::fTimeLapseGetFields( sTimeLapseDialogElements *dialogElements,
	sTimeLapseData *timeLapse ) {

	LStr255 string;
	double value;

	Boolean isSnapshot = timeLapse->outputMode == kOutputModeSnapshot;

	dialogElements->intervalValue->GetDescriptor( string );
	fStringToFloat( string, &value );
	timeLapse->intervalValue = (SInt32)(value * 65536);
	timeLapse->intervalUnit = dialogElements->intervalUnit->GetValue() - 1;

	timeLapse->hasDuration = dialogElements->hasDuration->GetValue() == 0 ? 0 : 1;
	dialogElements->durationValue->GetDescriptor( string );
	fStringToFloat( string, &value );
	timeLapse->durationValue = (SInt32)(value * 65536);
	timeLapse->durationUnit = dialogElements->durationUnit->GetValue() - 1;

	if( !isSnapshot ) {
		dialogElements->outputFrameRate->GetDescriptor( string );
		fStringToFloat( string, &value );
		timeLapse->outputFrameRate = (SInt32)(value * 65536);
		}
	}

/*======================================================================================*/
void CCaptureWindow::fTimeLapseUpdateOutputDuration( const sTimeLapseDialogElements *dialogElements,
	const sTimeLapseData *timeLapse ) {
/* Put something useful in the output duration field. Insanely complicated.				*/

	LStr255 string;
	LStr255 outputDurationString;
	LStr255 partial;

	Boolean isSnapshot = timeLapse->outputMode == kOutputModeSnapshot;

	Boolean gotInterval = false;
	Boolean gotDuration = timeLapse->hasDuration == 0;
	Boolean gotOutputFrameRate = isSnapshot;
	double intervalValue = timeLapse->intervalValue / 65536.0;
	double durationValue = timeLapse->durationValue / 65536.0;
	double outputFrameRate = timeLapse->outputFrameRate / 65536.0;

	/* Validate the fields. Needs to stay in sync with validation in dialog handler */
	if( intervalValue < 1.0 || intervalValue > 1000.0 )
		CUtilities2::fGetStringASCII( "Capture", "kCaptureIntervalWarning", outputDurationString );
	else
		gotInterval = true;
	if( gotInterval )
		if( timeLapse->hasDuration != 0 )
			if( durationValue < 1.0 || durationValue > 1000.0 )
				CUtilities2::fGetStringASCII( "Capture", "kCaptureDurationWarning", outputDurationString );
			else
				gotDuration = true;
	if( gotInterval && gotDuration )
		if( !isSnapshot )
			if( outputFrameRate < 0.01 || outputFrameRate > 100.0 )
				CUtilities2::fGetStringASCII( "Capture", "kCaptureOutputFrameRateWarning", outputDurationString );
			else
				gotOutputFrameRate = true;

	/* If the fields are valid, create a string decribing how long the output movie is	*/
	/* going to be.																		*/
	if( gotInterval && gotDuration && gotOutputFrameRate )

		if( timeLapse->hasDuration == 0 ) {
			if( !isSnapshot )
				CUtilities2::fGetStringASCII( "Capture", "kCaptureFramesContinuous", outputDurationString );
			else
				CUtilities2::fGetStringASCII( "Capture", "kCaptureSnapshotsManual", outputDurationString );
			}
		else {

			/* Calculate the duration of the output movie. Create a descriptive string */
			double interval = intervalValue *
				(timeLapse->intervalUnit == 0 ? 1 :
				timeLapse->intervalUnit == 1 ? 60 : 3600);
			double duration = durationValue *
				(timeLapse->durationUnit == 0 ? 1 :
				timeLapse->durationUnit == 1 ? 60 :
				timeLapse->durationUnit == 2 ? 3600 : 86400);
			double frames = duration / interval;

			if( !isSnapshot ) {
				double outputDuration = frames / outputFrameRate;
				SInt32 outputDurationHours = (SInt32)(outputDuration / 3600);
				SInt32 outputDurationMinutes = (SInt32)((outputDuration - 3600 * outputDurationHours) / 60);
				double outputDurationSeconds = outputDuration - 3600 * outputDurationHours - 60 * outputDurationMinutes;
				CUtilities2::fGetStringASCII( "Capture", "kCaptureDurationMessage0", outputDurationString );
				LStr255 string;
				if( outputDurationHours > 0 ) {
					::NumToString( outputDurationHours, string );
					CUtilities2::fGetStringASCII( "Capture", "kCapture_hour", partial );
					if( outputDurationHours != 1 )
						CUtilities2::fGetStringASCII( "Capture", "kCapture_hours", partial );
					outputDurationString += string + "\p " + partial;
					if( outputDurationMinutes > 0 || outputDurationSeconds > 0 )
						outputDurationString += LStr255( ", " );
					}
				if( outputDurationMinutes > 0 ) {
					::NumToString( outputDurationMinutes, string );
					CUtilities2::fGetStringASCII( "Capture", "kCapture_minute", partial );
					if( outputDurationMinutes != 1 )
						CUtilities2::fGetStringASCII( "Capture", "kCapture_minutes", partial );
					outputDurationString += string + "\p " + partial;
					if( outputDurationSeconds > 0 )
						outputDurationString += LStr255( ", " );
					}
				if( outputDurationSeconds > 0 || (outputDurationHours == 0 && outputDurationMinutes == 0) ) {
					fFloatToString( outputDurationSeconds, string );
					CUtilities2::fGetStringASCII( "Capture", "kCapture_second", partial );
					if( outputDurationSeconds != 1.0 )
						CUtilities2::fGetStringASCII( "Capture", "kCapture_seconds", partial );
					outputDurationString += string + "\p " + partial;
					}
				CUtilities2::fGetStringASCII( "Capture", "kCaptureDurationMessage1", partial );
				outputDurationString += partial;
				}
			else {
				CUtilities2::fGetStringASCII( "Capture", "kCaptureDurationMessage2", outputDurationString );
				::NumToString( (SInt32)frames, string );
				CUtilities2::fGetStringASCII( "Capture", "kCaptureDurationMessage3", partial );
				outputDurationString += string + partial;
				}
			}

	dialogElements->outputDuration->SetDescriptor( outputDurationString );
	}

/*======================================================================================*/
void CCaptureWindow::fTimeLapseUpdateMenus( const sTimeLapseDialogElements *dialogElements,
	const sTimeLapseData *timeLapse ) {

	LStr255 partial;

	Boolean isSnapshot = timeLapse->outputMode == kOutputModeSnapshot;

	/* Update menu plurals */
	if( timeLapse->intervalValue / 65536.0 == 1 ) {
		dialogElements->intervalUnit->SetMenuItemText( 1, CUtilities2::fGetStringASCII( "Capture", "kCaptureSecond", partial ) );
		dialogElements->intervalUnit->SetMenuItemText( 2, CUtilities2::fGetStringASCII( "Capture", "kCaptureMinute", partial ) );
		dialogElements->intervalUnit->SetMenuItemText( 3, CUtilities2::fGetStringASCII( "Capture", "kCaptureHour", partial ) );
		}
	else {
		dialogElements->intervalUnit->SetMenuItemText( 1, CUtilities2::fGetStringASCII( "Capture", "kCaptureSeconds", partial ) );
		dialogElements->intervalUnit->SetMenuItemText( 2, CUtilities2::fGetStringASCII( "Capture", "kCaptureMinutes", partial ) );
		dialogElements->intervalUnit->SetMenuItemText( 3, CUtilities2::fGetStringASCII( "Capture", "kCaptureHours", partial ) );
		}
	if( timeLapse->durationValue / 65536.0 == 1 ) {
		dialogElements->durationUnit->SetMenuItemText( 1, CUtilities2::fGetStringASCII( "Capture", "kCaptureSecond", partial ) );
		dialogElements->durationUnit->SetMenuItemText( 2, CUtilities2::fGetStringASCII( "Capture", "kCaptureMinute", partial ) );
		dialogElements->durationUnit->SetMenuItemText( 3, CUtilities2::fGetStringASCII( "Capture", "kCaptureHour", partial ) );
		dialogElements->durationUnit->SetMenuItemText( 4, CUtilities2::fGetStringASCII( "Capture", "kCaptureDay", partial ) );
		}
	else {
		dialogElements->durationUnit->SetMenuItemText( 1, CUtilities2::fGetStringASCII( "Capture", "kCaptureSeconds", partial ) );
		dialogElements->durationUnit->SetMenuItemText( 2, CUtilities2::fGetStringASCII( "Capture", "kCaptureMinutes", partial ) );
		dialogElements->durationUnit->SetMenuItemText( 3, CUtilities2::fGetStringASCII( "Capture", "kCaptureHours", partial ) );
		dialogElements->durationUnit->SetMenuItemText( 4, CUtilities2::fGetStringASCII( "Capture", "kCaptureDays", partial ) );
		}
	}

/*======================================================================================*/
void CCaptureWindow::fFloatToString( double value, LStr255 &string ) {

	if( mSystemVersion >= 0x00001030 ) {
		CFNumberFormatterRef numberFormatter = ::CFNumberFormatterCreate( NULL,
			::CFLocaleCopyCurrent(), kCFNumberFormatterDecimalStyle );
		if( numberFormatter != NULL ) {
			::CFNumberFormatterSetFormat( numberFormatter, CFSTR( "###,##0.##" ) );
			CFStringRef cfString = ::CFNumberFormatterCreateStringWithValue( NULL,
				numberFormatter, kCFNumberDoubleType, &value );
			if( cfString != NULL ) {
				::CFStringGetPascalString( cfString, string, 256, kCFStringEncodingMacRoman );
				::CFRelease( cfString );
				}
			::CFRelease( numberFormatter );
			}
		}
	else
		string.Assign( value, "\p###,##0.##", 200 );
	}

/*======================================================================================*/
void CCaptureWindow::fStringToFloat( const LStr255 &string, double *value ) {

	if( mSystemVersion >= 0x00001030 ) {
		CFNumberFormatterRef numberFormatter = ::CFNumberFormatterCreate( NULL,
			::CFLocaleCopyCurrent(), kCFNumberFormatterDecimalStyle );
		if( numberFormatter != NULL ) {
			::CFNumberFormatterSetFormat( numberFormatter, CFSTR( "###,###.##" ) );
			CFStringRef cfString = ::CFStringCreateWithPascalString( NULL,
				string, kCFStringEncodingMacRoman );
			if( cfString != NULL ) {
				::CFNumberFormatterGetValueFromString( numberFormatter, cfString, NULL,
					kCFNumberDoubleType, value );
				::CFRelease( cfString );
				}
			::CFRelease( numberFormatter );
			}
		}
	else
		*value = LString::StringToLongDouble( string, "\p###,###.##" );
	}

/*======================================================================================*/
/*======================================================================================*/

