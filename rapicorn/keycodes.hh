/* fgrep "define GDK" gdkkeysyms.h  | sed -e 's/#define //' -e 's/GDK_/KEY_/g' -e 's/ / = /' -e 's/$/,/' | sort -k 3 */

KEY_space = 0x020,
KEY_exclam = 0x021,
KEY_quotedbl = 0x022,
KEY_numbersign = 0x023,
KEY_dollar = 0x024,
KEY_percent = 0x025,
KEY_ampersand = 0x026,
KEY_apostrophe = 0x027,
KEY_quoteright = 0x027,
KEY_parenleft = 0x028,
KEY_parenright = 0x029,
KEY_asterisk = 0x02a,
KEY_plus = 0x02b,
KEY_comma = 0x02c,
KEY_minus = 0x02d,
KEY_period = 0x02e,
KEY_slash = 0x02f,
KEY_0 = 0x030,
KEY_1 = 0x031,
KEY_2 = 0x032,
KEY_3 = 0x033,
KEY_4 = 0x034,
KEY_5 = 0x035,
KEY_6 = 0x036,
KEY_7 = 0x037,
KEY_8 = 0x038,
KEY_9 = 0x039,
KEY_colon = 0x03a,
KEY_semicolon = 0x03b,
KEY_less = 0x03c,
KEY_equal = 0x03d,
KEY_greater = 0x03e,
KEY_question = 0x03f,
KEY_at = 0x040,
KEY_A = 0x041,
KEY_B = 0x042,
KEY_C = 0x043,
KEY_D = 0x044,
KEY_E = 0x045,
KEY_F = 0x046,
KEY_G = 0x047,
KEY_H = 0x048,
KEY_I = 0x049,
KEY_J = 0x04a,
KEY_K = 0x04b,
KEY_L = 0x04c,
KEY_M = 0x04d,
KEY_N = 0x04e,
KEY_O = 0x04f,
KEY_P = 0x050,
KEY_Q = 0x051,
KEY_R = 0x052,
KEY_S = 0x053,
KEY_T = 0x054,
KEY_U = 0x055,
KEY_V = 0x056,
KEY_W = 0x057,
KEY_X = 0x058,
KEY_Y = 0x059,
KEY_Z = 0x05a,
KEY_bracketleft = 0x05b,
KEY_backslash = 0x05c,
KEY_bracketright = 0x05d,
KEY_asciicircum = 0x05e,
KEY_underscore = 0x05f,
KEY_grave = 0x060,
KEY_quoteleft = 0x060,
KEY_a = 0x061,
KEY_b = 0x062,
KEY_c = 0x063,
KEY_d = 0x064,
KEY_e = 0x065,
KEY_f = 0x066,
KEY_g = 0x067,
KEY_h = 0x068,
KEY_i = 0x069,
KEY_j = 0x06a,
KEY_k = 0x06b,
KEY_l = 0x06c,
KEY_m = 0x06d,
KEY_n = 0x06e,
KEY_o = 0x06f,
KEY_p = 0x070,
KEY_q = 0x071,
KEY_r = 0x072,
KEY_s = 0x073,
KEY_t = 0x074,
KEY_u = 0x075,
KEY_v = 0x076,
KEY_w = 0x077,
KEY_x = 0x078,
KEY_y = 0x079,
KEY_z = 0x07a,
KEY_braceleft = 0x07b,
KEY_bar = 0x07c,
KEY_braceright = 0x07d,
KEY_asciitilde = 0x07e,
KEY_nobreakspace = 0x0a0,
KEY_exclamdown = 0x0a1,
KEY_cent = 0x0a2,
KEY_sterling = 0x0a3,
KEY_currency = 0x0a4,
KEY_yen = 0x0a5,
KEY_brokenbar = 0x0a6,
KEY_section = 0x0a7,
KEY_diaeresis = 0x0a8,
KEY_copyright = 0x0a9,
KEY_ordfeminine = 0x0aa,
KEY_guillemotleft = 0x0ab,
KEY_notsign = 0x0ac,
KEY_hyphen = 0x0ad,
KEY_registered = 0x0ae,
KEY_macron = 0x0af,
KEY_degree = 0x0b0,
KEY_plusminus = 0x0b1,
KEY_twosuperior = 0x0b2,
KEY_threesuperior = 0x0b3,
KEY_acute = 0x0b4,
KEY_mu = 0x0b5,
KEY_paragraph = 0x0b6,
KEY_periodcentered = 0x0b7,
KEY_cedilla = 0x0b8,
KEY_onesuperior = 0x0b9,
KEY_masculine = 0x0ba,
KEY_guillemotright = 0x0bb,
KEY_onequarter = 0x0bc,
KEY_onehalf = 0x0bd,
KEY_threequarters = 0x0be,
KEY_questiondown = 0x0bf,
KEY_Agrave = 0x0c0,
KEY_Aacute = 0x0c1,
KEY_Acircumflex = 0x0c2,
KEY_Atilde = 0x0c3,
KEY_Adiaeresis = 0x0c4,
KEY_Aring = 0x0c5,
KEY_AE = 0x0c6,
KEY_Ccedilla = 0x0c7,
KEY_Egrave = 0x0c8,
KEY_Eacute = 0x0c9,
KEY_Ecircumflex = 0x0ca,
KEY_Ediaeresis = 0x0cb,
KEY_Igrave = 0x0cc,
KEY_Iacute = 0x0cd,
KEY_Icircumflex = 0x0ce,
KEY_Idiaeresis = 0x0cf,
KEY_ETH = 0x0d0,
KEY_Eth = 0x0d0,
KEY_Ntilde = 0x0d1,
KEY_Ograve = 0x0d2,
KEY_Oacute = 0x0d3,
KEY_Ocircumflex = 0x0d4,
KEY_Otilde = 0x0d5,
KEY_Odiaeresis = 0x0d6,
KEY_multiply = 0x0d7,
KEY_Ooblique = 0x0d8,
KEY_Ugrave = 0x0d9,
KEY_Uacute = 0x0da,
KEY_Ucircumflex = 0x0db,
KEY_Udiaeresis = 0x0dc,
KEY_Yacute = 0x0dd,
KEY_THORN = 0x0de,
KEY_Thorn = 0x0de,
KEY_ssharp = 0x0df,
KEY_agrave = 0x0e0,
KEY_aacute = 0x0e1,
KEY_acircumflex = 0x0e2,
KEY_atilde = 0x0e3,
KEY_adiaeresis = 0x0e4,
KEY_aring = 0x0e5,
KEY_ae = 0x0e6,
KEY_ccedilla = 0x0e7,
KEY_egrave = 0x0e8,
KEY_eacute = 0x0e9,
KEY_ecircumflex = 0x0ea,
KEY_ediaeresis = 0x0eb,
KEY_igrave = 0x0ec,
KEY_iacute = 0x0ed,
KEY_icircumflex = 0x0ee,
KEY_idiaeresis = 0x0ef,
KEY_eth = 0x0f0,
KEY_ntilde = 0x0f1,
KEY_ograve = 0x0f2,
KEY_oacute = 0x0f3,
KEY_ocircumflex = 0x0f4,
KEY_otilde = 0x0f5,
KEY_odiaeresis = 0x0f6,
KEY_division = 0x0f7,
KEY_oslash = 0x0f8,
KEY_ugrave = 0x0f9,
KEY_uacute = 0x0fa,
KEY_ucircumflex = 0x0fb,
KEY_udiaeresis = 0x0fc,
KEY_yacute = 0x0fd,
KEY_thorn = 0x0fe,
KEY_ydiaeresis = 0x0ff,
KEY_OE = 0x13bc,
KEY_oe = 0x13bd,
KEY_Ydiaeresis = 0x13be,
KEY_Aogonek = 0x1a1,
KEY_breve = 0x1a2,
KEY_Lstroke = 0x1a3,
KEY_Lcaron = 0x1a5,
KEY_Sacute = 0x1a6,
KEY_Scaron = 0x1a9,
KEY_Scedilla = 0x1aa,
KEY_Tcaron = 0x1ab,
KEY_Zacute = 0x1ac,
KEY_Zcaron = 0x1ae,
KEY_Zabovedot = 0x1af,
KEY_aogonek = 0x1b1,
KEY_ogonek = 0x1b2,
KEY_lstroke = 0x1b3,
KEY_lcaron = 0x1b5,
KEY_sacute = 0x1b6,
KEY_caron = 0x1b7,
KEY_scaron = 0x1b9,
KEY_scedilla = 0x1ba,
KEY_tcaron = 0x1bb,
KEY_zacute = 0x1bc,
KEY_doubleacute = 0x1bd,
KEY_zcaron = 0x1be,
KEY_zabovedot = 0x1bf,
KEY_Racute = 0x1c0,
KEY_Abreve = 0x1c3,
KEY_Lacute = 0x1c5,
KEY_Cacute = 0x1c6,
KEY_Ccaron = 0x1c8,
KEY_Eogonek = 0x1ca,
KEY_Ecaron = 0x1cc,
KEY_Dcaron = 0x1cf,
KEY_Dstroke = 0x1d0,
KEY_Nacute = 0x1d1,
KEY_Ncaron = 0x1d2,
KEY_Odoubleacute = 0x1d5,
KEY_Rcaron = 0x1d8,
KEY_Uring = 0x1d9,
KEY_Udoubleacute = 0x1db,
KEY_Tcedilla = 0x1de,
KEY_racute = 0x1e0,
KEY_abreve = 0x1e3,
KEY_lacute = 0x1e5,
KEY_cacute = 0x1e6,
KEY_ccaron = 0x1e8,
KEY_eogonek = 0x1ea,
KEY_ecaron = 0x1ec,
KEY_dcaron = 0x1ef,
KEY_dstroke = 0x1f0,
KEY_nacute = 0x1f1,
KEY_ncaron = 0x1f2,
KEY_odoubleacute = 0x1f5,
KEY_rcaron = 0x1f8,
KEY_uring = 0x1f9,
KEY_udoubleacute = 0x1fb,
KEY_tcedilla = 0x1fe,
KEY_abovedot = 0x1ff,
KEY_EcuSign = 0x20a0,
KEY_ColonSign = 0x20a1,
KEY_CruzeiroSign = 0x20a2,
KEY_FFrancSign = 0x20a3,
KEY_LiraSign = 0x20a4,
KEY_MillSign = 0x20a5,
KEY_NairaSign = 0x20a6,
KEY_PesetaSign = 0x20a7,
KEY_RupeeSign = 0x20a8,
KEY_WonSign = 0x20a9,
KEY_NewSheqelSign = 0x20aa,
KEY_DongSign = 0x20ab,
KEY_EuroSign = 0x20ac,
KEY_Hstroke = 0x2a1,
KEY_Hcircumflex = 0x2a6,
KEY_Iabovedot = 0x2a9,
KEY_Gbreve = 0x2ab,
KEY_Jcircumflex = 0x2ac,
KEY_hstroke = 0x2b1,
KEY_hcircumflex = 0x2b6,
KEY_idotless = 0x2b9,
KEY_gbreve = 0x2bb,
KEY_jcircumflex = 0x2bc,
KEY_Cabovedot = 0x2c5,
KEY_Ccircumflex = 0x2c6,
KEY_Gabovedot = 0x2d5,
KEY_Gcircumflex = 0x2d8,
KEY_Ubreve = 0x2dd,
KEY_Scircumflex = 0x2de,
KEY_cabovedot = 0x2e5,
KEY_ccircumflex = 0x2e6,
KEY_gabovedot = 0x2f5,
KEY_gcircumflex = 0x2f8,
KEY_ubreve = 0x2fd,
KEY_scircumflex = 0x2fe,
KEY_kappa = 0x3a2,
KEY_kra = 0x3a2,
KEY_Rcedilla = 0x3a3,
KEY_Itilde = 0x3a5,
KEY_Lcedilla = 0x3a6,
KEY_Emacron = 0x3aa,
KEY_Gcedilla = 0x3ab,
KEY_Tslash = 0x3ac,
KEY_rcedilla = 0x3b3,
KEY_itilde = 0x3b5,
KEY_lcedilla = 0x3b6,
KEY_emacron = 0x3ba,
KEY_gcedilla = 0x3bb,
KEY_tslash = 0x3bc,
KEY_ENG = 0x3bd,
KEY_eng = 0x3bf,
KEY_Amacron = 0x3c0,
KEY_Iogonek = 0x3c7,
KEY_Eabovedot = 0x3cc,
KEY_Imacron = 0x3cf,
KEY_Ncedilla = 0x3d1,
KEY_Omacron = 0x3d2,
KEY_Kcedilla = 0x3d3,
KEY_Uogonek = 0x3d9,
KEY_Utilde = 0x3dd,
KEY_Umacron = 0x3de,
KEY_amacron = 0x3e0,
KEY_iogonek = 0x3e7,
KEY_eabovedot = 0x3ec,
KEY_imacron = 0x3ef,
KEY_ncedilla = 0x3f1,
KEY_omacron = 0x3f2,
KEY_kcedilla = 0x3f3,
KEY_uogonek = 0x3f9,
KEY_utilde = 0x3fd,
KEY_umacron = 0x3fe,
KEY_overline = 0x47e,
KEY_kana_fullstop = 0x4a1,
KEY_kana_openingbracket = 0x4a2,
KEY_kana_closingbracket = 0x4a3,
KEY_kana_comma = 0x4a4,
KEY_kana_conjunctive = 0x4a5,
KEY_kana_middledot = 0x4a5,
KEY_kana_WO = 0x4a6,
KEY_kana_a = 0x4a7,
KEY_kana_i = 0x4a8,
KEY_kana_u = 0x4a9,
KEY_kana_e = 0x4aa,
KEY_kana_o = 0x4ab,
KEY_kana_ya = 0x4ac,
KEY_kana_yu = 0x4ad,
KEY_kana_yo = 0x4ae,
KEY_kana_tsu = 0x4af,
KEY_kana_tu = 0x4af,
KEY_prolongedsound = 0x4b0,
KEY_kana_A = 0x4b1,
KEY_kana_I = 0x4b2,
KEY_kana_U = 0x4b3,
KEY_kana_E = 0x4b4,
KEY_kana_O = 0x4b5,
KEY_kana_KA = 0x4b6,
KEY_kana_KI = 0x4b7,
KEY_kana_KU = 0x4b8,
KEY_kana_KE = 0x4b9,
KEY_kana_KO = 0x4ba,
KEY_kana_SA = 0x4bb,
KEY_kana_SHI = 0x4bc,
KEY_kana_SU = 0x4bd,
KEY_kana_SE = 0x4be,
KEY_kana_SO = 0x4bf,
KEY_kana_TA = 0x4c0,
KEY_kana_CHI = 0x4c1,
KEY_kana_TI = 0x4c1,
KEY_kana_TSU = 0x4c2,
KEY_kana_TU = 0x4c2,
KEY_kana_TE = 0x4c3,
KEY_kana_TO = 0x4c4,
KEY_kana_NA = 0x4c5,
KEY_kana_NI = 0x4c6,
KEY_kana_NU = 0x4c7,
KEY_kana_NE = 0x4c8,
KEY_kana_NO = 0x4c9,
KEY_kana_HA = 0x4ca,
KEY_kana_HI = 0x4cb,
KEY_kana_FU = 0x4cc,
KEY_kana_HU = 0x4cc,
KEY_kana_HE = 0x4cd,
KEY_kana_HO = 0x4ce,
KEY_kana_MA = 0x4cf,
KEY_kana_MI = 0x4d0,
KEY_kana_MU = 0x4d1,
KEY_kana_ME = 0x4d2,
KEY_kana_MO = 0x4d3,
KEY_kana_YA = 0x4d4,
KEY_kana_YU = 0x4d5,
KEY_kana_YO = 0x4d6,
KEY_kana_RA = 0x4d7,
KEY_kana_RI = 0x4d8,
KEY_kana_RU = 0x4d9,
KEY_kana_RE = 0x4da,
KEY_kana_RO = 0x4db,
KEY_kana_WA = 0x4dc,
KEY_kana_N = 0x4dd,
KEY_voicedsound = 0x4de,
KEY_semivoicedsound = 0x4df,
KEY_Arabic_comma = 0x5ac,
KEY_Arabic_semicolon = 0x5bb,
KEY_Arabic_question_mark = 0x5bf,
KEY_Arabic_hamza = 0x5c1,
KEY_Arabic_maddaonalef = 0x5c2,
KEY_Arabic_hamzaonalef = 0x5c3,
KEY_Arabic_hamzaonwaw = 0x5c4,
KEY_Arabic_hamzaunderalef = 0x5c5,
KEY_Arabic_hamzaonyeh = 0x5c6,
KEY_Arabic_alef = 0x5c7,
KEY_Arabic_beh = 0x5c8,
KEY_Arabic_tehmarbuta = 0x5c9,
KEY_Arabic_teh = 0x5ca,
KEY_Arabic_theh = 0x5cb,
KEY_Arabic_jeem = 0x5cc,
KEY_Arabic_hah = 0x5cd,
KEY_Arabic_khah = 0x5ce,
KEY_Arabic_dal = 0x5cf,
KEY_Arabic_thal = 0x5d0,
KEY_Arabic_ra = 0x5d1,
KEY_Arabic_zain = 0x5d2,
KEY_Arabic_seen = 0x5d3,
KEY_Arabic_sheen = 0x5d4,
KEY_Arabic_sad = 0x5d5,
KEY_Arabic_dad = 0x5d6,
KEY_Arabic_tah = 0x5d7,
KEY_Arabic_zah = 0x5d8,
KEY_Arabic_ain = 0x5d9,
KEY_Arabic_ghain = 0x5da,
KEY_Arabic_tatweel = 0x5e0,
KEY_Arabic_feh = 0x5e1,
KEY_Arabic_qaf = 0x5e2,
KEY_Arabic_kaf = 0x5e3,
KEY_Arabic_lam = 0x5e4,
KEY_Arabic_meem = 0x5e5,
KEY_Arabic_noon = 0x5e6,
KEY_Arabic_ha = 0x5e7,
KEY_Arabic_heh = 0x5e7,
KEY_Arabic_waw = 0x5e8,
KEY_Arabic_alefmaksura = 0x5e9,
KEY_Arabic_yeh = 0x5ea,
KEY_Arabic_fathatan = 0x5eb,
KEY_Arabic_dammatan = 0x5ec,
KEY_Arabic_kasratan = 0x5ed,
KEY_Arabic_fatha = 0x5ee,
KEY_Arabic_damma = 0x5ef,
KEY_Arabic_kasra = 0x5f0,
KEY_Arabic_shadda = 0x5f1,
KEY_Arabic_sukun = 0x5f2,
KEY_Serbian_dje = 0x6a1,
KEY_Macedonia_gje = 0x6a2,
KEY_Cyrillic_io = 0x6a3,
KEY_Ukrainian_ie = 0x6a4,
KEY_Ukranian_je = 0x6a4,
KEY_Macedonia_dse = 0x6a5,
KEY_Ukrainian_i = 0x6a6,
KEY_Ukranian_i = 0x6a6,
KEY_Ukrainian_yi = 0x6a7,
KEY_Ukranian_yi = 0x6a7,
KEY_Cyrillic_je = 0x6a8,
KEY_Serbian_je = 0x6a8,
KEY_Cyrillic_lje = 0x6a9,
KEY_Serbian_lje = 0x6a9,
KEY_Cyrillic_nje = 0x6aa,
KEY_Serbian_nje = 0x6aa,
KEY_Serbian_tshe = 0x6ab,
KEY_Macedonia_kje = 0x6ac,
KEY_Ukrainian_ghe_with_upturn = 0x6ad,
KEY_Byelorussian_shortu = 0x6ae,
KEY_Cyrillic_dzhe = 0x6af,
KEY_Serbian_dze = 0x6af,
KEY_numerosign = 0x6b0,
KEY_Serbian_DJE = 0x6b1,
KEY_Macedonia_GJE = 0x6b2,
KEY_Cyrillic_IO = 0x6b3,
KEY_Ukrainian_IE = 0x6b4,
KEY_Ukranian_JE = 0x6b4,
KEY_Macedonia_DSE = 0x6b5,
KEY_Ukrainian_I = 0x6b6,
KEY_Ukranian_I = 0x6b6,
KEY_Ukrainian_YI = 0x6b7,
KEY_Ukranian_YI = 0x6b7,
KEY_Cyrillic_JE = 0x6b8,
KEY_Serbian_JE = 0x6b8,
KEY_Cyrillic_LJE = 0x6b9,
KEY_Serbian_LJE = 0x6b9,
KEY_Cyrillic_NJE = 0x6ba,
KEY_Serbian_NJE = 0x6ba,
KEY_Serbian_TSHE = 0x6bb,
KEY_Macedonia_KJE = 0x6bc,
KEY_Ukrainian_GHE_WITH_UPTURN = 0x6bd,
KEY_Byelorussian_SHORTU = 0x6be,
KEY_Cyrillic_DZHE = 0x6bf,
KEY_Serbian_DZE = 0x6bf,
KEY_Cyrillic_yu = 0x6c0,
KEY_Cyrillic_a = 0x6c1,
KEY_Cyrillic_be = 0x6c2,
KEY_Cyrillic_tse = 0x6c3,
KEY_Cyrillic_de = 0x6c4,
KEY_Cyrillic_ie = 0x6c5,
KEY_Cyrillic_ef = 0x6c6,
KEY_Cyrillic_ghe = 0x6c7,
KEY_Cyrillic_ha = 0x6c8,
KEY_Cyrillic_i = 0x6c9,
KEY_Cyrillic_shorti = 0x6ca,
KEY_Cyrillic_ka = 0x6cb,
KEY_Cyrillic_el = 0x6cc,
KEY_Cyrillic_em = 0x6cd,
KEY_Cyrillic_en = 0x6ce,
KEY_Cyrillic_o = 0x6cf,
KEY_Cyrillic_pe = 0x6d0,
KEY_Cyrillic_ya = 0x6d1,
KEY_Cyrillic_er = 0x6d2,
KEY_Cyrillic_es = 0x6d3,
KEY_Cyrillic_te = 0x6d4,
KEY_Cyrillic_u = 0x6d5,
KEY_Cyrillic_zhe = 0x6d6,
KEY_Cyrillic_ve = 0x6d7,
KEY_Cyrillic_softsign = 0x6d8,
KEY_Cyrillic_yeru = 0x6d9,
KEY_Cyrillic_ze = 0x6da,
KEY_Cyrillic_sha = 0x6db,
KEY_Cyrillic_e = 0x6dc,
KEY_Cyrillic_shcha = 0x6dd,
KEY_Cyrillic_che = 0x6de,
KEY_Cyrillic_hardsign = 0x6df,
KEY_Cyrillic_YU = 0x6e0,
KEY_Cyrillic_A = 0x6e1,
KEY_Cyrillic_BE = 0x6e2,
KEY_Cyrillic_TSE = 0x6e3,
KEY_Cyrillic_DE = 0x6e4,
KEY_Cyrillic_IE = 0x6e5,
KEY_Cyrillic_EF = 0x6e6,
KEY_Cyrillic_GHE = 0x6e7,
KEY_Cyrillic_HA = 0x6e8,
KEY_Cyrillic_I = 0x6e9,
KEY_Cyrillic_SHORTI = 0x6ea,
KEY_Cyrillic_KA = 0x6eb,
KEY_Cyrillic_EL = 0x6ec,
KEY_Cyrillic_EM = 0x6ed,
KEY_Cyrillic_EN = 0x6ee,
KEY_Cyrillic_O = 0x6ef,
KEY_Cyrillic_PE = 0x6f0,
KEY_Cyrillic_YA = 0x6f1,
KEY_Cyrillic_ER = 0x6f2,
KEY_Cyrillic_ES = 0x6f3,
KEY_Cyrillic_TE = 0x6f4,
KEY_Cyrillic_U = 0x6f5,
KEY_Cyrillic_ZHE = 0x6f6,
KEY_Cyrillic_VE = 0x6f7,
KEY_Cyrillic_SOFTSIGN = 0x6f8,
KEY_Cyrillic_YERU = 0x6f9,
KEY_Cyrillic_ZE = 0x6fa,
KEY_Cyrillic_SHA = 0x6fb,
KEY_Cyrillic_E = 0x6fc,
KEY_Cyrillic_SHCHA = 0x6fd,
KEY_Cyrillic_CHE = 0x6fe,
KEY_Cyrillic_HARDSIGN = 0x6ff,
KEY_Greek_ALPHAaccent = 0x7a1,
KEY_Greek_EPSILONaccent = 0x7a2,
KEY_Greek_ETAaccent = 0x7a3,
KEY_Greek_IOTAaccent = 0x7a4,
KEY_Greek_IOTAdieresis = 0x7a5,
KEY_Greek_OMICRONaccent = 0x7a7,
KEY_Greek_UPSILONaccent = 0x7a8,
KEY_Greek_UPSILONdieresis = 0x7a9,
KEY_Greek_OMEGAaccent = 0x7ab,
KEY_Greek_accentdieresis = 0x7ae,
KEY_Greek_horizbar = 0x7af,
KEY_Greek_alphaaccent = 0x7b1,
KEY_Greek_epsilonaccent = 0x7b2,
KEY_Greek_etaaccent = 0x7b3,
KEY_Greek_iotaaccent = 0x7b4,
KEY_Greek_iotadieresis = 0x7b5,
KEY_Greek_iotaaccentdieresis = 0x7b6,
KEY_Greek_omicronaccent = 0x7b7,
KEY_Greek_upsilonaccent = 0x7b8,
KEY_Greek_upsilondieresis = 0x7b9,
KEY_Greek_upsilonaccentdieresis = 0x7ba,
KEY_Greek_omegaaccent = 0x7bb,
KEY_Greek_ALPHA = 0x7c1,
KEY_Greek_BETA = 0x7c2,
KEY_Greek_GAMMA = 0x7c3,
KEY_Greek_DELTA = 0x7c4,
KEY_Greek_EPSILON = 0x7c5,
KEY_Greek_ZETA = 0x7c6,
KEY_Greek_ETA = 0x7c7,
KEY_Greek_THETA = 0x7c8,
KEY_Greek_IOTA = 0x7c9,
KEY_Greek_KAPPA = 0x7ca,
KEY_Greek_LAMBDA = 0x7cb,
KEY_Greek_LAMDA = 0x7cb,
KEY_Greek_MU = 0x7cc,
KEY_Greek_NU = 0x7cd,
KEY_Greek_XI = 0x7ce,
KEY_Greek_OMICRON = 0x7cf,
KEY_Greek_PI = 0x7d0,
KEY_Greek_RHO = 0x7d1,
KEY_Greek_SIGMA = 0x7d2,
KEY_Greek_TAU = 0x7d4,
KEY_Greek_UPSILON = 0x7d5,
KEY_Greek_PHI = 0x7d6,
KEY_Greek_CHI = 0x7d7,
KEY_Greek_PSI = 0x7d8,
KEY_Greek_OMEGA = 0x7d9,
KEY_Greek_alpha = 0x7e1,
KEY_Greek_beta = 0x7e2,
KEY_Greek_gamma = 0x7e3,
KEY_Greek_delta = 0x7e4,
KEY_Greek_epsilon = 0x7e5,
KEY_Greek_zeta = 0x7e6,
KEY_Greek_eta = 0x7e7,
KEY_Greek_theta = 0x7e8,
KEY_Greek_iota = 0x7e9,
KEY_Greek_kappa = 0x7ea,
KEY_Greek_lambda = 0x7eb,
KEY_Greek_lamda = 0x7eb,
KEY_Greek_mu = 0x7ec,
KEY_Greek_nu = 0x7ed,
KEY_Greek_xi = 0x7ee,
KEY_Greek_omicron = 0x7ef,
KEY_Greek_pi = 0x7f0,
KEY_Greek_rho = 0x7f1,
KEY_Greek_sigma = 0x7f2,
KEY_Greek_finalsmallsigma = 0x7f3,
KEY_Greek_tau = 0x7f4,
KEY_Greek_upsilon = 0x7f5,
KEY_Greek_phi = 0x7f6,
KEY_Greek_chi = 0x7f7,
KEY_Greek_psi = 0x7f8,
KEY_Greek_omega = 0x7f9,
KEY_leftradical = 0x8a1,
KEY_topleftradical = 0x8a2,
KEY_horizconnector = 0x8a3,
KEY_topintegral = 0x8a4,
KEY_botintegral = 0x8a5,
KEY_vertconnector = 0x8a6,
KEY_topleftsqbracket = 0x8a7,
KEY_botleftsqbracket = 0x8a8,
KEY_toprightsqbracket = 0x8a9,
KEY_botrightsqbracket = 0x8aa,
KEY_topleftparens = 0x8ab,
KEY_botleftparens = 0x8ac,
KEY_toprightparens = 0x8ad,
KEY_botrightparens = 0x8ae,
KEY_leftmiddlecurlybrace = 0x8af,
KEY_rightmiddlecurlybrace = 0x8b0,
KEY_topleftsummation = 0x8b1,
KEY_botleftsummation = 0x8b2,
KEY_topvertsummationconnector = 0x8b3,
KEY_botvertsummationconnector = 0x8b4,
KEY_toprightsummation = 0x8b5,
KEY_botrightsummation = 0x8b6,
KEY_rightmiddlesummation = 0x8b7,
KEY_lessthanequal = 0x8bc,
KEY_notequal = 0x8bd,
KEY_greaterthanequal = 0x8be,
KEY_integral = 0x8bf,
KEY_therefore = 0x8c0,
KEY_variation = 0x8c1,
KEY_infinity = 0x8c2,
KEY_nabla = 0x8c5,
KEY_approximate = 0x8c8,
KEY_similarequal = 0x8c9,
KEY_ifonlyif = 0x8cd,
KEY_implies = 0x8ce,
KEY_identical = 0x8cf,
KEY_radical = 0x8d6,
KEY_includedin = 0x8da,
KEY_includes = 0x8db,
KEY_intersection = 0x8dc,
KEY_union = 0x8dd,
KEY_logicaland = 0x8de,
KEY_logicalor = 0x8df,
KEY_partialderivative = 0x8ef,
KEY_function = 0x8f6,
KEY_leftarrow = 0x8fb,
KEY_uparrow = 0x8fc,
KEY_rightarrow = 0x8fd,
KEY_downarrow = 0x8fe,
KEY_blank = 0x9df,
KEY_soliddiamond = 0x9e0,
KEY_checkerboard = 0x9e1,
KEY_ht = 0x9e2,
KEY_ff = 0x9e3,
KEY_cr = 0x9e4,
KEY_lf = 0x9e5,
KEY_nl = 0x9e8,
KEY_vt = 0x9e9,
KEY_lowrightcorner = 0x9ea,
KEY_uprightcorner = 0x9eb,
KEY_upleftcorner = 0x9ec,
KEY_lowleftcorner = 0x9ed,
KEY_crossinglines = 0x9ee,
KEY_horizlinescan1 = 0x9ef,
KEY_horizlinescan3 = 0x9f0,
KEY_horizlinescan5 = 0x9f1,
KEY_horizlinescan7 = 0x9f2,
KEY_horizlinescan9 = 0x9f3,
KEY_leftt = 0x9f4,
KEY_rightt = 0x9f5,
KEY_bott = 0x9f6,
KEY_topt = 0x9f7,
KEY_vertbar = 0x9f8,
KEY_3270_Duplicate = 0xFD01,
KEY_3270_FieldMark = 0xFD02,
KEY_3270_Right2 = 0xFD03,
KEY_3270_Left2 = 0xFD04,
KEY_3270_BackTab = 0xFD05,
KEY_3270_EraseEOF = 0xFD06,
KEY_3270_EraseInput = 0xFD07,
KEY_3270_Reset = 0xFD08,
KEY_3270_Quit = 0xFD09,
KEY_3270_PA1 = 0xFD0A,
KEY_3270_PA2 = 0xFD0B,
KEY_3270_PA3 = 0xFD0C,
KEY_3270_Test = 0xFD0D,
KEY_3270_Attn = 0xFD0E,
KEY_3270_CursorBlink = 0xFD0F,
KEY_3270_AltCursor = 0xFD10,
KEY_3270_KeyClick = 0xFD11,
KEY_3270_Jump = 0xFD12,
KEY_3270_Ident = 0xFD13,
KEY_3270_Rule = 0xFD14,
KEY_3270_Copy = 0xFD15,
KEY_3270_Play = 0xFD16,
KEY_3270_Setup = 0xFD17,
KEY_3270_Record = 0xFD18,
KEY_3270_ChangeScreen = 0xFD19,
KEY_3270_DeleteWord = 0xFD1A,
KEY_3270_ExSelect = 0xFD1B,
KEY_3270_CursorSelect = 0xFD1C,
KEY_3270_PrintScreen = 0xFD1D,
KEY_3270_Enter = 0xFD1E,
KEY_ISO_Lock = 0xFE01,
KEY_ISO_Level2_Latch = 0xFE02,
KEY_ISO_Level3_Shift = 0xFE03,
KEY_ISO_Level3_Latch = 0xFE04,
KEY_ISO_Level3_Lock = 0xFE05,
KEY_ISO_Group_Latch = 0xFE06,
KEY_ISO_Group_Lock = 0xFE07,
KEY_ISO_Next_Group = 0xFE08,
KEY_ISO_Next_Group_Lock = 0xFE09,
KEY_ISO_Prev_Group = 0xFE0A,
KEY_ISO_Prev_Group_Lock = 0xFE0B,
KEY_ISO_First_Group = 0xFE0C,
KEY_ISO_First_Group_Lock = 0xFE0D,
KEY_ISO_Last_Group = 0xFE0E,
KEY_ISO_Last_Group_Lock = 0xFE0F,
KEY_ISO_Left_Tab = 0xFE20,
KEY_ISO_Move_Line_Up = 0xFE21,
KEY_ISO_Move_Line_Down = 0xFE22,
KEY_ISO_Partial_Line_Up = 0xFE23,
KEY_ISO_Partial_Line_Down = 0xFE24,
KEY_ISO_Partial_Space_Left = 0xFE25,
KEY_ISO_Partial_Space_Right = 0xFE26,
KEY_ISO_Set_Margin_Left = 0xFE27,
KEY_ISO_Set_Margin_Right = 0xFE28,
KEY_ISO_Release_Margin_Left = 0xFE29,
KEY_ISO_Release_Margin_Right = 0xFE2A,
KEY_ISO_Release_Both_Margins = 0xFE2B,
KEY_ISO_Fast_Cursor_Left = 0xFE2C,
KEY_ISO_Fast_Cursor_Right = 0xFE2D,
KEY_ISO_Fast_Cursor_Up = 0xFE2E,
KEY_ISO_Fast_Cursor_Down = 0xFE2F,
KEY_ISO_Continuous_Underline = 0xFE30,
KEY_ISO_Discontinuous_Underline = 0xFE31,
KEY_ISO_Emphasize = 0xFE32,
KEY_ISO_Center_Object = 0xFE33,
KEY_ISO_Enter = 0xFE34,
KEY_dead_grave = 0xFE50,
KEY_dead_acute = 0xFE51,
KEY_dead_circumflex = 0xFE52,
KEY_dead_tilde = 0xFE53,
KEY_dead_macron = 0xFE54,
KEY_dead_breve = 0xFE55,
KEY_dead_abovedot = 0xFE56,
KEY_dead_diaeresis = 0xFE57,
KEY_dead_abovering = 0xFE58,
KEY_dead_doubleacute = 0xFE59,
KEY_dead_caron = 0xFE5A,
KEY_dead_cedilla = 0xFE5B,
KEY_dead_ogonek = 0xFE5C,
KEY_dead_iota = 0xFE5D,
KEY_dead_voiced_sound = 0xFE5E,
KEY_dead_semivoiced_sound = 0xFE5F,
KEY_dead_belowdot = 0xFE60,
KEY_AccessX_Enable = 0xFE70,
KEY_AccessX_Feedback_Enable = 0xFE71,
KEY_RepeatKeys_Enable = 0xFE72,
KEY_SlowKeys_Enable = 0xFE73,
KEY_BounceKeys_Enable = 0xFE74,
KEY_StickyKeys_Enable = 0xFE75,
KEY_MouseKeys_Enable = 0xFE76,
KEY_MouseKeys_Accel_Enable = 0xFE77,
KEY_Overlay1_Enable = 0xFE78,
KEY_Overlay2_Enable = 0xFE79,
KEY_AudibleBell_Enable = 0xFE7A,
KEY_First_Virtual_Screen = 0xFED0,
KEY_Prev_Virtual_Screen = 0xFED1,
KEY_Next_Virtual_Screen = 0xFED2,
KEY_Last_Virtual_Screen = 0xFED4,
KEY_Terminate_Server = 0xFED5,
KEY_Pointer_Left = 0xFEE0,
KEY_Pointer_Right = 0xFEE1,
KEY_Pointer_Up = 0xFEE2,
KEY_Pointer_Down = 0xFEE3,
KEY_Pointer_UpLeft = 0xFEE4,
KEY_Pointer_UpRight = 0xFEE5,
KEY_Pointer_DownLeft = 0xFEE6,
KEY_Pointer_DownRight = 0xFEE7,
KEY_Pointer_Button_Dflt = 0xFEE8,
KEY_Pointer_Button1 = 0xFEE9,
KEY_Pointer_Button2 = 0xFEEA,
KEY_Pointer_Button3 = 0xFEEB,
KEY_Pointer_Button4 = 0xFEEC,
KEY_Pointer_Button5 = 0xFEED,
KEY_Pointer_DblClick_Dflt = 0xFEEE,
KEY_Pointer_DblClick1 = 0xFEEF,
KEY_Pointer_DblClick2 = 0xFEF0,
KEY_Pointer_DblClick3 = 0xFEF1,
KEY_Pointer_DblClick4 = 0xFEF2,
KEY_Pointer_DblClick5 = 0xFEF3,
KEY_Pointer_Drag_Dflt = 0xFEF4,
KEY_Pointer_Drag1 = 0xFEF5,
KEY_Pointer_Drag2 = 0xFEF6,
KEY_Pointer_Drag3 = 0xFEF7,
KEY_Pointer_Drag4 = 0xFEF8,
KEY_Pointer_EnableKeys = 0xFEF9,
KEY_Pointer_Accelerate = 0xFEFA,
KEY_Pointer_DfltBtnNext = 0xFEFB,
KEY_Pointer_DfltBtnPrev = 0xFEFC,
KEY_Pointer_Drag5 = 0xFEFD,
KEY_BackSpace = 0xFF08,
KEY_Tab = 0xFF09,
KEY_Linefeed = 0xFF0A,
KEY_Clear = 0xFF0B,
KEY_Return = 0xFF0D,
KEY_Pause = 0xFF13,
KEY_Scroll_Lock = 0xFF14,
KEY_Sys_Req = 0xFF15,
KEY_Escape = 0xFF1B,
KEY_Multi_key = 0xFF20,
KEY_Kanji = 0xFF21,
KEY_Muhenkan = 0xFF22,
KEY_Henkan = 0xFF23,
KEY_Henkan_Mode = 0xFF23,
KEY_Romaji = 0xFF24,
KEY_Hiragana = 0xFF25,
KEY_Katakana = 0xFF26,
KEY_Hiragana_Katakana = 0xFF27,
KEY_Zenkaku = 0xFF28,
KEY_Hankaku = 0xFF29,
KEY_Zenkaku_Hankaku = 0xFF2A,
KEY_Touroku = 0xFF2B,
KEY_Massyo = 0xFF2C,
KEY_Kana_Lock = 0xFF2D,
KEY_Kana_Shift = 0xFF2E,
KEY_Eisu_Shift = 0xFF2F,
KEY_Eisu_toggle = 0xFF30,
KEY_Codeinput = 0xFF37,
KEY_Kanji_Bangou = 0xFF37,
KEY_SingleCandidate = 0xFF3C,
KEY_MultipleCandidate = 0xFF3D,
KEY_Zen_Koho = 0xFF3D,
KEY_Mae_Koho = 0xFF3E,
KEY_PreviousCandidate = 0xFF3E,
KEY_Home = 0xFF50,
KEY_Left = 0xFF51,
KEY_Up = 0xFF52,
KEY_Right = 0xFF53,
KEY_Down = 0xFF54,
KEY_Page_Up = 0xFF55,
KEY_Prior = 0xFF55,
KEY_Next = 0xFF56,
KEY_Page_Down = 0xFF56,
KEY_End = 0xFF57,
KEY_Begin = 0xFF58,
KEY_Select = 0xFF60,
KEY_Print = 0xFF61,
KEY_Execute = 0xFF62,
KEY_Insert = 0xFF63,
KEY_Undo = 0xFF65,
KEY_Redo = 0xFF66,
KEY_Menu = 0xFF67,
KEY_Find = 0xFF68,
KEY_Cancel = 0xFF69,
KEY_Help = 0xFF6A,
KEY_Break = 0xFF6B,
KEY_Arabic_switch = 0xFF7E,
KEY_Greek_switch = 0xFF7E,
KEY_Hangul_switch = 0xFF7E,
KEY_Hebrew_switch = 0xFF7E,
KEY_ISO_Group_Shift = 0xFF7E,
KEY_Mode_switch = 0xFF7E,
KEY_kana_switch = 0xFF7E,
KEY_script_switch = 0xFF7E,
KEY_Num_Lock = 0xFF7F,
KEY_KP_Space = 0xFF80,
KEY_KP_Tab = 0xFF89,
KEY_KP_Enter = 0xFF8D,
KEY_KP_F1 = 0xFF91,
KEY_KP_F2 = 0xFF92,
KEY_KP_F3 = 0xFF93,
KEY_KP_F4 = 0xFF94,
KEY_KP_Home = 0xFF95,
KEY_KP_Left = 0xFF96,
KEY_KP_Up = 0xFF97,
KEY_KP_Right = 0xFF98,
KEY_KP_Down = 0xFF99,
KEY_KP_Page_Up = 0xFF9A,
KEY_KP_Prior = 0xFF9A,
KEY_KP_Next = 0xFF9B,
KEY_KP_Page_Down = 0xFF9B,
KEY_KP_End = 0xFF9C,
KEY_KP_Begin = 0xFF9D,
KEY_KP_Insert = 0xFF9E,
KEY_KP_Delete = 0xFF9F,
KEY_KP_Multiply = 0xFFAA,
KEY_KP_Add = 0xFFAB,
KEY_KP_Separator = 0xFFAC,
KEY_KP_Subtract = 0xFFAD,
KEY_KP_Decimal = 0xFFAE,
KEY_KP_Divide = 0xFFAF,
KEY_KP_0 = 0xFFB0,
KEY_KP_1 = 0xFFB1,
KEY_KP_2 = 0xFFB2,
KEY_KP_3 = 0xFFB3,
KEY_KP_4 = 0xFFB4,
KEY_KP_5 = 0xFFB5,
KEY_KP_6 = 0xFFB6,
KEY_KP_7 = 0xFFB7,
KEY_KP_8 = 0xFFB8,
KEY_KP_9 = 0xFFB9,
KEY_KP_Equal = 0xFFBD,
KEY_F1 = 0xFFBE,
KEY_F2 = 0xFFBF,
KEY_F3 = 0xFFC0,
KEY_F4 = 0xFFC1,
KEY_F5 = 0xFFC2,
KEY_F6 = 0xFFC3,
KEY_F7 = 0xFFC4,
KEY_F8 = 0xFFC5,
KEY_F9 = 0xFFC6,
KEY_F10 = 0xFFC7,
KEY_F11 = 0xFFC8,
KEY_L1 = 0xFFC8,
KEY_F12 = 0xFFC9,
KEY_L2 = 0xFFC9,
KEY_F13 = 0xFFCA,
KEY_L3 = 0xFFCA,
KEY_F14 = 0xFFCB,
KEY_L4 = 0xFFCB,
KEY_F15 = 0xFFCC,
KEY_L5 = 0xFFCC,
KEY_F16 = 0xFFCD,
KEY_L6 = 0xFFCD,
KEY_F17 = 0xFFCE,
KEY_L7 = 0xFFCE,
KEY_F18 = 0xFFCF,
KEY_L8 = 0xFFCF,
KEY_F19 = 0xFFD0,
KEY_L9 = 0xFFD0,
KEY_F20 = 0xFFD1,
KEY_L10 = 0xFFD1,
KEY_F21 = 0xFFD2,
KEY_R1 = 0xFFD2,
KEY_F22 = 0xFFD3,
KEY_R2 = 0xFFD3,
KEY_F23 = 0xFFD4,
KEY_R3 = 0xFFD4,
KEY_F24 = 0xFFD5,
KEY_R4 = 0xFFD5,
KEY_F25 = 0xFFD6,
KEY_R5 = 0xFFD6,
KEY_F26 = 0xFFD7,
KEY_R6 = 0xFFD7,
KEY_F27 = 0xFFD8,
KEY_R7 = 0xFFD8,
KEY_F28 = 0xFFD9,
KEY_R8 = 0xFFD9,
KEY_F29 = 0xFFDA,
KEY_R9 = 0xFFDA,
KEY_F30 = 0xFFDB,
KEY_R10 = 0xFFDB,
KEY_F31 = 0xFFDC,
KEY_R11 = 0xFFDC,
KEY_F32 = 0xFFDD,
KEY_R12 = 0xFFDD,
KEY_F33 = 0xFFDE,
KEY_R13 = 0xFFDE,
KEY_F34 = 0xFFDF,
KEY_R14 = 0xFFDF,
KEY_F35 = 0xFFE0,
KEY_R15 = 0xFFE0,
KEY_Shift_L = 0xFFE1,
KEY_Shift_R = 0xFFE2,
KEY_Control_L = 0xFFE3,
KEY_Control_R = 0xFFE4,
KEY_Caps_Lock = 0xFFE5,
KEY_Shift_Lock = 0xFFE6,
KEY_Meta_L = 0xFFE7,
KEY_Meta_R = 0xFFE8,
KEY_Alt_L = 0xFFE9,
KEY_Alt_R = 0xFFEA,
KEY_Super_L = 0xFFEB,
KEY_Super_R = 0xFFEC,
KEY_Hyper_L = 0xFFED,
KEY_Hyper_R = 0xFFEE,
KEY_Delete = 0xFFFF,
KEY_VoidSymbol = 0xFFFFFF,
KEY_emspace = 0xaa1,
KEY_enspace = 0xaa2,
KEY_em3space = 0xaa3,
KEY_em4space = 0xaa4,
KEY_digitspace = 0xaa5,
KEY_punctspace = 0xaa6,
KEY_thinspace = 0xaa7,
KEY_hairspace = 0xaa8,
KEY_emdash = 0xaa9,
KEY_endash = 0xaaa,
KEY_signifblank = 0xaac,
KEY_ellipsis = 0xaae,
KEY_doubbaselinedot = 0xaaf,
KEY_onethird = 0xab0,
KEY_twothirds = 0xab1,
KEY_onefifth = 0xab2,
KEY_twofifths = 0xab3,
KEY_threefifths = 0xab4,
KEY_fourfifths = 0xab5,
KEY_onesixth = 0xab6,
KEY_fivesixths = 0xab7,
KEY_careof = 0xab8,
KEY_figdash = 0xabb,
KEY_leftanglebracket = 0xabc,
KEY_decimalpoint = 0xabd,
KEY_rightanglebracket = 0xabe,
KEY_marker = 0xabf,
KEY_oneeighth = 0xac3,
KEY_threeeighths = 0xac4,
KEY_fiveeighths = 0xac5,
KEY_seveneighths = 0xac6,
KEY_trademark = 0xac9,
KEY_signaturemark = 0xaca,
KEY_trademarkincircle = 0xacb,
KEY_leftopentriangle = 0xacc,
KEY_rightopentriangle = 0xacd,
KEY_emopencircle = 0xace,
KEY_emopenrectangle = 0xacf,
KEY_leftsinglequotemark = 0xad0,
KEY_rightsinglequotemark = 0xad1,
KEY_leftdoublequotemark = 0xad2,
KEY_rightdoublequotemark = 0xad3,
KEY_prescription = 0xad4,
KEY_minutes = 0xad6,
KEY_seconds = 0xad7,
KEY_latincross = 0xad9,
KEY_hexagram = 0xada,
KEY_filledrectbullet = 0xadb,
KEY_filledlefttribullet = 0xadc,
KEY_filledrighttribullet = 0xadd,
KEY_emfilledcircle = 0xade,
KEY_emfilledrect = 0xadf,
KEY_enopencircbullet = 0xae0,
KEY_enopensquarebullet = 0xae1,
KEY_openrectbullet = 0xae2,
KEY_opentribulletup = 0xae3,
KEY_opentribulletdown = 0xae4,
KEY_openstar = 0xae5,
KEY_enfilledcircbullet = 0xae6,
KEY_enfilledsqbullet = 0xae7,
KEY_filledtribulletup = 0xae8,
KEY_filledtribulletdown = 0xae9,
KEY_leftpointer = 0xaea,
KEY_rightpointer = 0xaeb,
KEY_club = 0xaec,
KEY_diamond = 0xaed,
KEY_heart = 0xaee,
KEY_maltesecross = 0xaf0,
KEY_dagger = 0xaf1,
KEY_doubledagger = 0xaf2,
KEY_checkmark = 0xaf3,
KEY_ballotcross = 0xaf4,
KEY_musicalsharp = 0xaf5,
KEY_musicalflat = 0xaf6,
KEY_malesymbol = 0xaf7,
KEY_femalesymbol = 0xaf8,
KEY_telephone = 0xaf9,
KEY_telephonerecorder = 0xafa,
KEY_phonographcopyright = 0xafb,
KEY_caret = 0xafc,
KEY_singlelowquotemark = 0xafd,
KEY_doublelowquotemark = 0xafe,
KEY_cursor = 0xaff,
KEY_leftcaret = 0xba3,
KEY_rightcaret = 0xba6,
KEY_downcaret = 0xba8,
KEY_upcaret = 0xba9,
KEY_overbar = 0xbc0,
KEY_downtack = 0xbc2,
KEY_upshoe = 0xbc3,
KEY_downstile = 0xbc4,
KEY_underbar = 0xbc6,
KEY_jot = 0xbca,
KEY_quad = 0xbcc,
KEY_uptack = 0xbce,
KEY_circle = 0xbcf,
KEY_upstile = 0xbd3,
KEY_downshoe = 0xbd6,
KEY_rightshoe = 0xbd8,
KEY_leftshoe = 0xbda,
KEY_lefttack = 0xbdc,
KEY_righttack = 0xbfc,
KEY_hebrew_doublelowline = 0xcdf,
KEY_hebrew_aleph = 0xce0,
KEY_hebrew_bet = 0xce1,
KEY_hebrew_beth = 0xce1,
KEY_hebrew_gimel = 0xce2,
KEY_hebrew_gimmel = 0xce2,
KEY_hebrew_dalet = 0xce3,
KEY_hebrew_daleth = 0xce3,
KEY_hebrew_he = 0xce4,
KEY_hebrew_waw = 0xce5,
KEY_hebrew_zain = 0xce6,
KEY_hebrew_zayin = 0xce6,
KEY_hebrew_chet = 0xce7,
KEY_hebrew_het = 0xce7,
KEY_hebrew_tet = 0xce8,
KEY_hebrew_teth = 0xce8,
KEY_hebrew_yod = 0xce9,
KEY_hebrew_finalkaph = 0xcea,
KEY_hebrew_kaph = 0xceb,
KEY_hebrew_lamed = 0xcec,
KEY_hebrew_finalmem = 0xced,
KEY_hebrew_mem = 0xcee,
KEY_hebrew_finalnun = 0xcef,
KEY_hebrew_nun = 0xcf0,
KEY_hebrew_samech = 0xcf1,
KEY_hebrew_samekh = 0xcf1,
KEY_hebrew_ayin = 0xcf2,
KEY_hebrew_finalpe = 0xcf3,
KEY_hebrew_pe = 0xcf4,
KEY_hebrew_finalzade = 0xcf5,
KEY_hebrew_finalzadi = 0xcf5,
KEY_hebrew_zade = 0xcf6,
KEY_hebrew_zadi = 0xcf6,
KEY_hebrew_kuf = 0xcf7,
KEY_hebrew_qoph = 0xcf7,
KEY_hebrew_resh = 0xcf8,
KEY_hebrew_shin = 0xcf9,
KEY_hebrew_taf = 0xcfa,
KEY_hebrew_taw = 0xcfa,
KEY_Thai_kokai = 0xda1,
KEY_Thai_khokhai = 0xda2,
KEY_Thai_khokhuat = 0xda3,
KEY_Thai_khokhwai = 0xda4,
KEY_Thai_khokhon = 0xda5,
KEY_Thai_khorakhang = 0xda6,
KEY_Thai_ngongu = 0xda7,
KEY_Thai_chochan = 0xda8,
KEY_Thai_choching = 0xda9,
KEY_Thai_chochang = 0xdaa,
KEY_Thai_soso = 0xdab,
KEY_Thai_chochoe = 0xdac,
KEY_Thai_yoying = 0xdad,
KEY_Thai_dochada = 0xdae,
KEY_Thai_topatak = 0xdaf,
KEY_Thai_thothan = 0xdb0,
KEY_Thai_thonangmontho = 0xdb1,
KEY_Thai_thophuthao = 0xdb2,
KEY_Thai_nonen = 0xdb3,
KEY_Thai_dodek = 0xdb4,
KEY_Thai_totao = 0xdb5,
KEY_Thai_thothung = 0xdb6,
KEY_Thai_thothahan = 0xdb7,
KEY_Thai_thothong = 0xdb8,
KEY_Thai_nonu = 0xdb9,
KEY_Thai_bobaimai = 0xdba,
KEY_Thai_popla = 0xdbb,
KEY_Thai_phophung = 0xdbc,
KEY_Thai_fofa = 0xdbd,
KEY_Thai_phophan = 0xdbe,
KEY_Thai_fofan = 0xdbf,
KEY_Thai_phosamphao = 0xdc0,
KEY_Thai_moma = 0xdc1,
KEY_Thai_yoyak = 0xdc2,
KEY_Thai_rorua = 0xdc3,
KEY_Thai_ru = 0xdc4,
KEY_Thai_loling = 0xdc5,
KEY_Thai_lu = 0xdc6,
KEY_Thai_wowaen = 0xdc7,
KEY_Thai_sosala = 0xdc8,
KEY_Thai_sorusi = 0xdc9,
KEY_Thai_sosua = 0xdca,
KEY_Thai_hohip = 0xdcb,
KEY_Thai_lochula = 0xdcc,
KEY_Thai_oang = 0xdcd,
KEY_Thai_honokhuk = 0xdce,
KEY_Thai_paiyannoi = 0xdcf,
KEY_Thai_saraa = 0xdd0,
KEY_Thai_maihanakat = 0xdd1,
KEY_Thai_saraaa = 0xdd2,
KEY_Thai_saraam = 0xdd3,
KEY_Thai_sarai = 0xdd4,
KEY_Thai_saraii = 0xdd5,
KEY_Thai_saraue = 0xdd6,
KEY_Thai_sarauee = 0xdd7,
KEY_Thai_sarau = 0xdd8,
KEY_Thai_sarauu = 0xdd9,
KEY_Thai_phinthu = 0xdda,
KEY_Thai_maihanakat_maitho = 0xdde,
KEY_Thai_baht = 0xddf,
KEY_Thai_sarae = 0xde0,
KEY_Thai_saraae = 0xde1,
KEY_Thai_sarao = 0xde2,
KEY_Thai_saraaimaimuan = 0xde3,
KEY_Thai_saraaimaimalai = 0xde4,
KEY_Thai_lakkhangyao = 0xde5,
KEY_Thai_maiyamok = 0xde6,
KEY_Thai_maitaikhu = 0xde7,
KEY_Thai_maiek = 0xde8,
KEY_Thai_maitho = 0xde9,
KEY_Thai_maitri = 0xdea,
KEY_Thai_maichattawa = 0xdeb,
KEY_Thai_thanthakhat = 0xdec,
KEY_Thai_nikhahit = 0xded,
KEY_Thai_leksun = 0xdf0,
KEY_Thai_leknung = 0xdf1,
KEY_Thai_leksong = 0xdf2,
KEY_Thai_leksam = 0xdf3,
KEY_Thai_leksi = 0xdf4,
KEY_Thai_lekha = 0xdf5,
KEY_Thai_lekhok = 0xdf6,
KEY_Thai_lekchet = 0xdf7,
KEY_Thai_lekpaet = 0xdf8,
KEY_Thai_lekkao = 0xdf9,
KEY_Hangul_Kiyeog = 0xea1,
KEY_Hangul_SsangKiyeog = 0xea2,
KEY_Hangul_KiyeogSios = 0xea3,
KEY_Hangul_Nieun = 0xea4,
KEY_Hangul_NieunJieuj = 0xea5,
KEY_Hangul_NieunHieuh = 0xea6,
KEY_Hangul_Dikeud = 0xea7,
KEY_Hangul_SsangDikeud = 0xea8,
KEY_Hangul_Rieul = 0xea9,
KEY_Hangul_RieulKiyeog = 0xeaa,
KEY_Hangul_RieulMieum = 0xeab,
KEY_Hangul_RieulPieub = 0xeac,
KEY_Hangul_RieulSios = 0xead,
KEY_Hangul_RieulTieut = 0xeae,
KEY_Hangul_RieulPhieuf = 0xeaf,
KEY_Hangul_RieulHieuh = 0xeb0,
KEY_Hangul_Mieum = 0xeb1,
KEY_Hangul_Pieub = 0xeb2,
KEY_Hangul_SsangPieub = 0xeb3,
KEY_Hangul_PieubSios = 0xeb4,
KEY_Hangul_Sios = 0xeb5,
KEY_Hangul_SsangSios = 0xeb6,
KEY_Hangul_Ieung = 0xeb7,
KEY_Hangul_Jieuj = 0xeb8,
KEY_Hangul_SsangJieuj = 0xeb9,
KEY_Hangul_Cieuc = 0xeba,
KEY_Hangul_Khieuq = 0xebb,
KEY_Hangul_Tieut = 0xebc,
KEY_Hangul_Phieuf = 0xebd,
KEY_Hangul_Hieuh = 0xebe,
KEY_Hangul_A = 0xebf,
KEY_Hangul_AE = 0xec0,
KEY_Hangul_YA = 0xec1,
KEY_Hangul_YAE = 0xec2,
KEY_Hangul_EO = 0xec3,
KEY_Hangul_E = 0xec4,
KEY_Hangul_YEO = 0xec5,
KEY_Hangul_YE = 0xec6,
KEY_Hangul_O = 0xec7,
KEY_Hangul_WA = 0xec8,
KEY_Hangul_WAE = 0xec9,
KEY_Hangul_OE = 0xeca,
KEY_Hangul_YO = 0xecb,
KEY_Hangul_U = 0xecc,
KEY_Hangul_WEO = 0xecd,
KEY_Hangul_WE = 0xece,
KEY_Hangul_WI = 0xecf,
KEY_Hangul_YU = 0xed0,
KEY_Hangul_EU = 0xed1,
KEY_Hangul_YI = 0xed2,
KEY_Hangul_I = 0xed3,
KEY_Hangul_J_Kiyeog = 0xed4,
KEY_Hangul_J_SsangKiyeog = 0xed5,
KEY_Hangul_J_KiyeogSios = 0xed6,
KEY_Hangul_J_Nieun = 0xed7,
KEY_Hangul_J_NieunJieuj = 0xed8,
KEY_Hangul_J_NieunHieuh = 0xed9,
KEY_Hangul_J_Dikeud = 0xeda,
KEY_Hangul_J_Rieul = 0xedb,
KEY_Hangul_J_RieulKiyeog = 0xedc,
KEY_Hangul_J_RieulMieum = 0xedd,
KEY_Hangul_J_RieulPieub = 0xede,
KEY_Hangul_J_RieulSios = 0xedf,
KEY_Hangul_J_RieulTieut = 0xee0,
KEY_Hangul_J_RieulPhieuf = 0xee1,
KEY_Hangul_J_RieulHieuh = 0xee2,
KEY_Hangul_J_Mieum = 0xee3,
KEY_Hangul_J_Pieub = 0xee4,
KEY_Hangul_J_PieubSios = 0xee5,
KEY_Hangul_J_Sios = 0xee6,
KEY_Hangul_J_SsangSios = 0xee7,
KEY_Hangul_J_Ieung = 0xee8,
KEY_Hangul_J_Jieuj = 0xee9,
KEY_Hangul_J_Cieuc = 0xeea,
KEY_Hangul_J_Khieuq = 0xeeb,
KEY_Hangul_J_Tieut = 0xeec,
KEY_Hangul_J_Phieuf = 0xeed,
KEY_Hangul_J_Hieuh = 0xeee,
KEY_Hangul_RieulYeorinHieuh = 0xeef,
KEY_Hangul_SunkyeongeumMieum = 0xef0,
KEY_Hangul_SunkyeongeumPieub = 0xef1,
KEY_Hangul_PanSios = 0xef2,
KEY_Hangul_KkogjiDalrinIeung = 0xef3,
KEY_Hangul_SunkyeongeumPhieuf = 0xef4,
KEY_Hangul_YeorinHieuh = 0xef5,
KEY_Hangul_AraeA = 0xef6,
KEY_Hangul_AraeAE = 0xef7,
KEY_Hangul_J_PanSios = 0xef8,
KEY_Hangul_J_KkogjiDalrinIeung = 0xef9,
KEY_Hangul_J_YeorinHieuh = 0xefa,
KEY_Korean_Won = 0xeff,
KEY_Hangul = 0xff31,
KEY_Hangul_Start = 0xff32,
KEY_Hangul_End = 0xff33,
KEY_Hangul_Hanja = 0xff34,
KEY_Hangul_Jamo = 0xff35,
KEY_Hangul_Romaja = 0xff36,
KEY_Hangul_Codeinput = 0xff37,
KEY_Hangul_Jeonja = 0xff38,
KEY_Hangul_Banja = 0xff39,
KEY_Hangul_PreHanja = 0xff3a,
KEY_Hangul_PostHanja = 0xff3b,
KEY_Hangul_SingleCandidate = 0xff3c,
KEY_Hangul_MultipleCandidate = 0xff3d,
KEY_Hangul_PreviousCandidate = 0xff3e,
KEY_Hangul_Special = 0xff3f,
KEY_Greek_IOTAdiaeresis = KEY_Greek_IOTAdieresis,