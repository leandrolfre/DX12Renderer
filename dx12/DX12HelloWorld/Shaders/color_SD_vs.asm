//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Buffer Definitions: 
//
// cbuffer cbPerObject
// {
//
//   float4x4 gWorld;                   // Offset:    0 Size:    64
//   uint gMaterialIndex;               // Offset:   64 Size:     4 [unused]
//   uint pad1;                         // Offset:   68 Size:     4 [unused]
//   uint pad2;                         // Offset:   72 Size:     4 [unused]
//   uint pad3;                         // Offset:   76 Size:     4 [unused]
//
// }
//
// cbuffer cbPerPass
// {
//
//   float4x4 gView;                    // Offset:    0 Size:    64 [unused]
//   float4x4 gInvView;                 // Offset:   64 Size:    64 [unused]
//   float4x4 gProj;                    // Offset:  128 Size:    64 [unused]
//   float4x4 gInvProj;                 // Offset:  192 Size:    64 [unused]
//   float4x4 gViewProj;                // Offset:  256 Size:    64
//   float4x4 gInvViewProj;             // Offset:  320 Size:    64 [unused]
//   float3 gEyePosW;                   // Offset:  384 Size:    12 [unused]
//   float cbPerObjectPad1;             // Offset:  396 Size:     4 [unused]
//   float2 gRenderTargetSize;          // Offset:  400 Size:     8 [unused]
//   float2 gInvRenderTargetSize;       // Offset:  408 Size:     8 [unused]
//   float gNearZ;                      // Offset:  416 Size:     4 [unused]
//   float gFarZ;                       // Offset:  420 Size:     4 [unused]
//   float gTotalTime;                  // Offset:  424 Size:     4 [unused]
//   float gDeltaTime;                  // Offset:  428 Size:     4 [unused]
//   float4 gAmbientLight;              // Offset:  432 Size:    16 [unused]
//   
//   struct Light
//   {
//       
//       float3 Strength;               // Offset:  448
//       float FalloffStart;            // Offset:  460
//       float3 Direction;              // Offset:  464
//       float FalloffEnd;              // Offset:  476
//       float3 Position;               // Offset:  480
//       float SpotPower;               // Offset:  492
//       float4x4 ShadowViewProj;       // Offset:  496
//       float4x4 ShadowTransform;      // Offset:  560
//
//   } gLights[16];                     // Offset:  448 Size:  2816
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      ID      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- ------- -------------- ------
// cbPerObject                       cbuffer      NA          NA     CB0            cb0      1 
// cbPerPass                         cbuffer      NA          NA     CB1            cb1      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// POSITION                 0   xyz         0     NONE   float   xyz 
// TANGENT                  0   xyz         1     NONE   float   xyz 
// NORMAL                   0   xyz         2     NONE   float   xyz 
// TEX                      0   xy          3     NONE   float   xy  
// TEX                      1   xy          4     NONE   float       
// COLOR                    0   xyzw        5     NONE   float       
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float   xyzw
// POSITION                 0   xyz         1     NONE   float   xyz 
// POSITION                 1   xyzw        2     NONE   float   xyzw
// NORMAL                   0   xyz         3     NONE   float   xyz 
// TEXCOORD                 0   xy          4     NONE   float   xy  
// TANGENT                  0   xyz         5     NONE   float   xyz 
// Binormal                 0   xyz         6     NONE   float   xyz 
//
vs_5_1
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_constantbuffer CB0[0:0][4], immediateIndexed, space=0
dcl_constantbuffer CB1[1:1][39], immediateIndexed, space=0
dcl_input v0.xyz
dcl_input v1.xyz
dcl_input v2.xyz
dcl_input v3.xy
dcl_output_siv o0.xyzw, position
dcl_output o1.xyz
dcl_output o2.xyzw
dcl_output o3.xyz
dcl_output o4.xy
dcl_output o5.xyz
dcl_output o6.xyz
dcl_temps 7
//
// Initial variable locations:
//   v0.x <- vin.PosL.x; v0.y <- vin.PosL.y; v0.z <- vin.PosL.z; 
//   v1.x <- vin.Tangent.x; v1.y <- vin.Tangent.y; v1.z <- vin.Tangent.z; 
//   v2.x <- vin.Normal.x; v2.y <- vin.Normal.y; v2.z <- vin.Normal.z; 
//   v3.x <- vin.Tex0.x; v3.y <- vin.Tex0.y; 
//   v4.x <- vin.Tex1.x; v4.y <- vin.Tex1.y; 
//   v5.x <- Color.x; v5.y <- Color.y; v5.z <- Color.z; v5.w <- Color.w; 
//   o6.x <- <VS return value>.BitangentW.x; o6.y <- <VS return value>.BitangentW.y; o6.z <- <VS return value>.BitangentW.z; 
//   o5.x <- <VS return value>.TangentW.x; o5.y <- <VS return value>.TangentW.y; o5.z <- <VS return value>.TangentW.z; 
//   o4.x <- <VS return value>.TexCoord.x; o4.y <- <VS return value>.TexCoord.y; 
//   o3.x <- <VS return value>.NormalW.x; o3.y <- <VS return value>.NormalW.y; o3.z <- <VS return value>.NormalW.z; 
//   o2.x <- <VS return value>.ShadowPosH.x; o2.y <- <VS return value>.ShadowPosH.y; o2.z <- <VS return value>.ShadowPosH.z; o2.w <- <VS return value>.ShadowPosH.w; 
//   o1.x <- <VS return value>.PosW.x; o1.y <- <VS return value>.PosW.y; o1.z <- <VS return value>.PosW.z; 
//   o0.x <- <VS return value>.PosH.x; o0.y <- <VS return value>.PosH.y; o0.z <- <VS return value>.PosH.z; o0.w <- <VS return value>.PosH.w
//
#line 36 "C:\gamedev\DX12Renderer\dx12\DX12HelloWorld\Shaders\color_SD.hlsl"
mul r0.xyzw, v0.xxxx, CB0[0][0].xyzw
mul r1.xyzw, v0.yyyy, CB0[0][1].xyzw
add r0.xyzw, r0.xyzw, r1.xyzw
mul r1.xyzw, v0.zzzz, CB0[0][2].xyzw
add r0.xyzw, r0.xyzw, r1.xyzw
mul r1.xyzw, CB0[0][3].xyzw, l(1.000000, 1.000000, 1.000000, 1.000000)
add r0.xyzw, r0.xyzw, r1.xyzw  // r0.x <- posW.x; r0.y <- posW.y; r0.z <- posW.z; r0.w <- posW.w

