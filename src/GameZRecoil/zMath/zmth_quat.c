/* Quaternion operations. The filename is an inferred reconstruction name. */
#include "GameZRecoil/zMath/zmth.h"

/**
 * Placeholders for debug-build declarations absent from the recovered shipping
 * source. They are never referenced and emit no code, data or symbols; they keep
 * VC5's C1 ID counter where the original declarations left it.
 */
extern int g_RemovedDebug_ZmthQuat_0;
extern int g_RemovedDebug_ZmthQuat_1;
extern int g_RemovedDebug_ZmthQuat_2;
extern int g_RemovedDebug_ZmthQuat_3;
extern int g_RemovedDebug_ZmthQuat_4;
extern int g_RemovedDebug_ZmthQuat_5;
extern int g_RemovedDebug_ZmthQuat_6;
extern int g_RemovedDebug_ZmthQuat_7;
extern int g_RemovedDebug_ZmthQuat_8;
extern int g_RemovedDebug_ZmthQuat_9;
extern int g_RemovedDebug_ZmthQuat_10;
extern int g_RemovedDebug_ZmthQuat_11;
extern int g_RemovedDebug_ZmthQuat_12;
extern int g_RemovedDebug_ZmthQuat_13;
extern int g_RemovedDebug_ZmthQuat_14;
extern int g_RemovedDebug_ZmthQuat_15;
extern int g_RemovedDebug_ZmthQuat_16;
extern int g_RemovedDebug_ZmthQuat_17;
extern int g_RemovedDebug_ZmthQuat_18;
extern int g_RemovedDebug_ZmthQuat_19;
extern int g_RemovedDebug_ZmthQuat_20;
extern int g_RemovedDebug_ZmthQuat_21;
extern int g_RemovedDebug_ZmthQuat_22;
extern int g_RemovedDebug_ZmthQuat_23;
extern int g_RemovedDebug_ZmthQuat_24;
extern int g_RemovedDebug_ZmthQuat_25;
extern int g_RemovedDebug_ZmthQuat_26;
extern int g_RemovedDebug_ZmthQuat_27;
extern int g_RemovedDebug_ZmthQuat_28;
extern int g_RemovedDebug_ZmthQuat_29;
extern int g_RemovedDebug_ZmthQuat_30;
extern int g_RemovedDebug_ZmthQuat_31;
extern int g_RemovedDebug_ZmthQuat_32;
extern int g_RemovedDebug_ZmthQuat_33;
extern int g_RemovedDebug_ZmthQuat_34;
extern int g_RemovedDebug_ZmthQuat_35;
extern int g_RemovedDebug_ZmthQuat_36;
extern int g_RemovedDebug_ZmthQuat_37;
extern int g_RemovedDebug_ZmthQuat_38;
extern int g_RemovedDebug_ZmthQuat_39;
extern int g_RemovedDebug_ZmthQuat_40;
extern int g_RemovedDebug_ZmthQuat_41;
extern int g_RemovedDebug_ZmthQuat_42;
extern int g_RemovedDebug_ZmthQuat_43;
extern int g_RemovedDebug_ZmthQuat_44;
extern int g_RemovedDebug_ZmthQuat_45;
extern int g_RemovedDebug_ZmthQuat_46;
extern int g_RemovedDebug_ZmthQuat_47;
extern int g_RemovedDebug_ZmthQuat_48;
extern int g_RemovedDebug_ZmthQuat_49;
extern int g_RemovedDebug_ZmthQuat_50;
extern int g_RemovedDebug_ZmthQuat_51;
extern int g_RemovedDebug_ZmthQuat_52;
extern int g_RemovedDebug_ZmthQuat_53;
extern int g_RemovedDebug_ZmthQuat_54;
extern int g_RemovedDebug_ZmthQuat_55;
extern int g_RemovedDebug_ZmthQuat_56;
extern int g_RemovedDebug_ZmthQuat_57;
extern int g_RemovedDebug_ZmthQuat_58;
extern int g_RemovedDebug_ZmthQuat_59;
extern int g_RemovedDebug_ZmthQuat_60;
extern int g_RemovedDebug_ZmthQuat_61;
extern int g_RemovedDebug_ZmthQuat_62;
extern int g_RemovedDebug_ZmthQuat_63;
extern int g_RemovedDebug_ZmthQuat_64;
extern int g_RemovedDebug_ZmthQuat_65;
extern int g_RemovedDebug_ZmthQuat_66;
extern int g_RemovedDebug_ZmthQuat_67;
extern int g_RemovedDebug_ZmthQuat_68;
extern int g_RemovedDebug_ZmthQuat_69;
extern int g_RemovedDebug_ZmthQuat_70;
extern int g_RemovedDebug_ZmthQuat_71;
extern int g_RemovedDebug_ZmthQuat_72;
extern int g_RemovedDebug_ZmthQuat_73;
extern int g_RemovedDebug_ZmthQuat_74;
extern int g_RemovedDebug_ZmthQuat_75;
extern int g_RemovedDebug_ZmthQuat_76;
extern int g_RemovedDebug_ZmthQuat_77;
extern int g_RemovedDebug_ZmthQuat_78;
extern int g_RemovedDebug_ZmthQuat_79;
extern int g_RemovedDebug_ZmthQuat_80;
extern int g_RemovedDebug_ZmthQuat_81;
extern int g_RemovedDebug_ZmthQuat_82;
extern int g_RemovedDebug_ZmthQuat_83;
extern int g_RemovedDebug_ZmthQuat_84;
extern int g_RemovedDebug_ZmthQuat_85;
extern int g_RemovedDebug_ZmthQuat_86;
extern int g_RemovedDebug_ZmthQuat_87;
extern int g_RemovedDebug_ZmthQuat_88;
extern int g_RemovedDebug_ZmthQuat_89;
extern int g_RemovedDebug_ZmthQuat_90;
extern int g_RemovedDebug_ZmthQuat_91;
extern int g_RemovedDebug_ZmthQuat_92;
extern int g_RemovedDebug_ZmthQuat_93;
extern int g_RemovedDebug_ZmthQuat_94;
extern int g_RemovedDebug_ZmthQuat_95;
extern int g_RemovedDebug_ZmthQuat_96;
extern int g_RemovedDebug_ZmthQuat_97;
extern int g_RemovedDebug_ZmthQuat_98;
extern int g_RemovedDebug_ZmthQuat_99;
extern int g_RemovedDebug_ZmthQuat_100;
extern int g_RemovedDebug_ZmthQuat_101;
extern int g_RemovedDebug_ZmthQuat_102;
extern int g_RemovedDebug_ZmthQuat_103;
extern int g_RemovedDebug_ZmthQuat_104;
extern int g_RemovedDebug_ZmthQuat_105;
extern int g_RemovedDebug_ZmthQuat_106;
extern int g_RemovedDebug_ZmthQuat_107;
extern int g_RemovedDebug_ZmthQuat_108;
extern int g_RemovedDebug_ZmthQuat_109;
extern int g_RemovedDebug_ZmthQuat_110;
extern int g_RemovedDebug_ZmthQuat_111;
extern int g_RemovedDebug_ZmthQuat_112;
extern int g_RemovedDebug_ZmthQuat_113;
extern int g_RemovedDebug_ZmthQuat_114;
extern int g_RemovedDebug_ZmthQuat_115;
extern int g_RemovedDebug_ZmthQuat_116;
extern int g_RemovedDebug_ZmthQuat_117;
extern int g_RemovedDebug_ZmthQuat_118;
extern int g_RemovedDebug_ZmthQuat_119;
extern int g_RemovedDebug_ZmthQuat_120;
extern int g_RemovedDebug_ZmthQuat_121;
extern int g_RemovedDebug_ZmthQuat_122;
extern int g_RemovedDebug_ZmthQuat_123;
extern int g_RemovedDebug_ZmthQuat_124;
extern int g_RemovedDebug_ZmthQuat_125;
extern int g_RemovedDebug_ZmthQuat_126;
extern int g_RemovedDebug_ZmthQuat_127;
extern int g_RemovedDebug_ZmthQuat_128;
extern int g_RemovedDebug_ZmthQuat_129;
extern int g_RemovedDebug_ZmthQuat_130;
extern int g_RemovedDebug_ZmthQuat_131;
extern int g_RemovedDebug_ZmthQuat_132;
extern int g_RemovedDebug_ZmthQuat_133;
extern int g_RemovedDebug_ZmthQuat_134;
extern int g_RemovedDebug_ZmthQuat_135;
extern int g_RemovedDebug_ZmthQuat_136;
extern int g_RemovedDebug_ZmthQuat_137;
extern int g_RemovedDebug_ZmthQuat_138;
extern int g_RemovedDebug_ZmthQuat_139;
extern int g_RemovedDebug_ZmthQuat_140;
extern int g_RemovedDebug_ZmthQuat_141;
extern int g_RemovedDebug_ZmthQuat_142;
extern int g_RemovedDebug_ZmthQuat_143;
extern int g_RemovedDebug_ZmthQuat_144;
extern int g_RemovedDebug_ZmthQuat_145;
extern int g_RemovedDebug_ZmthQuat_146;
extern int g_RemovedDebug_ZmthQuat_147;
extern int g_RemovedDebug_ZmthQuat_148;
extern int g_RemovedDebug_ZmthQuat_149;
extern int g_RemovedDebug_ZmthQuat_150;
extern int g_RemovedDebug_ZmthQuat_151;
extern int g_RemovedDebug_ZmthQuat_152;
extern int g_RemovedDebug_ZmthQuat_153;
extern int g_RemovedDebug_ZmthQuat_154;
extern int g_RemovedDebug_ZmthQuat_155;
extern int g_RemovedDebug_ZmthQuat_156;
extern int g_RemovedDebug_ZmthQuat_157;
extern int g_RemovedDebug_ZmthQuat_158;
extern int g_RemovedDebug_ZmthQuat_159;
extern int g_RemovedDebug_ZmthQuat_160;
extern int g_RemovedDebug_ZmthQuat_161;
extern int g_RemovedDebug_ZmthQuat_162;
extern int g_RemovedDebug_ZmthQuat_163;
extern int g_RemovedDebug_ZmthQuat_164;
extern int g_RemovedDebug_ZmthQuat_165;
extern int g_RemovedDebug_ZmthQuat_166;
extern int g_RemovedDebug_ZmthQuat_167;
extern int g_RemovedDebug_ZmthQuat_168;
extern int g_RemovedDebug_ZmthQuat_169;
extern int g_RemovedDebug_ZmthQuat_170;
extern int g_RemovedDebug_ZmthQuat_171;
extern int g_RemovedDebug_ZmthQuat_172;
extern int g_RemovedDebug_ZmthQuat_173;
extern int g_RemovedDebug_ZmthQuat_174;
extern int g_RemovedDebug_ZmthQuat_175;
extern int g_RemovedDebug_ZmthQuat_176;
extern int g_RemovedDebug_ZmthQuat_177;
extern int g_RemovedDebug_ZmthQuat_178;
extern int g_RemovedDebug_ZmthQuat_179;
extern int g_RemovedDebug_ZmthQuat_180;
extern int g_RemovedDebug_ZmthQuat_181;
extern int g_RemovedDebug_ZmthQuat_182;
extern int g_RemovedDebug_ZmthQuat_183;
extern int g_RemovedDebug_ZmthQuat_184;
extern int g_RemovedDebug_ZmthQuat_185;
extern int g_RemovedDebug_ZmthQuat_186;
extern int g_RemovedDebug_ZmthQuat_187;
extern int g_RemovedDebug_ZmthQuat_188;
extern int g_RemovedDebug_ZmthQuat_189;
extern int g_RemovedDebug_ZmthQuat_190;
extern int g_RemovedDebug_ZmthQuat_191;
extern int g_RemovedDebug_ZmthQuat_192;
extern int g_RemovedDebug_ZmthQuat_193;
extern int g_RemovedDebug_ZmthQuat_194;
extern int g_RemovedDebug_ZmthQuat_195;
extern int g_RemovedDebug_ZmthQuat_196;
extern int g_RemovedDebug_ZmthQuat_197;
extern int g_RemovedDebug_ZmthQuat_198;
extern int g_RemovedDebug_ZmthQuat_199;
extern int g_RemovedDebug_ZmthQuat_200;
extern int g_RemovedDebug_ZmthQuat_201;
extern int g_RemovedDebug_ZmthQuat_202;
extern int g_RemovedDebug_ZmthQuat_203;
extern int g_RemovedDebug_ZmthQuat_204;
extern int g_RemovedDebug_ZmthQuat_205;
extern int g_RemovedDebug_ZmthQuat_206;
extern int g_RemovedDebug_ZmthQuat_207;
extern int g_RemovedDebug_ZmthQuat_208;
extern int g_RemovedDebug_ZmthQuat_209;
extern int g_RemovedDebug_ZmthQuat_210;
extern int g_RemovedDebug_ZmthQuat_211;
extern int g_RemovedDebug_ZmthQuat_212;
extern int g_RemovedDebug_ZmthQuat_213;
extern int g_RemovedDebug_ZmthQuat_214;
extern int g_RemovedDebug_ZmthQuat_215;
extern int g_RemovedDebug_ZmthQuat_216;
extern int g_RemovedDebug_ZmthQuat_217;
extern int g_RemovedDebug_ZmthQuat_218;
extern int g_RemovedDebug_ZmthQuat_219;
extern int g_RemovedDebug_ZmthQuat_220;
extern int g_RemovedDebug_ZmthQuat_221;
extern int g_RemovedDebug_ZmthQuat_222;
extern int g_RemovedDebug_ZmthQuat_223;
extern int g_RemovedDebug_ZmthQuat_224;
extern int g_RemovedDebug_ZmthQuat_225;
extern int g_RemovedDebug_ZmthQuat_226;
extern int g_RemovedDebug_ZmthQuat_227;
extern int g_RemovedDebug_ZmthQuat_228;
extern int g_RemovedDebug_ZmthQuat_229;
extern int g_RemovedDebug_ZmthQuat_230;
extern int g_RemovedDebug_ZmthQuat_231;
extern int g_RemovedDebug_ZmthQuat_232;
extern int g_RemovedDebug_ZmthQuat_233;
extern int g_RemovedDebug_ZmthQuat_234;
extern int g_RemovedDebug_ZmthQuat_235;
extern int g_RemovedDebug_ZmthQuat_236;
extern int g_RemovedDebug_ZmthQuat_237;
extern int g_RemovedDebug_ZmthQuat_238;
extern int g_RemovedDebug_ZmthQuat_239;
extern int g_RemovedDebug_ZmthQuat_240;
extern int g_RemovedDebug_ZmthQuat_241;
extern int g_RemovedDebug_ZmthQuat_242;
extern int g_RemovedDebug_ZmthQuat_243;
extern int g_RemovedDebug_ZmthQuat_244;
extern int g_RemovedDebug_ZmthQuat_245;
extern int g_RemovedDebug_ZmthQuat_246;
extern int g_RemovedDebug_ZmthQuat_247;
extern int g_RemovedDebug_ZmthQuat_248;
extern int g_RemovedDebug_ZmthQuat_249;
extern int g_RemovedDebug_ZmthQuat_250;
extern int g_RemovedDebug_ZmthQuat_251;
extern int g_RemovedDebug_ZmthQuat_252;
extern int g_RemovedDebug_ZmthQuat_253;
extern int g_RemovedDebug_ZmthQuat_254;
extern int g_RemovedDebug_ZmthQuat_255;
extern int g_RemovedDebug_ZmthQuat_256;
extern int g_RemovedDebug_ZmthQuat_257;
extern int g_RemovedDebug_ZmthQuat_258;
extern int g_RemovedDebug_ZmthQuat_259;
extern int g_RemovedDebug_ZmthQuat_260;
extern int g_RemovedDebug_ZmthQuat_261;
extern int g_RemovedDebug_ZmthQuat_262;
extern int g_RemovedDebug_ZmthQuat_263;
extern int g_RemovedDebug_ZmthQuat_264;
extern int g_RemovedDebug_ZmthQuat_265;
extern int g_RemovedDebug_ZmthQuat_266;
extern int g_RemovedDebug_ZmthQuat_267;
extern int g_RemovedDebug_ZmthQuat_268;
extern int g_RemovedDebug_ZmthQuat_269;
extern int g_RemovedDebug_ZmthQuat_270;
extern int g_RemovedDebug_ZmthQuat_271;
extern int g_RemovedDebug_ZmthQuat_272;
extern int g_RemovedDebug_ZmthQuat_273;
extern int g_RemovedDebug_ZmthQuat_274;
extern int g_RemovedDebug_ZmthQuat_275;
extern int g_RemovedDebug_ZmthQuat_276;
extern int g_RemovedDebug_ZmthQuat_277;
extern int g_RemovedDebug_ZmthQuat_278;
extern int g_RemovedDebug_ZmthQuat_279;
extern int g_RemovedDebug_ZmthQuat_280;
extern int g_RemovedDebug_ZmthQuat_281;
extern int g_RemovedDebug_ZmthQuat_282;
extern int g_RemovedDebug_ZmthQuat_283;
extern int g_RemovedDebug_ZmthQuat_284;
extern int g_RemovedDebug_ZmthQuat_285;
extern int g_RemovedDebug_ZmthQuat_286;
extern int g_RemovedDebug_ZmthQuat_287;
extern int g_RemovedDebug_ZmthQuat_288;
extern int g_RemovedDebug_ZmthQuat_289;
extern int g_RemovedDebug_ZmthQuat_290;
extern int g_RemovedDebug_ZmthQuat_291;
extern int g_RemovedDebug_ZmthQuat_292;
extern int g_RemovedDebug_ZmthQuat_293;
extern int g_RemovedDebug_ZmthQuat_294;
extern int g_RemovedDebug_ZmthQuat_295;
extern int g_RemovedDebug_ZmthQuat_296;
extern int g_RemovedDebug_ZmthQuat_297;
extern int g_RemovedDebug_ZmthQuat_298;
extern int g_RemovedDebug_ZmthQuat_299;
extern int g_RemovedDebug_ZmthQuat_300;
extern int g_RemovedDebug_ZmthQuat_301;
extern int g_RemovedDebug_ZmthQuat_302;
extern int g_RemovedDebug_ZmthQuat_303;
extern int g_RemovedDebug_ZmthQuat_304;
extern int g_RemovedDebug_ZmthQuat_305;
extern int g_RemovedDebug_ZmthQuat_306;
extern int g_RemovedDebug_ZmthQuat_307;
extern int g_RemovedDebug_ZmthQuat_308;
extern int g_RemovedDebug_ZmthQuat_309;
extern int g_RemovedDebug_ZmthQuat_310;
extern int g_RemovedDebug_ZmthQuat_311;
extern int g_RemovedDebug_ZmthQuat_312;
extern int g_RemovedDebug_ZmthQuat_313;
extern int g_RemovedDebug_ZmthQuat_314;
extern int g_RemovedDebug_ZmthQuat_315;
extern int g_RemovedDebug_ZmthQuat_316;
extern int g_RemovedDebug_ZmthQuat_317;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromeuler
 * @recoil-artifact defines .text recoil:function:0x4757c0: zMathQuatFromEulerYXZ
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Purpose: converts three Euler rotation angles into a quaternion using Y-X-Z composition.
 * Data: reads no authored zMath globals; VC5 materializes literal and x87
 * range-check constants while lowering the sin/cos half-angle calls.
 */
