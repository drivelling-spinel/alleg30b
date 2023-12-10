/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves,
 *      1 Salisbury Road,
 *      Market Drayton,
 *      Shropshire,
 *      England, TF9 1AJ.
 *
 *      Interpolation routines for 24 and 32 bit truecolor pixels.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro.h"


#if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)



#define BLEND(n)                                                             \
									     \
    static unsigned long blend24_##n(unsigned long x, unsigned long y)       \
    {                                                                        \
	unsigned long r1, g1, b1, r2, g2, b2, r, g, b;                       \
									     \
	r1 = (x >> 16) & 0xFF;                                               \
	r2 = (y >> 16) & 0xFF;                                               \
	r = (r1*(n*256/255) + r2*(256-(n*256/255))) / 256;                   \
									     \
	g1 = (x >> 8) & 0xFF;                                                \
	g2 = (y >> 8) & 0xFF;                                                \
	g = (g1*(n*256/255) + g2*(256-(n*256/255))) / 256;                   \
									     \
	b1 = x & 0xFF;                                                       \
	b2 = y & 0xFF;                                                       \
	b = (b1*(n*256/255) + b2*(256-(n*256/255))) / 256;                   \
									     \
	return (r << 16) | (g << 8) | b;                                     \
    }



/* declare 256 interpolation functions (one for each alpha) */ 
BLEND(0)   BLEND(1)   BLEND(2)   BLEND(3) 
BLEND(4)   BLEND(5)   BLEND(6)   BLEND(7) 
BLEND(8)   BLEND(9)   BLEND(10)  BLEND(11) 
BLEND(12)  BLEND(13)  BLEND(14)  BLEND(15) 
BLEND(16)  BLEND(17)  BLEND(18)  BLEND(19) 
BLEND(20)  BLEND(21)  BLEND(22)  BLEND(23) 
BLEND(24)  BLEND(25)  BLEND(26)  BLEND(27) 
BLEND(28)  BLEND(29)  BLEND(30)  BLEND(31) 
BLEND(32)  BLEND(33)  BLEND(34)  BLEND(35) 
BLEND(36)  BLEND(37)  BLEND(38)  BLEND(39) 
BLEND(40)  BLEND(41)  BLEND(42)  BLEND(43) 
BLEND(44)  BLEND(45)  BLEND(46)  BLEND(47) 
BLEND(48)  BLEND(49)  BLEND(50)  BLEND(51) 
BLEND(52)  BLEND(53)  BLEND(54)  BLEND(55) 
BLEND(56)  BLEND(57)  BLEND(58)  BLEND(59) 
BLEND(60)  BLEND(61)  BLEND(62)  BLEND(63) 
BLEND(64)  BLEND(65)  BLEND(66)  BLEND(67) 
BLEND(68)  BLEND(69)  BLEND(70)  BLEND(71) 
BLEND(72)  BLEND(73)  BLEND(74)  BLEND(75) 
BLEND(76)  BLEND(77)  BLEND(78)  BLEND(79) 
BLEND(80)  BLEND(81)  BLEND(82)  BLEND(83) 
BLEND(84)  BLEND(85)  BLEND(86)  BLEND(87) 
BLEND(88)  BLEND(89)  BLEND(90)  BLEND(91) 
BLEND(92)  BLEND(93)  BLEND(94)  BLEND(95) 
BLEND(96)  BLEND(97)  BLEND(98)  BLEND(99) 
BLEND(100) BLEND(101) BLEND(102) BLEND(103) 
BLEND(104) BLEND(105) BLEND(106) BLEND(107) 
BLEND(108) BLEND(109) BLEND(110) BLEND(111) 
BLEND(112) BLEND(113) BLEND(114) BLEND(115) 
BLEND(116) BLEND(117) BLEND(118) BLEND(119) 
BLEND(120) BLEND(121) BLEND(122) BLEND(123) 
BLEND(124) BLEND(125) BLEND(126) BLEND(127) 
BLEND(128) BLEND(129) BLEND(130) BLEND(131) 
BLEND(132) BLEND(133) BLEND(134) BLEND(135) 
BLEND(136) BLEND(137) BLEND(138) BLEND(139) 
BLEND(140) BLEND(141) BLEND(142) BLEND(143) 
BLEND(144) BLEND(145) BLEND(146) BLEND(147) 
BLEND(148) BLEND(149) BLEND(150) BLEND(151) 
BLEND(152) BLEND(153) BLEND(154) BLEND(155) 
BLEND(156) BLEND(157) BLEND(158) BLEND(159) 
BLEND(160) BLEND(161) BLEND(162) BLEND(163) 
BLEND(164) BLEND(165) BLEND(166) BLEND(167) 
BLEND(168) BLEND(169) BLEND(170) BLEND(171) 
BLEND(172) BLEND(173) BLEND(174) BLEND(175) 
BLEND(176) BLEND(177) BLEND(178) BLEND(179) 
BLEND(180) BLEND(181) BLEND(182) BLEND(183) 
BLEND(184) BLEND(185) BLEND(186) BLEND(187) 
BLEND(188) BLEND(189) BLEND(190) BLEND(191) 
BLEND(192) BLEND(193) BLEND(194) BLEND(195) 
BLEND(196) BLEND(197) BLEND(198) BLEND(199) 
BLEND(200) BLEND(201) BLEND(202) BLEND(203)
BLEND(204) BLEND(205) BLEND(206) BLEND(207) 
BLEND(208) BLEND(209) BLEND(210) BLEND(211) 
BLEND(212) BLEND(213) BLEND(214) BLEND(215) 
BLEND(216) BLEND(217) BLEND(218) BLEND(219) 
BLEND(220) BLEND(221) BLEND(222) BLEND(223) 
BLEND(224) BLEND(225) BLEND(226) BLEND(227) 
BLEND(228) BLEND(229) BLEND(230) BLEND(231) 
BLEND(232) BLEND(233) BLEND(234) BLEND(235) 
BLEND(236) BLEND(237) BLEND(238) BLEND(239) 
BLEND(240) BLEND(241) BLEND(242) BLEND(243) 
BLEND(244) BLEND(245) BLEND(246) BLEND(247) 
BLEND(248) BLEND(249) BLEND(250) BLEND(251) 
BLEND(252) BLEND(253) BLEND(254) BLEND(255) 



