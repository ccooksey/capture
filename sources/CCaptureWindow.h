/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#pragma once
#ifndef CCaptureWindow_h
#define CCaptureWindow_h

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

#include <LWindow.h>
#include <LEditText.h>
#include <LPopupButton.h>
#include <LCheckBox.h>
#include <LBevelButton.h>

#include "CClickWindow.h"
#include "CClickView.h"

/*--------------------------------------------------------------------------------------*/
typedef struct {
	GWorldPtr world;
	PixMapHandle map;
	Ptr base;
	SInt32 rowBytes;
	Rect rect;
	} sLockedPixMap;

#define kMaxInputNames 32
typedef struct {
	Component component;
	UInt32 subType;
	UInt32 manufacturer;
	Str63 name;
	SInt32 inputCount;
	Str255 inputNames[kMaxInputNames];
	} sDevice, *sDevicePtr;

typedef struct {
	SInt32 unused;
	UInt32 subType;
	UInt32 manufacturer;
	Str63 name;
	Str255 inputName;
	} sDeviceTriple, *sDeviceTriplePtr;

typedef struct {
	SInt32 count;
	sDevice devices[1];
	} sDeviceList, *sDeviceListPtr;

enum {
	kOutputModeSnapshot,
	kOutputModeMovie
	};

typedef struct {
	SInt32 version;
	SInt32 intervalValue;		// fixed 16.16 
	SInt32 intervalUnit;		// 0 = seconds, 1 = minutes, 2 = hours
	Boolean hasDuration;		// true = limit duration
	SInt32 durationValue;		// fixed 16.16
	SInt32 durationUnit;		// 0 = seconds, 1 = minutes, 2 = hours
	SInt32 outputFrameRate;		// fixed 16.16 in frames per second;
	SInt32 outputMode;			// 0 = individual snapshots, 1 = movie
	} sTimeLapseData;

typedef struct {
	LEditText *intervalValue;
	LPopupButton *intervalUnit;
	LCheckBox *hasDuration;
	LEditText *durationValue;
	LPopupButton *durationUnit;
	LEditText *outputFrameRate;
	LStaticText *outputDuration;
	} sTimeLapseDialogElements;