void __fastcall zMathQuatFromEulerYXZ(zQuat* outQuat, float angle0, float angle1, float angle2)
{
    float sy;
    float cy;
    float sp;
    float cp;
    float sr;
    float cr;
    float cpcy;
    float spcy;
    float cpsy;
    float spsy;
    SinCos(angle0 * 0.5f, &sy, &cy);
    SinCos(angle1 * 0.5f, &sp, &cp);

    cpcy = cp * cy;
    spcy = sp * cy;
    cpsy = cp * sy;
    spsy = sp * sy;

    SinCos(angle2 * 0.5f, &sr, &cr);
    outQuat->w = spsy * sr + cpcy * cr;
    outQuat->x = cpsy * sr + spcy * cr;
    outQuat->y = cpsy * cr - spcy * sr;
    outQuat->z = cpcy * sr - spsy * cr;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiply
 * @recoil-artifact defines .text recoil:function:0x475910: zMathQuatMultiply
 * @recoil-match byte
 *
 * Purpose: computes the quaternion product used by zMath rotation composition.
 */
void __fastcall zMathQuatMultiply(const zQuat* quatA, const zQuat* quatB, zQuat* outAB)
{
    float scaledX;
    float scaledY;
    float scaledZ;
    outAB->w = quatB->w * quatA->w - quatA->x * quatB->x - quatA->y * quatB->y - quatA->z * quatB->z;
    scaledX = quatB->w * quatA->x;
    outAB->x = scaledX + quatA->w * quatB->x + quatB->z * quatA->y - quatA->z * quatB->y;
    scaledY = quatB->w * quatA->y;
    outAB->y = scaledY + quatA->w * quatB->y + quatA->z * quatB->x - quatB->z * quatA->x;
    scaledZ = quatB->w * quatA->z;
    outAB->z = scaledZ + quatA->w * quatB->z + quatB->y * quatA->x - quatA->y * quatB->x;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiplyinverse
 * @recoil-artifact defines .text recoil:function:0x4759d0: zMathQuatMultiplyConjugate
 * @recoil-match byte
 *
 * Purpose: multiplies a quaternion by the conjugate of a second quaternion (its inverse only for unit quaternions).
 */
void __fastcall zMathQuatMultiplyConjugate(const zQuat* quatA, const zQuat* quatB, zQuat* outAConjB)
{
    outAConjB->w = quatB->w * quatA->w + quatB->x * quatA->x + quatA->y * quatB->y + quatB->z * quatA->z;
    outAConjB->x = quatB->w * quatA->x - quatA->w * quatB->x - quatB->z * quatA->y + quatA->z * quatB->y;
    outAConjB->y = quatB->w * quatA->y - quatA->w * quatB->y - quatA->z * quatB->x + quatB->z * quatA->x;
    outAConjB->z = quatB->w * quatA->z - quatA->w * quatB->z - quatB->y * quatA->x + quatA->y * quatB->x;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-tomatrix
 * @recoil-artifact defines .text recoil:function:0x475a80: zMathQuatToMatrix
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Raw assembly: the inline zMath::Vec3Add expansion [0x475a9c,0x475abf)
 * (requires zmth_quat.c /Ob1).
 *
 * Purpose: expands a quaternion into the rotational part of a 4x3 matrix.
 */
void __fastcall zMathQuatToMatrix(const zQuat* quat, zMat4x3* outMatrix3x3)
{
    zVec3 twice;
    float xx2;
    float yy2;
    float zz2;
    float xy2;
    float yz2;
    float xz2;
    float xw2;
    float yw2;
    float zw2;
    float* out;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    Vec3Add((const zVec3*)&quat->x, (const zVec3*)&quat->x, &twice);
#else
    twice.x = quat->x + quat->x;
    twice.y = quat->y + quat->y;
    twice.z = quat->z + quat->z;
#endif

    xx2 = twice.x * quat->x;
    yy2 = twice.y * quat->y;
    zz2 = twice.z * quat->z;
    xy2 = twice.y * quat->x;
    yz2 = twice.z * quat->y;
    xz2 = twice.x * quat->z;
    xw2 = twice.x * quat->w;
    yw2 = twice.y * quat->w;
    zw2 = twice.z * quat->w;

    out = &outMatrix3x3->xx;
    *out++ = 1.0f - yy2 - zz2;
    *out++ = zw2 + xy2;
    *out++ = xz2 - yw2;
    *out++ = xy2 - zw2;
    *out++ = 1.0f - zz2 - xx2;
    *out++ = xw2 + yz2;
    *out++ = yw2 + xz2;
    *out++ = yz2 - xw2;
    *out = 1.0f - xx2 - yy2;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.quat.vector-length
 *
 * Purpose: return FSQRT of the grouped (x*x + y*y) + z*z sum as binary32.
 * Reconstruction: zmth_quat.c-resident copy of the Camera.c inline helper,
 * following the zwep_ammo.c-resident precedent; this /Ob1 TU inlines it at
 * 0x475b80 with the parameter argument bound to its inline parameter home.
 * Raw assembly: identical body to the reviewed Camera.c Vec3Length island.
 * Retail inline-expansion evidence: the listed consumer contains the operand reloads, arithmetic
 * sequence and result store without a call at that site; the original inline helper's header
 * ownership and declaration placement are not established (TU-resident reconstruction model).
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x475b80.
 */
__inline float Vec3Length(const zVec3* vec)
{
    float vecLength;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov ecx, vec
    fld dword ptr [ecx]zVec3.x
    fmul dword ptr [ecx]zVec3.x
    fld dword ptr [ecx]zVec3.y
    fmul dword ptr [ecx]zVec3.y
    fld dword ptr [ecx]zVec3.z
    fmul dword ptr [ecx]zVec3.z
    fxch st(1)
    faddp st(2), st
    faddp st(1), st
    fsqrt
    fstp vecLength
    }
#else
    vecLength = (float)sqrt((vec->x * vec->x + vec->y * vec->y) + vec->z * vec->z);
#endif
    return vecLength;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromrotationvector
 * @recoil-artifact defines .text recoil:function:0x475b80: zMathQuatExp
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.quat.vector-length
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Raw assembly: inline zMath::Vec3Length [0x475b92,0x475bb0)
 * and the inline zMath::SinCos fsincos arm [0x475c01,0x475c10)
 * (requires zmth_quat.c /Ob1).
 *
 * Purpose: computes the quaternion exponential of a half-angle rotation vector, returning identity for zero.
 */
void __fastcall zMathQuatExp(const zVec3* rotationVector, zQuat* outQuat)
{
    float sinLength;
    const float length = Vec3Length(rotationVector);
    float scale;

    if (length == 0.0f) {
        outQuat->w = 1.0f;
        outQuat->x = 0.0f;
        outQuat->y = 0.0f;
        outQuat->z = 0.0f;
        return;
    }

    SinCos(length, &sinLength, &outQuat->w);
    scale = sinLength / length;
    outQuat->x = scale * rotationVector->x;
    outQuat->y = scale * rotationVector->y;
    outQuat->z = scale * rotationVector->z;
}