/* and list them all in a table */ 
BLENDER_MAP _trans_blender24 = 
{ { 
   blend24_0,   blend24_1,   blend24_2,   blend24_3, 
   blend24_4,   blend24_5,   blend24_6,   blend24_7, 
   blend24_8,   blend24_9,   blend24_10,  blend24_11, 
   blend24_12,  blend24_13,  blend24_14,  blend24_15, 
   blend24_16,  blend24_17,  blend24_18,  blend24_19, 
   blend24_20,  blend24_21,  blend24_22,  blend24_23, 
   blend24_24,  blend24_25,  blend24_26,  blend24_27, 
   blend24_28,  blend24_29,  blend24_30,  blend24_31, 
   blend24_32,  blend24_33,  blend24_34,  blend24_35, 
   blend24_36,  blend24_37,  blend24_38,  blend24_39, 
   blend24_40,  blend24_41,  blend24_42,  blend24_43, 
   blend24_44,  blend24_45,  blend24_46,  blend24_47, 
   blend24_48,  blend24_49,  blend24_50,  blend24_51, 
   blend24_52,  blend24_53,  blend24_54,  blend24_55, 
   blend24_56,  blend24_57,  blend24_58,  blend24_59, 
   blend24_60,  blend24_61,  blend24_62,  blend24_63, 
   blend24_64,  blend24_65,  blend24_66,  blend24_67, 
   blend24_68,  blend24_69,  blend24_70,  blend24_71, 
   blend24_72,  blend24_73,  blend24_74,  blend24_75, 
   blend24_76,  blend24_77,  blend24_78,  blend24_79, 
   blend24_80,  blend24_81,  blend24_82,  blend24_83, 
   blend24_84,  blend24_85,  blend24_86,  blend24_87, 
   blend24_88,  blend24_89,  blend24_90,  blend24_91, 
   blend24_92,  blend24_93,  blend24_94,  blend24_95, 
   blend24_96,  blend24_97,  blend24_98,  blend24_99, 
   blend24_100, blend24_101, blend24_102, blend24_103, 
   blend24_104, blend24_105, blend24_106, blend24_107, 
   blend24_108, blend24_109, blend24_110, blend24_111, 
   blend24_112, blend24_113, blend24_114, blend24_115, 
   blend24_116, blend24_117, blend24_118, blend24_119, 
   blend24_120, blend24_121, blend24_122, blend24_123, 
   blend24_124, blend24_125, blend24_126, blend24_127, 
   blend24_128, blend24_129, blend24_130, blend24_131, 
   blend24_132, blend24_133, blend24_134, blend24_135, 
   blend24_136, blend24_137, blend24_138, blend24_139, 
   blend24_140, blend24_141, blend24_142, blend24_143, 
   blend24_144, blend24_145, blend24_146, blend24_147, 
   blend24_148, blend24_149, blend24_150, blend24_151, 
   blend24_152, blend24_153, blend24_154, blend24_155, 
   blend24_156, blend24_157, blend24_158, blend24_159, 
   blend24_160, blend24_161, blend24_162, blend24_163, 
   blend24_164, blend24_165, blend24_166, blend24_167, 
   blend24_168, blend24_169, blend24_170, blend24_171, 
   blend24_172, blend24_173, blend24_174, blend24_175, 
   blend24_176, blend24_177, blend24_178, blend24_179, 
   blend24_180, blend24_181, blend24_182, blend24_183, 
   blend24_184, blend24_185, blend24_186, blend24_187, 
   blend24_188, blend24_189, blend24_190, blend24_191, 
   blend24_192, blend24_193, blend24_194, blend24_195, 
   blend24_196, blend24_197, blend24_198, blend24_199, 
   blend24_200, blend24_201, blend24_202, blend24_203, 
   blend24_204, blend24_205, blend24_206, blend24_207, 
   blend24_208, blend24_209, blend24_210, blend24_211, 
   blend24_212, blend24_213, blend24_214, blend24_215, 
   blend24_216, blend24_217, blend24_218, blend24_219, 
   blend24_220, blend24_221, blend24_222, blend24_223, 
   blend24_224, blend24_225, blend24_226, blend24_227, 
   blend24_228, blend24_229, blend24_230, blend24_231, 
   blend24_232, blend24_233, blend24_234, blend24_235, 
   blend24_236, blend24_237, blend24_238, blend24_239, 
   blend24_240, blend24_241, blend24_242, blend24_243, 
   blend24_244, blend24_245, blend24_246, blend24_247, 
   blend24_248, blend24_249, blend24_250, blend24_251, 
   blend24_252, blend24_253, blend24_254, blend24_255, 
} };



/* use these routines */
#define TB24   &_trans_blender24

#else

/* no 24/32 bit code */
#define TB24   NULL

#endif




#ifdef ALLEGRO_COLOR16

/* use 15/16 bit blenders from other files */
extern BLENDER_MAP _trans_blender15;
extern BLENDER_MAP _trans_blender16;

#define TB15   &_trans_blender15
#define TB16   &_trans_blender16

#else

/* no 15/16 bit code */
#define TB15   NULL
#define TB16   NULL

#endif



/* set_trans_blender:
 *  Selects a translucent pixel blending mode for truecolor pixels, using
 *  the specified light color (when drawing lit sprites), or alpha (when
 *  using a translucent drawing mode).
 */
void set_trans_blender(int r, int g, int b, int a)
{
   set_blender_mode(TB15, TB16, TB24, r, g, b, a);
}

