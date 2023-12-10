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
 *      Interpolation routines for 15 bit truecolor pixels.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro.h"


#ifdef ALLEGRO_COLOR16



/* macro for constructing the blender routines */
#define BLEND(n)                                                             \
									     \
    static unsigned long blend15_##n(unsigned long x, unsigned long y)       \
    {                                                                        \
	unsigned long r1, g1, b1, r2, g2, b2, r, g, b;                       \
									     \
	r1 = (x >> 10) & 0x1F;                                               \
	r2 = (y >> 10) & 0x1F;                                               \
	r = (r1*(n*256/255) + r2*(256-(n*256/255))) / 256;                   \
									     \
	g1 = (x >> 5) & 0x1F;                                                \
	g2 = (y >> 5) & 0x1F;                                                \
	g = (g1*(n*256/255) + g2*(256-(n*256/255))) / 256;                   \
									     \
	b1 = x & 0x1F;                                                       \
	b2 = y & 0x1F;                                                       \
	b = (b1*(n*256/255) + b2*(256-(n*256/255))) / 256;                   \
									     \
	return (r << 10) | (g << 5) | b;                                     \
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
BLENDER_MAP _trans_blender15 = 
{ { 
   blend15_0,   blend15_1,   blend15_2,   blend15_3, 
   blend15_4,   blend15_5,   blend15_6,   blend15_7, 
   blend15_8,   blend15_9,   blend15_10,  blend15_11, 
   blend15_12,  blend15_13,  blend15_14,  blend15_15, 
   blend15_16,  blend15_17,  blend15_18,  blend15_19, 
   blend15_20,  blend15_21,  blend15_22,  blend15_23, 
   blend15_24,  blend15_25,  blend15_26,  blend15_27, 
   blend15_28,  blend15_29,  blend15_30,  blend15_31, 
   blend15_32,  blend15_33,  blend15_34,  blend15_35, 
   blend15_36,  blend15_37,  blend15_38,  blend15_39, 
   blend15_40,  blend15_41,  blend15_42,  blend15_43, 
   blend15_44,  blend15_45,  blend15_46,  blend15_47, 
   blend15_48,  blend15_49,  blend15_50,  blend15_51, 
   blend15_52,  blend15_53,  blend15_54,  blend15_55, 
   blend15_56,  blend15_57,  blend15_58,  blend15_59, 
   blend15_60,  blend15_61,  blend15_62,  blend15_63, 
   blend15_64,  blend15_65,  blend15_66,  blend15_67, 
   blend15_68,  blend15_69,  blend15_70,  blend15_71, 
   blend15_72,  blend15_73,  blend15_74,  blend15_75, 
   blend15_76,  blend15_77,  blend15_78,  blend15_79, 
   blend15_80,  blend15_81,  blend15_82,  blend15_83, 
   blend15_84,  blend15_85,  blend15_86,  blend15_87, 
   blend15_88,  blend15_89,  blend15_90,  blend15_91, 
   blend15_92,  blend15_93,  blend15_94,  blend15_95, 
   blend15_96,  blend15_97,  blend15_98,  blend15_99, 
   blend15_100, blend15_101, blend15_102, blend15_103, 
   blend15_104, blend15_105, blend15_106, blend15_107, 
   blend15_108, blend15_109, blend15_110, blend15_111, 
   blend15_112, blend15_113, blend15_114, blend15_115, 
   blend15_116, blend15_117, blend15_118, blend15_119, 
   blend15_120, blend15_121, blend15_122, blend15_123, 
   blend15_124, blend15_125, blend15_126, blend15_127, 
   blend15_128, blend15_129, blend15_130, blend15_131, 
   blend15_132, blend15_133, blend15_134, blend15_135, 
   blend15_136, blend15_137, blend15_138, blend15_139, 
   blend15_140, blend15_141, blend15_142, blend15_143, 
   blend15_144, blend15_145, blend15_146, blend15_147, 
   blend15_148, blend15_149, blend15_150, blend15_151, 
   blend15_152, blend15_153, blend15_154, blend15_155, 
   blend15_156, blend15_157, blend15_158, blend15_159, 
   blend15_160, blend15_161, blend15_162, blend15_163, 
   blend15_164, blend15_165, blend15_166, blend15_167, 
   blend15_168, blend15_169, blend15_170, blend15_171, 
   blend15_172, blend15_173, blend15_174, blend15_175, 
   blend15_176, blend15_177, blend15_178, blend15_179, 
   blend15_180, blend15_181, blend15_182, blend15_183, 
   blend15_184, blend15_185, blend15_186, blend15_187, 
   blend15_188, blend15_189, blend15_190, blend15_191, 
   blend15_192, blend15_193, blend15_194, blend15_195, 
   blend15_196, blend15_197, blend15_198, blend15_199, 
   blend15_200, blend15_201, blend15_202, blend15_203, 
   blend15_204, blend15_205, blend15_206, blend15_207, 
   blend15_208, blend15_209, blend15_210, blend15_211, 
   blend15_212, blend15_213, blend15_214, blend15_215, 
   blend15_216, blend15_217, blend15_218, blend15_219, 
   blend15_220, blend15_221, blend15_222, blend15_223, 
   blend15_224, blend15_225, blend15_226, blend15_227, 
   blend15_228, blend15_229, blend15_230, blend15_231, 
   blend15_232, blend15_233, blend15_234, blend15_235, 
   blend15_236, blend15_237, blend15_238, blend15_239, 
   blend15_240, blend15_241, blend15_242, blend15_243, 
   blend15_244, blend15_245, blend15_246, blend15_247, 
   blend15_248, blend15_249, blend15_250, blend15_251, 
   blend15_252, blend15_253, blend15_254, blend15_255, 
} };



#endif
