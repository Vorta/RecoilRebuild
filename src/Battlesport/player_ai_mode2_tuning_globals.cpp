#include "player.h"

extern "C" {
/**
 * Reimplements data 0x4da0c0: g_Player_AiMode2_PathFollowPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI
 * path-follow pitch steering input.
 * Purpose: Scales path-follow vertical steering error into pitch input.
 */
float g_Player_AiMode2_PathFollowPitchInputScale = 0.0174499992f;
/**
 * Reimplements data 0x4da0c4: g_Player_AiMode2_PathFollowPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * path-follow pitch input scale.
 * Purpose: Scales path-follow pitch input into turn correction.
 */
float g_Player_AiMode2_PathFollowPitchTurnGain = 5.69999981f;
/**
 * Reimplements data 0x4da0c8: g_Player_AiMode2_SteeringPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical distance into pitch input.
 */
float g_Player_AiMode2_SteeringPitchInputScale = 0.800000012f;
/**
 * Reimplements data 0x4da0cc: g_Player_AiMode2_SteeringPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * steering pitch input scale.
 * Purpose: Scales steering pitch input into turn correction.
 */
float g_Player_AiMode2_SteeringPitchTurnGain = 5.69999981f;
/**
 * Reimplements data 0x4da0d0: g_Player_AiMode2_SteeringVerticalErrorScale.
 * BN types this as an initialized .data float read by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical error before pitch correction.
 */
float g_Player_AiMode2_SteeringVerticalErrorScale = 0.100000001f;
/**
 * Reimplements data 0x4da0d4: g_Player_AiMode2_TuningScalar55A.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the first Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55A = 55.0f;
/**
 * Reimplements data 0x4da0d8: g_Player_AiMode2_TuningScalar55B.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the second Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55B = 55.0f;
/**
 * Reimplements data 0x4da0dc: g_Player_AiMode2_TuningScalar5.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 5.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar5 = 5.0f;
/**
 * Reimplements data 0x4da0e0: g_Player_AiMode2_TuningScalar10.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 10.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar10 = 10.0f;
/**
 * Reimplements data 0x4da0e4: g_Player_AiMode2_OffsetTargetRotateCos15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Purpose: Stores the retail cosine scalar for offset-target rotation.
 */
float g_Player_AiMode2_OffsetTargetRotateCos15Deg = 0.965900004f;
/**
 * Reimplements data 0x4da0e8: g_Player_AiMode2_OffsetTargetRotateSin15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Purpose: Stores the retail sine scalar for offset-target rotation.
 */
float g_Player_AiMode2_OffsetTargetRotateSin15Deg = 0.25879999995f;
} // extern "C"
