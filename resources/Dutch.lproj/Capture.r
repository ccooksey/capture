/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include "NTZRelease.h"

resource 'STR#' (16000) {
		{
		kNTZDeveloper,
		kNTZLicenseeNameShort,
		kNTZProductName,
		kNTZPackageName,
		"Bewaar nieuwe film als:",
		" Film",
		" Snapshot",
		"Over ^0É",
		"Niet beschikbaar",
		"Snapshot",
		"Opnemen"
		}
	};

/* These aren't used anymore. They used to go in PPob layout 134, which isn't used		*/
/* anymore either.																		*/
resource 'STR#' (16001) {
		{
		"the QuickTime sequence grabber could not be opened. This may be a QuickTime installation problem.",
		"a video channel could not be opened. Please make sure that " kNTZPackageName " For Mac OS X is installed properly.",
		"the video digitizer associated with the video channel could not be obtained.",
		"the QuickTime sequence grabber could not be configured.",
		"a sound channel could not be opened or configured.",
		"the video digitizer could not be configured.",
		"full-screen support could not be configured.",
		"a device is not available."
		}
	};
