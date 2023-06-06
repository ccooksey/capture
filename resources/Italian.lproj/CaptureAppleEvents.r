/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include "NTZRelease.h"

resource 'aete' (0, "Italian") {
        0x1,
        0x0,
        english,
        roman,
        {       /* array Suites: 1 element */

                /* [1] */
                "Cattura",
                "",
                'efxc',
                1,
                1,
                {       /* array Events: 3 elements */
                        /* [1] */
                        "avvia registrazione",
                        "Avvia la registrazione di un video, ad esempio\n"
						"avvia registrazione,\n"
						"avvia registrazione come \"myMovie.mov\", o\n"
						"avvia registrazione finestra \"VideoGlide\" come \"myMovie.mov\"",
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
                                "come",
                                'kFil',
                                'fss ',
                                "Il nome del file o il percorso del file. In mancanza di "
								"un nome esso verrà richiesto, a meno che non sia "
								"attivata la funzione di nome automatico.",
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
                                "finestra",
                                'kWch',
                                'TEXT',
                                "Il nome della finestra da registrare. In mancanza del "
								"nome verrà usata la finestra in primo piano.",
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
                                "durata",
                                'kDur',
                                'magn',
                                "Durata desiderata della registrazione in secondi.",
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
                        "stop registrazione",
                        "Ferma la registrazione di un video, ad esempio\n"
						"stop registrazione, o\n"
						"stop registrazione finestra \"VideoGlide\"",
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
                                "finestra",
                                'kWch',
                                'TEXT',
                                "Il nome della finestra della quale fermare la "
								"registrazione. In mancanza del nome verrà usata la "
								"finestra in primo piano.",
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
                        "riprendi istantanea",
                        "Riprende un'istantanea, ad esempio\n"
						"riprendi istantanea\n"
						"riprendi istantanea come \"istantanea\", o\n"
						"riprendi istantanea finestra \"VideoGlide\" come \"istantanea\"\n"
						"Il formato delle immagini è impostato tramite \"Impostazioni istantanea…\".",
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
                                "come",
                                'kFil',
                                'fss ',
                                "Il nome del file o il percorso del file. In mancanza di "
								"un nome esso verrà richiesto, a meno che non sia "
								"attivata la funzione di nome automatico.",
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
                                "finestra",
                                'kWch',
                                'TEXT',
                                "Il nome della finestra della quale riprendere una "
								"istantanea. In mancanza del nome verrà usata la "
								"finestra in primo piano.",
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
                                "grande",
                                'kLrg',
                                'bool',
                                "La dimensione desiderata per l'istantanea. Se non "
								"specificato viene usata la dimensione corrente del "
								"fotogramma. Se è specificato \"grande\" e il digitizer "
								"lo supporta, verrà prodotta un'immagine specifica più "
								"grande.",
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
