/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#pragma once

#if TARGET_RT_MAC_MACHO
#ifndef __CARBON__
#include <Carbon/Carbon.h>
#endif
#else
#include <MacTypes.h>
#endif

const SInt32 kStrings =								16000;
enum {
	kStringsManufacturer =							1,
	kStringsLicenseeShort,
	kStringsProduct,
	kStringsDriverName,
	kStringsSavePrompt,
	kStringsMovieSuffix,
	kStringsSnapshotsSuffix,
	kStringsAbout,
	kStringsCaptureDisabled,
	kStringsSnaphot,
	kStringsRecording
	};
const SInt32 kStringsPreviewProblem =				16001;

const ResIDT ALRT_CouldNotSave =					16000;
const ResIDT ALRT_CouldNotSaveFileBusy =			16001;
const ResIDT ALRT_NoQuickTime =						16010;
const ResIDT ALRT_ApplicationAlreadyRunning =		16011;
const ResIDT ALRT_CameraNotDetected =				16012;
const ResIDT ALRT_NumberBad =						16020;

const ResIDT PPob_Monitor =							128;
const ResIDT PPob_AboutWindow =						129;
const ResIDT PPob_ClickToStopMovie =				130;
const ResIDT PPob_ClickToStopSnapshots =			131;
const ResIDT PPob_MovieRecorded =					132;
const ResIDT PPob_SnapshotsTaken =					133;
const ResIDT PPob_PreviewProblem =					134;
const ResIDT PPob_RecordingDiskFull =				135;
const ResIDT PPob_SnapshotsDiskFull =				136;
const ResIDT PPob_RecordingProblem =				137;
const ResIDT PPob_SnapshotsProblem =				138;
const ResIDT PPob_NoDigitizers =					139;
const ResIDT PPob_SnapshotSettings =				140;
const ResIDT PPob_AutoSnapshotSetup =				141;
const ResIDT PPob_AutoRecordSetup =					142;
const ResIDT PPob_CustomSize =						143;
const ResIDT PPob_Duration =						144;
const ResIDT PPob_WarningAspectRatio =				145;
const ResIDT PPob_Unserialized =					146;
const ResIDT PPob_Preferences =						147;
const ResIDT PPob_SoundChannelError =				148;
const ResIDT PPob_SnapshotTimeLapse =				149;
const ResIDT PPob_RecordTimeLapse =					150;
const ResIDT PPob_NoDigitizersOpen =				151;
const ResIDT PPob_RecordAutoFolderNotFound =		152;
const ResIDT PPob_SnapshotAutoFolderNotFound =		153;
const ResIDT PPob_StartSequencerError =				154;
const ResIDT PPob_AutoSnapshotButtonSetup =			155;
const ResIDT PPob_AutoRecordButtonSetup =			156;
const ResIDT PPob_LogicalChoiceNoDigitizers =		200;
const ResIDT PPob_LogicalChoiceSelectDigitizer =	201;
const ResIDT PPob_LogicalChoiceMonitorFS =			202;
const ResIDT PPob_LogicalChoicePictureOnClip =		203;
const ResIDT PPob_LogicalChoiceSelectResolution =	204;
const ResIDT PPob_LogicalChoiceMonitorVR =			205;
const ResIDT PPob_LogicalChoiceDiskSpaceExhausted =	206;
const ResIDT PPob_EditRegisters =					1000;
const ResIDT PPob_WrongSerialNumber =				2000;

const SInt32 kPrefTagKeep =							'Nloc';

const SInt32 kPrefTagLocation =						'Nloc';
const SInt32 kPrefTagSelectedSize =					'Nssz';
const SInt32 kPrefTagRegularSize =					'Nrsz';
const SInt32 kPrefTagCustomWidth =					'Ncwi';
const SInt32 kPrefTagCustomHeight =					'Nche';
const SInt32 kPrefTagRecordVideo =					'Nrvd';
const SInt32 kPrefTagRecordSound =					'Nrsd';
const SInt32 kPrefTagHours =						'Nhou';
const SInt32 kPrefTagMinutes =						'Nmin';
const SInt32 kPrefTagSeconds =						'Nsec';
const SInt32 kPrefTagARWarningOff =					'Narw';
const SInt32 kPrefTagTimeLapseRecord =				'TLpR';
const SInt32 kPrefTagTimeLapseSnapshot =			'TLpS';

