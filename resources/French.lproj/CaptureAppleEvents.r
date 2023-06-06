/****************************************************************************************/
/*																						*/
/*	� EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include "NTZRelease.h"

resource 'aete' (0, "French") {
        0x1,
        0x0,
        english,
        roman,
        {       /* array Suites: 1 element */

                /* [1] */
                "Capturer",
                "",
                'efxc',
                1,
                1,
                {       /* array Events: 3 elements */
                        /* [1] */
                        "commencez enregistrer",
                        "Enregistrer une vid�o, p.ex.\n"
						"commencez enregistrer,\n"
						"commencez enregistrer sous \"myMovie.mov\", ou\n"
						"commencez enregistrer fenetre \"VideoGlide\" sous \"myMovie.mov\"",
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
                                "sous",
                                'kFil',
                                'fss ',
                                "Le nom du fichier ou le r�pertoire. Si vous n'indiquez "
								"pas le nom, un message vous rappelle d'en saisir un "
								"sauf si l'attribution automatique du nom est active.",
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
                                "fenetre",
                                'kWch',
                                'TEXT',
                                "Le nom de la fen�tre � enregistrer. Si le nom n'est pas "
								"pr�cis�, la fen�tre du dessus est enregistr�e.",
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
                                "duree",
                                'kDur',
                                'magn',
                                "Dur�e voulue de l'enregistrement en secondes.",
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
                        "arreter enregistrer",
                        "Arr�te l'enregistrement d'une vid�o, p.ex.\n"
						"arreter enregistrer, ou\n"
						"arreter enregistrer fenetre \"VideoGlide\"",
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
                                "fenetre",
                                'kWch',
                                'TEXT',
                                "Le nom de la fen�tre dont l'enregistrement doit �tre "
								"arr�t�. Si le nom n'est pas pr�cis�, la fen�tre du "
								"dessus est enregistr�e.",
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
                        "faire instantane",
                        "Fait une instantan�, p.ex.\n"
						"faire instantane\n"
						"faire instantane sous \"snapshot\", ou\n"
						"faire instantane fenetre \"VideoGlide\" sous \"instantan�\"\n"
						"Le format de l'image est fix� dans \"Param�tres des instantan��\".",
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
                                "sous",
                                'kFil',
                                'fss ',
                                "Le nom du fichier ou le r�pertoire. Si vous n'indiquez "
								"pas le nom, un message vous rappelle d'en saisir un "
								"sauf si l'attribution automatique du nom est active.",
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
                                "fenetre",
                                'kWch',
                                'TEXT',
                                "Le nom de la fen�tre � capturer. Si le nom n'est pas "
								"pr�cis�, la fen�tre du dessus est enregistr�e.",
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
                                "grand",
                                'kLrg',
                                'bool',
                                "La taille voulue de la instantan�. Par d�faut, "
								"l'image est enregistr�e dans la taille actuelle. Si "
								"vous pr�cisez \"grand\" et si le num�riseur poss�de "
								"cette fonction, une image plus grande sp�cifique au "
								"num�riseur sera enregistr�e.",
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