/*--------------------------------------------------------------------------------------*/
class CCaptureWindow :
	public LWindow,
	public LPeriodical,
	public LListener,
	public LBroadcaster {

	public:

		enum { class_ID = 'WMon' };
		static CCaptureWindow *CreateFromStream( LStream *inStream );
		CCaptureWindow( LStream *inStream );
		virtual ~CCaptureWindow( void );
		#if kNTZPackageNumber == '3Cam'
		void FinishCreateSelf( void );
		#endif

		void HandleAppleEvent( const AppleEvent &inAppleEvent, AppleEvent &outAEReply, AEDesc &outResult, long inAENumber );

		static SInt32 fNewDeviceListCopy( sDeviceListPtr *list, Boolean *serialized );
		static SInt32 fDisposeDeviceListCopy( sDeviceListPtr list );
		static Boolean fGetPreferenceID( sDevicePtr device, SInt32 input, SInt32 *preferenceID );

		OSStatus fStartSequencer( SInt32 i, sDevicePtr device, SInt32 input );
		void fResizeWindow( SInt32 inSize );
		OSStatus fResizeSequencer( SInt32 inSize );
		void fStartRecording( UInt32 duration, FSSpec *spec, sTimeLapseData *timeLapse, Boolean forceAutomatic );
		Boolean fIsRecording( void );
		void fStopRecording( void );
		void fSnapshotSmall( FSSpec *spec, Boolean forceAutomatic );
		#if kNTZPackageNumber == '3Cam'
		void fSnapshotSmallLCTFSCopy( void );
		virtual void AttemptClose( void );
		virtual Boolean AttemptQuit( long inSaveOption );
		#endif
		#if kNTZLicenseeNumber != 'eEm '
		void fSnapshotLarge( FSSpec *spec, Boolean forceAutomatic );
		#endif
		void fPause( Boolean pause, CCaptureWindow *who );
		void fActivateSound( void );
		void fStopSoundPreview( void );
		void fStartSoundPreview( void );

		virtual void ActivateSelf( void );
		virtual void Suspend( void );
		virtual void Resume( void );
		virtual void DrawSelf( void );
		virtual Boolean HandleKeyPress( const EventRecord &inKeyEvent );
		virtual Boolean ObeyCommand( CommandT inCommand, void *ioParam );
		virtual void FindCommandStatus( CommandT inCommand, Boolean &outEnabled, Boolean &outUsesMark, UInt16 &outMark, Str255 outName );
		virtual	void SpendTime( const EventRecord &inMacEvent );
		virtual void ListenToMessage( MessageT inMessage, void *ioParam );

	protected:

		static SInt32 fUnregisterUndesirables( void );
		void fAspectRatioWarning( void );
		SInt32 fGetBestSize( const Rect *rect );
		void fReportStartSequencerError( const char *errorMessageKey, SInt32 theErr );
		void fReportSoundError( ConstStr255Param quickTimeCall, SInt32 theErr );
		OSStatus fBeginFullScreen( SInt32 inSize );
		OSStatus fEndFullScreen( void );
		void fVideoSettings( void );
		void fSoundSettings( void );
		SInt32 fLoadSettings( SInt32 inPrefTag, SGChannel channel );
		SInt32 fSaveSettings( SInt32 inPrefTag, SGChannel channel );
		void fPreconfigureNewDevice( sDevicePtr device );
		SInt32 fSetSoundInputDriver( ConstStr255Param name );
		void fLoadSoundDriver( void );
		void fSaveSoundDriver( void );
		SInt32 fSnapshotSave( PicHandle picture, const FSSpec *spec, Boolean forceAutomatic, Boolean mute );
		SInt32 fGetSnapshotExporterFileMetaData( LStr255 &extension, OSType &fileType, OSType &fileCreator );
		//void fGetImporterExtension( GraphicsImportComponent importer, OSType format, char *extension );
		SInt32 fGetAutomaticFileDestination( char *path, char *name, char *extension, SInt32 extensionLength, Boolean reset, FSSpec *spec );
		Boolean fGetDuration( UInt32 *outHours, UInt32 *outMinutes, UInt32 *outSeconds );
		Boolean fGetCustomSize( SInt32 *customWidth, SInt32 *customHeight );
		void fUpdateTimePanel( UInt32 timeRemaining );

		sDevice mDevice;
		SInt32 mInput;
		SInt32 mPreferenceID;
		SInt32 mQuickTimeVersion; // e.g. 0x00708000

		Boolean mSoundActive;
		SInt32 mSelectedSize;
		Boolean mRecordVideo;
		Boolean mRecordSound;

		SInt32 mWidth;
		SInt32 mHeight;

		SInt32 mCaptureWidth;		// VDIG channel bounds requested
		SInt32 mCaptureHeight;
		SInt32 mWindowWidth;		// Window bounds requested (can be different from the
		SInt32 mWindowHeight;		// channel bounds in the 3Cam case, but not in typical case).

		Boolean mAspectRatioWarning;

		SeqGrabComponent mSequenceGrabber;
		SGChannel mVideoChannel;
		ComponentInstance mVideoDigitizer;
		SGChannel mSoundChannel;
		OSType mSoundDevice;
		Boolean mSoundDeviceChanged;
		SInt32 mSoundDuringPreview;
		SInt32 mSoundDuringRecord;

		Boolean mNTZInterfaceAvailable;
		SInt32 mNTZDeviceType;

		//Boolean mDisplayFrameRate;
		//Boolean mLetterBox;
		//SInt32 mStartScan;
		//SInt32 mNumScans;
		//SInt32 mStartColumn;
		//SInt32 mNumColumns;
		//SInt32 mReadyCount;
		//UInt32 mTickCount;
		//SInt32 mFrameCount;
		//SInt32 mFramesPerSecond;
		//SInt32 mAdjustTick;

		Boolean mInPosition;

		float mMultiplier;
		SInt32 mFSWidth;
		SInt32 mFSHeight;

		/* Full screen state */
		Boolean mFullScreen;
		SInt32 mCustomWidth;
		SInt32 mCustomHeight;
		SInt32 mRegularSize;			// Last non-fullscreen, non-custom size user selected
		Point mTopLeft;
		Boolean mVideoUsageChanged;
		SInt32 mVideoUsage;
		Boolean mSoundUsageChanged;
		SInt32 mSoundUsage;
		Boolean mFullScreenEntered;
		Boolean mWindowMoved;
		Ptr mRestoreState;
		OSType mSaveCompressor;
		OSType mSaveCompressorType;

	private:

		SInt32 fGetFileSpec( const FSSpec *inSpec, Boolean automatic, Boolean isTimeLapse, Boolean isSnapshot, FSSpec *outSpec, FSSpec *outSpecNoIndex, OSType *outFileType, OSType *outFileCreator );
		SInt32 fAddClipToITunes( const FSSpec *movie );
		static pascal OSErr fSGDataProcTimeLapse( SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType, long refCon );
		OSErr fSGDataProcTimeLapse( SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType );
		void fOpenMovie( const FSSpec *spec );
		SInt32 fSGSetChannelUsage( SGChannel channel, long usage );

		Boolean fGetTimeLapse( sTimeLapseData *timeLapse );
		void fTimeLapseSetFields( sTimeLapseDialogElements *dialogElements, sTimeLapseData *timeLapseData );
		void fTimeLapseGetFields( sTimeLapseDialogElements *dialogElements, sTimeLapseData *timeLapseData );
		void fTimeLapseUpdateOutputDuration( const sTimeLapseDialogElements *dialogElements, const sTimeLapseData *timeLapseData );
		void fTimeLapseUpdateMenus( const sTimeLapseDialogElements *dialogElements, const sTimeLapseData *timeLapseData );
		void fFloatToString( double value, LStr255 &string );
		void fStringToFloat( const LStr255 &string, double *value );
	
		static void fSnapshotCallback( SInt32 refCon );
		void fSnapshotCallback( void );

		static pascal OSErr fSGDataProcRecord( SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType, long refCon );
		OSErr fSGDataProcRecord( SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType );
		OSStatus fCreateDecompressionSession( ImageDescriptionHandle imageDesc, ICMDecompressionSessionRef *decompressionSessionOut );
		OSStatus fWriteVideoToMovie( UInt8 *p, long len, long *offset, TimeValue time, long chRefCon );
		OSStatus fWriteAudioToMovie( UInt8 *p, long len, long *offset, TimeValue time, long chRefCon );
		static void fDisplayFrame( void *decompressionTrackingRefCon, OSStatus result, ICMDecompressionTrackingFlags decompressionTrackingFlags, CVPixelBufferRef pixelBuffer, TimeValue64 displayTime, TimeValue64 displayDuration, ICMValidTimeFlags validTimeFlags, void *reserved, void *sourceFrameRefCon );
		void fDisplayFrame( OSStatus result, ICMDecompressionTrackingFlags decompressionTrackingFlags, CVPixelBufferRef pixelBuffer, TimeValue64 displayTime, TimeValue64 displayDuration, ICMValidTimeFlags validTimeFlags, void *reserved, void *sourceFrameRefCon );

		SInt32 mSystemVersion;		// e.g. 0x00001030

		UInt32 mWantVolumeAtTime;
		SInt16 mWantVolumeAt;
		SInt32 mPreSuspensionPreviewSoundUsage;
	
		/* fStart/StopRecording() support */
		SInt32 mRecordErr;
		Boolean mIsRecording;
		UInt32 mDuration;			// seconds
		UInt32 mStartTime;			// seconds
		UInt32 mNextDurationCheck;	// ticks
		Boolean mPreviewStopped;
		FSSpec mRecordSpec;
		FSSpec mRecordSpecNoIndex;
		SInt32 mRecordSpecDirID;
		Boolean mUsageRecording;
		Boolean mSuppressUI;
		CClickWindow *mClickWindow;
		CClickView *mClickView;
		LStaticText *mTimePanel;
		Boolean mIsTimeLapse;
		sTimeLapseData mTimeLapseData;
		double mTimeLapseNextTime;	// ticks
		double mTimeLapseInterval;	// ticks
		SInt16 mMovieResRefNum;
		Movie mMovie;
		Track mTrack;
		Media mMedia;
		ImageDescriptionHandle mImageDescription;
		TimeValue64 mTLDecodeDurationPerSample;
		Boolean mTimeLapseKeyFrameRateChanged;
		short mTLSaveDepth;
		CompressorComponent mTLSaveCompressor;
		CodecQ mTLSaveSpatialQuality;
		CodecQ mTLSaveTemporalQuality;
		long mTLSaveKeyFrameRate;
		GraphicsExportComponent mTLExporter;

		#if EXPIRES || EDIT_REGISTERS
		void fEditRegisters( void );
		void fEditRegistersRead( LWindow *window, SInt32 index, Boolean select = false );
		void fEditRegistersWrite( LWindow *window, SInt32 index );
		#endif

		SInt32 mSnapshotButtonPressed;
		UInt32 mTicksSinceStart;

		/* SGDataProc recording video display state */
		SInt32 mFrameCount;
		TimeScale mTimeScale;
		ImageSequence mDecompressionSession;

		#if kNTZPackageNumber == '3Cam'
		SInt32 mLCTVRResolution;
		LGWorld *mLCTGWorld;
		UInt32 mLCTLastCopyBits;
		UInt32 mLCTLastStatusUpdate;
		LStaticText *mLCTVRStatusText;
		LBevelButton *mLCTRecordButton;
		LBevelButton *mLCTStopButton;
		LBevelButton *mLCTToolsButton;
		UInt64 mLCTReserveSpace;
		LBevelButton *mLCTQuitButton;
		LBevelButton *mLCTSnapButton;
		#endif
	};

/*--------------------------------------------------------------------------------------*/
Boolean fOptionPressed( void );

#endif