const SInt32 kPrefTagChannelVDIGTripleData =		'TVTt';
const SInt32 kPrefTagVideoChannelData =				'TDat';
const SInt32 kPrefTagAudioChannelData =				'TDAd';
const SInt32 kPrefTagAudioDevice =					'TAud';
const SInt32 kPrefTagAudioDeviceUID =				'TAid';
const SInt32 kPrefTagDigitizerClosed =				'TClo';
const SInt32 kPrefDataStartID =						128;

const SInt32 kPrefTagSnapshot =						'SnpS';
const SInt32 kPrefSnapshotAuto =					128;
const SInt32 kPrefSnapshotPath =					129;
const SInt32 kPrefSnapshotName =					130;
const SInt32 kPrefSnapshotExporterFormat =			132;
const SInt32 kPrefTagExporterFormatOptions =		'SnpO';
//const SInt32 kPrefSnapshotImporterFormat =		133;
//const SInt32 kPrefTagImporterFormatOptions =		'SnpI';
const SInt32 kPrefSnapshotUseQuickTimeDialog =		134;

const SInt32 kPrefTagRecord =						'Reco';
const SInt32 kPrefRecordAuto =						128;
const SInt32 kPrefRecordPath =						129;
const SInt32 kPrefRecordName =						130;

const SInt32 kPrefTagSynchronize =					'Sync';
const SInt32 kPrefSynchronize =						128;

const SInt32 kPrefTagMisc =							'Misc';
const SInt32 kPrefMiscKeepAwake =					128;
const SInt32 kPrefMiscSnapshotButtonMode =			129;
const SInt32 kPrefMiscBackgroundMute =				130;
const SInt32 kPrefMiscAddClipsToITunes =			131;

const SInt32 kPrefTagLCT =							'Lct ';
const SInt32 kPrefLCTPreferredInput =				128;
const SInt32 kPrefLCTAutoSelectInput =				129;
const SInt32 kPrefLCTPreferredResolution =			130;
const SInt32 kPrefLCTAutoSelectResolution =			131;

const SInt32 kMenuFile =							129;
const SInt32 kMenuFileAqua =						229;

const SInt32 kMenuEdit =							130;
const SInt32 kCmdEditRegisters =					16650;

const SInt32 kMenuSnapshot =						131;
const SInt32 kCmdSnapshotSettings =					'SSst';
const SInt32 kCmdSnapshotSmall =					'SSsm';
const SInt32 kCmdSnapshotLarge =					'SSlg';
const SInt32 kCmdSnapshotTimeLapse =				'SStl';
const SInt32 kCmdAutoSnapshotSetup =				'SSsu';

const SInt32 kMenuRecord =							132;
const SInt32 kCmdVideoSettings =					'cVid';
const SInt32 kCmdSoundSettings =					'cSnd';
const SInt32 kCmdRecordVideo =						'cRVd';
const SInt32 kCmdRecordSound =						'cRSd';
const SInt32 kCmdQuarterSize =						'cQrt';
const SInt32 kCmdHalfSize =							'cHlf';
const SInt32 kCmdFullSize =							'cFll';
const SInt32 kCmdDoubleSize =						'cDub';
const SInt32 kCmd80x60 =							'c80 ';
const SInt32 kCmd160x120 =							'c160';
const SInt32 kCmd320x240 =							'c320';
const SInt32 kCmd640x480 =							'c640';
const SInt32 kCmdPAL360x288 =						'c360';
const SInt32 kCmdPAL720x576 =						'c720';
const SInt32 kCmdCustomSize =						'cCus';
const SInt32 kCmdFullScreen =						'cScr';
const SInt32 kCmdFullScreen16x9 =					'cWSc';
const SInt32 kCmdRecord =							'cRcd';
const SInt32 kCmdRecordDuration =					'cRcD';
const SInt32 kCmdRecordTimeLapse =					'cRcT';
const SInt32 kCmdAutoRecordSetup =					'cRsu';
const SInt32 kCmdDisplayFrameRate =					'cFrm';

const SInt32 kMenuDigitizers =						133;
const SInt32 kCmdDigitizerBase =					17000;

const SInt32 kMenuSnapshotMAGIX =					231;
const SInt32 kMenuRecordMAGIX =						232;
const SInt32 kMenuDigitizersMAGIX =					233;
const SInt32 kMenuHelpMAGIX =						234;
const SInt32 kCmdHelp =								'Help';
const SInt32 kCmdRegisterProduct =					'Regi';

#if kNTZPackageNumber == '3Cam'
const SInt32 kMsgLogicalChoiceFSSetup =				'LCfs';
const SInt32 kMsgLogicalChoiceVRSetup =				'LCvr';
#endif

const SInt32 kTextTraitGeneva9 =					130;
const SInt32 kTextTraitGeneva9Red =					137;


