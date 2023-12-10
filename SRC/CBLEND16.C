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
 *      Interpolation routines for 16 bit truecolor pixels.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro.h"


#ifdef ALLEGRO_COLOR16



/* macro for constructing the blender routines */
#define BLEND(n)                                                             \
									     \
    static unsigned long blend16_##n(unsigned long x, unsigned long y)       \
    {                                                                        \
	unsigned long r1, g1, b1, r2, g2, b2, r, g, b;                       \
									     \
	r1 = (x >> 11) & 0x1F;                                               \
	r2 = (y >> 11) & 0x1F;                                               \
	r = (r1*(n*256/255) + r2*(256-(n*256/255))) / 256;                   \
									     \
	g1 = (x >> 5) & 0x3F;                                                \
	g2 = (y >> 5) & 0x3F;                                                \
	g = (g1*(n*256/255) + g2*(256-(n*256/255))) / 256;                   \
									     \
	b1 = x & 0x1F;                                                       \
	b2 = y & 0x1F;                                                       \
	b = (b1*(n*256/255) + b2*(256-(n*256/255))) / 256;                   \
									     \
	return (r << 11) | (g << 5) | b;                                     \
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
BLENDER_MAP _trans_blender16 = 
{ { 
   blend16_0,   blend16_1,   blend16_2,   blend16_3, 
   blend16_4,   blend16_5,   blend16_6,   blend16_7, 
   blend16_8,   blend16_9,   blend16_10,  blend16_11, 
   blend16_12,  blend16_13,  blend16_14,  blend16_15, 
   blend16_16,  blend16_17,  blend16_18,  blend16_19, 
   blend16_20,  blend16_21,  blend16_22,  blend16_23, 
   blend16_24,  blend16_25,  blend16_26,  blend16_27, 
   blend16_28,  blend16_29,  blend16_30,  blend16_31, 
   blend16_32,  blend16_33,  blend16_34,  blend16_35, 
   blend16_36,  blend16_37,  blend16_38,  blend16_39, 
   blend16_40,  blend16_41,  blend16_42,  blend16_43, 
   blend16_44,  blend16_45,  blend16_46,  blend16_47, 
   blend16_48,  blend16_49,  blend16_50,  blend16_51, 
   blend16_52,  blend16_53,  blend16_54,  blend16_55, 
   blend16_56,  blend16_57,  blend16_58,  blend16_59, 
   blend16_60,  blend16_61,  blend16_62,  blend16_63, 
   blend16_64,  blend16_65,  blend16_66,  blend16_67, 
   blend16_68,  blend16_69,  blend16_70,  blend16_71, 
   blend16_72,  blend16_73,  blend16_74,  blend16_75, 
   blend16_76,  blend16_77,  blend16_78,  blend16_79, 
   blend16_80,  blend16_81,  blend16_82,  blend16_83, 
   blend16_84,  blend16_85,  blend16_86,  blend16_87, 
   blend16_88,  blend16_89,  blend16_90,  blend16_91, 
   blend16_92,  blend16_93,  blend16_94,  blend16_95, 
   blend16_96,  blend16_97,  blend16_98,  blend16_99, 
   blend16_100, blend16_101, blend16_102, blend16_103, 
   blend16_104, blend16_105, blend16_106, blend16_107, 
   blend16_108, blend16_109, blend16_110, blend16_111, 
   blend16_112, blend16_113, blend16_114, blend16_115, 
   blend16_116, blend16_117, blend16_118, blend16_119, 
   blend16_120, blend16_121, blend16_122, blend16_123, 
   blend16_124, blend16_125, blend16_126, blend16_127, 
   blend16_128, blend16_129, blend16_130, blend16_131, 
   blend16_132, blend16_133, blend16_134, blend16_135, 
   blend16_136, blend16_137, blend16_138, blend16_139, 
   blend16_140, blend16_141, blend16_142, blend16_143, 
   blend16_144, blend16_145, blend16_146, blend16_147, 
   blend16_148, blend16_149, blend16_150, blend16_151, 
   blend16_152, blend16_153, blend16_154, blend16_155, 
   blend16_156, blend16_157, blend16_158, blend16_159, 
   blend16_160, blend16_161, blend16_162, blend16_163, 
   blend16_164, blend16_165, blend16_166, blend16_167, 
   blend16_168, blend16_169, blend16_170, blend16_171, 
   blend16_172, blend16_173, blend16_174, blend16_175, 
   blend16_176, blend16_177, blend16_178, blend16_179, 
   blend16_180, blend16_181, blend16_182, blend16_183, 
   blend16_184, blend16_185, blend16_186, blend16_187, 
   blend16_188, blend16_189, blend16_190, blend16_191, 
   blend16_192, blend16_193, blend16_194, blend16_195, 
   blend16_196, blend16_197, blend16_198, blend16_199, 
   blend16_200, blend16_201, blend16_202, blend16_203, 
   blend16_204, blend16_205, blend16_206, blend16_207, 
   blend16_208, blend16_209, blend16_210, blend16_211, 
   blend16_212, blend16_213, blend16_214, blend16_215, 
   blend16_216, blend16_217, blend16_218, blend16_219, 
   blend16_220, blend16_221, blend16_222, blend16_223, 
   blend16_224, blend16_225, blend16_226, blend16_227, 
   blend16_228, blend16_229, blend16_230, blend16_231, 
   blend16_232, blend16_233, blend16_234, blend16_235, 
   blend16_236, blend16_237, blend16_238, blend16_239, 
   blend16_240, blend16_241, blend16_242, blend16_243, 
   blend16_244, blend16_245, blend16_246, blend16_247, 
   blend16_248, blend16_249, blend16_250, blend16_251, 
   blend16_252, blend16_253, blend16_254, blend16_255, 
} };



#endif
