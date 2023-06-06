/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include "NTZRelease.h"

resource 'aete' (0, "German") {
        0x1,
        0x0,
        english,
        roman,
        {       /* array Suites: 1 element */

                /* [1] */
                "Erfassen",
                "",
                'efxc',
                1,
                1,
                {       /* array Events: 3 elements */
                        /* [1] */
                        "aufnahme starten",
                        "Filmaufnahme starten, z.B.\n"
						"aufnahme starten,\n"
						"aufnahme starten als \"myMovie.mov\", oder\n"
						"aufnahme starten fenster \"VideoGlide\" als \"myMovie.mov\"",
                        'efxc',
                        'Star',
                        noReply,
                        "",
                        replyOptional,
                        singleItem,
                        notEnumerated,
                        notTightBindingFunction,
                        enumsAreConstants,
                        enumListCanRepeat,
                        replyIsValue,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        verbEvent,
                        reserved,
                        reserved,
                        reserved,
                        noParams,
                        "",
                        directParamOptional,
                        singleItem,
                        notEnumerated,
                        doesntChangeState,
                        enumsAreConstants,
                        enumListCanRepeat,
                        directParamIsValue,
                        directParamIsTarget,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        {       /* array OtherParams: 3 elements */
                                /* [1] */
                                "als",
                                'kFil',
                                'fss ',
                                "Der Dateiname oder Dateipfad. Wenn kein Name angegeben "
								"ist, wird der Anwender zur Eingabe aufgefordert, sofern "
								"Auto-Name nicht aktiviert ist.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular,
                                /* [2] */
                                "fenster",
                                'kWch',
                                'TEXT',
                                "Der Name des Fensters, in dem die Aufnahme vorgenommen "
								"werden soll. Wenn kein Name angegeben wird, wird das "
								"vorderste Fenster verwendet.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular,
                                /* [3] */
                                "dauer",
                                'kDur',
                                'magn',
                                "Die gewünschte Aufnahmedauer in Sekunden.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular
                        },
                        /* [2] */
                        "aufnahme anhalten",
                        "Die Filmaufnahme wird angehalten, z.B.\n"
						"aufnahme anhalten, oder\n"
						"aufnahme anhalten fenster \"VideoGlide\"",
                        'efxc',
                        'Stop',
                        noReply,
                        "",
                        replyOptional,
                        singleItem,
                        notEnumerated,
                        notTightBindingFunction,
                        enumsAreConstants,
                        enumListCanRepeat,
                        replyIsValue,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        verbEvent,
                        reserved,
                        reserved,
                        reserved,
                        noParams,
                        "",
                        directParamOptional,
                        singleItem,
                        notEnumerated,
                        doesntChangeState,
                        enumsAreConstants,
                        enumListCanRepeat,
                        directParamIsValue,
                        directParamIsTarget,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        {       /* array OtherParams: 1 elements */
                                /* [1] */
                                "fenster",
                                'kWch',
                                'TEXT',
                                "Der Name des Fensters, in dem die Aufnahme angehalten "
								"werden soll. Wenn kein Name angegeben wird, wird das "
								"vorderste Fenster verwendet.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular
                        },
                        /* [3] */
                        "snapshot erstellen",
                        "Ein Snapshot wird erstellt, z.B.\n"
						"snapshot erstellen\n"
						"snapshot erstellen als \"snapshot\", oder\n"
						"snapshot erstellen fenster \"VideoGlide\" als \"snapshot\"\n"
						"Das Format des Bilds wird über \"Snapshot-Einstellungen…\" festgelegt.",
                        'efxc',
                        'Snap',
                        noReply,
                        "",
                        replyOptional,
                        singleItem,
                        notEnumerated,
                        notTightBindingFunction,
                        enumsAreConstants,
                        enumListCanRepeat,
                        replyIsValue,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        verbEvent,
                        reserved,
                        reserved,
                        reserved,
                        noParams,
                        "",
                        directParamOptional,
                        singleItem,
                        notEnumerated,
                        doesntChangeState,
                        enumsAreConstants,
                        enumListCanRepeat,
                        directParamIsValue,
                        directParamIsTarget,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        reserved,
                        {       /* array OtherParams: 3 elements */
                                /* [1] */
                                "als",
                                'kFil',
                                'fss ',
                                "Der Dateiname oder Dateipfad. Wenn kein Name angegeben "
								"ist, wird der Anwender zur Eingabe aufgefordert, sofern "
								"Auto-Name nicht aktiviert ist.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular,
                                /* [2] */
                                "fenster",
                                'kWch',
                                'TEXT',
                                "Der Name des Fensters, in dem ein Snapshot erstellt "
								"werden soll. Wenn kein Name angegeben wird, wird das "
								"vorderste Fenster verwendet.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular,
                                /* [3] */
                                "gross",
                                'kLrg',
                                'bool',
                                "Die Größe des gewünschten Snapshots. Wenn keine Angabe "
								"gemacht wird, wird die aktuelle Frame-Größe verwendet. "
								"Wenn \"gross\" angegeben wird und der Digitalisierer "
								"diese Einstellung unterstützt, wird ein "
								"Digitalisierer-spezifisches großes Bild erstellt.",
                                optional,
                                singleItem,
                                notEnumerated,
                                reserved,
                                enumsAreConstants,
                                enumListCanRepeat,
                                paramIsValue,
                                notParamIsTarget,
                                reserved,
                                reserved,
                                reserved,
                                reserved,
                                prepositionParam,
                                notFeminine,
                                notMasculine,
                                singular
                        }
                },
                {       /* array Classes: 0 elements */
                },
                {       /* array ComparisonOps: 0 elements */
                },
                {       /* array Enumerations: 0 elements */
                }
        }
};

resource 'aedt' (16000) {
        {       /* array: 3 elements */
                /* [1] exfc Star */
                1701214307, 1400136050, 16000,
                /* [2] exfc Stop */
                1701214307, 1400139632, 16001,
                /* [3] efxc Snap */
                1701214307, 1399742832, 16002
        }
};
