/****************************************************************************************/
/*																						*/
/*	© EchoFX, Inc. 2002-2006															*/
/*	All Rights Reserved.																*/
/*																						*/
/****************************************************************************************/

#include "NTZRelease.h"

resource 'aete' (0, "Spanish") {
        0x1,
        0x0,
        english,
        roman,
        {       /* array Suites: 1 element */

                /* [1] */
                "Capturar",
                "",
                'efxc',
                1,
                1,
                {       /* array Events: 3 elements */
                        /* [1] */
                        "iniciar grabacion",
                        "Iniciar grabación de una película, p. ej.,\n"
						"iniciar grabacion,\n"
						"iniciar grabacion como \"myMovie.mov\", o\n"
						"iniciar grabacion ventana \"VideoGlide\" como \"myMovie.mov\"",
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
                                "como",
                                'kFil',
                                'fss ',
                                "El nombre o la ruta del archivo. Si no se proporciona "
								"un nombre, se solicitará al usuario que introduzca uno "
								"a no ser que la función Designación automática esté "
								"activada.",
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
                                "ventana",
                                'kWch',
                                'TEXT',
                                "El nombre de la ventana que se va a grabar. Si no se "
								"proporciona un nombre, se considera que es la ventana "
								"frontal.",
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
                                "duración",
                                'kDur',
                                'magn',
                                "Duración deseada de la grabación en segundos.",
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
                        "detener grabacion",
                        "Detiene la grabación de una película, p. ej.,\n"
						"detener grabacion, o\n"
						"detener grabacion ventana \"VideoGlide\"",
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
                                "ventana",
                                'kWch',
                                'TEXT',
                                "El nombre de la ventana desde la que se debe detener la "
								"grabación. Si no se proporciona un nombre, se considera "
								"que es la ventana frontal.",
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
                        "capturar instantanea",
                        "Captura una instantánea, p. ej.,\n"
						"capturar instantanea,\n"
						"capturar instantanea como \"instantánea\", o\n"
						"capturar instantanea ventana \"VideoGlide\" como \"instantánea\"\n"
						"El formato de la imagen se configura mediante \"Ajustes de instantánea…\".",
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
                                "como",
                                'kFil',
                                'fss ',
                                "El nombre o la ruta del archivo. Si no se proporciona "
								"un nombre, se solicitará al usuario que introduzca uno "
								"a no ser que la función Designación automática esté "
								"activada.",
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
                                "ventana",
                                'kWch',
                                'TEXT',
                                "El nombre de la ventana para la instantánea. Si no se "
								"proporciona un nombre, se considera que es la ventana "
								"frontal.",
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
                                "El tamaño de la instantánea deseada. Si no se "
								"especifica el tamaño, se utiliza el tamaño del "
								"fotograma actual. Si se especifica \"grande\" y es "
								"compatible con el digitalizador, se generará una imagen "
								"más grande específica del digitalizador.",
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