#line 37
mov r0.xyz, r0.xyzx  // r0.x <- vout.PosW.x; r0.y <- vout.PosW.y; r0.z <- vout.PosW.z

#line 40
mul r1.xyzw, r0.xxxx, CB1[1][16].xyzw
mul r2.xyzw, r0.yyyy, CB1[1][17].xyzw
add r1.xyzw, r1.xyzw, r2.xyzw
mul r2.xyzw, r0.zzzz, CB1[1][18].xyzw
add r1.xyzw, r1.xyzw, r2.xyzw
mul r2.xyzw, r0.wwww, CB1[1][19].xyzw
add r1.xyzw, r1.xyzw, r2.xyzw  // r1.x <- vout.PosH.x; r1.y <- vout.PosH.y; r1.z <- vout.PosH.z; r1.w <- vout.PosH.w

#line 41
mul r2.xyzw, r0.xxxx, CB1[1][35].xyzw
mul r3.xyzw, r0.yyyy, CB1[1][36].xyzw
add r2.xyzw, r2.xyzw, r3.xyzw
mul r3.xyzw, r0.zzzz, CB1[1][37].xyzw
add r2.xyzw, r2.xyzw, r3.xyzw
mul r3.xyzw, r0.wwww, CB1[1][38].xyzw
add r2.xyzw, r2.xyzw, r3.xyzw  // r2.x <- vout.ShadowPosH.x; r2.y <- vout.ShadowPosH.y; r2.z <- vout.ShadowPosH.z; r2.w <- vout.ShadowPosH.w

#line 42
mul r3.xyz, v2.xxxx, CB0[0][0].xyzx
mul r4.xyz, v2.yyyy, CB0[0][1].xyzx
add r3.xyz, r3.xyzx, r4.xyzx
mul r4.xyz, v2.zzzz, CB0[0][2].xyzx
add r3.xyz, r3.xyzx, r4.xyzx  // r3.x <- vout.NormalW.x; r3.y <- vout.NormalW.y; r3.z <- vout.NormalW.z

#line 43
mul r4.xyz, v1.xxxx, CB0[0][0].xyzx
mul r5.xyz, v1.yyyy, CB0[0][1].xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mul r5.xyz, v1.zzzz, CB0[0][2].xyzx
add r4.xyz, r4.xyzx, r5.xyzx  // r4.x <- vout.TangentW.x; r4.y <- vout.TangentW.y; r4.z <- vout.TangentW.z

#line 44
mul r5.xyz, r3.yzxy, r4.zxyz
mul r6.xyz, r3.zxyz, r4.yzxy
mov r6.xyz, -r6.xyzx
add r5.xyz, r5.xyzx, r6.xyzx  // r5.x <- vout.BitangentW.x; r5.y <- vout.BitangentW.y; r5.z <- vout.BitangentW.z

#line 45
mov r6.xy, v3.xyxx  // r6.x <- vout.TexCoord.x; r6.y <- vout.TexCoord.y

#line 46
mov o0.xyzw, r1.xyzw
mov o2.xyzw, r2.xyzw
mov o1.xyz, r0.xyzx
mov o3.xyz, r3.xyzx
mov o5.xyz, r4.xyzx
mov o6.xyz, r5.xyzx
mov o4.xy, r6.xyxx
ret 
// Approximately 45 instruction slots used
