; SPIR-V\n"
; Version: 1.0
; Generator: Khronos Glslang Reference Front End; 1
; Updated bound to reflect hand added variables
; Bound: 32
; Schema: 0
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main" %13 %25
               OpExecutionMode %4 OriginUpperLeft
               OpSource GLSL 400
               OpSourceExtension "GL_ARB_separate_shader_objects"
               OpSourceExtension "GL_ARB_shading_language_420pack"
               OpName %4 "main"
               OpName %13 "outColor"
               OpName %21 "tex"
               OpName %25 "texcoord"
               OpDecorate %7 SpecId 5
               OpDecorate %13 Location 0
               OpDecorate %14 SpecId 7

; Updated the spec constant decoration from %14 -> %29
               OpDecorate %29 SpecId 8

; Updated the spec constant decoration from %14 -> %30
               OpDecorate %30 SpecId 9

               OpDecorate %21 DescriptorSet 0
               OpDecorate %21 Binding 1
               OpDecorate %25 Location 0
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %6 = OpTypeBool
          %7 = OpSpecConstantFalse %6
         %10 = OpTypeFloat 32
         %11 = OpTypeVector %10 4
         %12 = OpTypePointer Output %11
         %13 = OpVariable %12 Output
         %14 = OpSpecConstant %10 0

; Add new spec constants
         %29 = OpSpecConstant %10 0.0
         %30 = OpSpecConstant %10 0.0

; Uncomment the below to emulate the app's specialization
;         %7 = OpSpecConstantTrue %6
;         %30 = OpSpecConstant %10 1.0

         %15 = OpConstant %10 1
         %18 = OpTypeImage %10 2D 0 0 0 1 Unknown
         %19 = OpTypeSampledImage %18
         %20 = OpTypePointer UniformConstant %19
         %21 = OpVariable %20 UniformConstant
         %23 = OpTypeVector %10 2
         %24 = OpTypePointer Input %23
         %25 = OpVariable %24 Input
         %27 = OpConstant %10 0
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpSelectionMerge %9 None
               OpBranchConditional %7 %8 %17
          %8 = OpLabel
; Consume the color spec_constants  %14 -> %29, %14 -> %30
         %16 = OpCompositeConstruct %11 %14 %29 %30 %15

               OpStore %13 %16
               OpBranch %9
         %17 = OpLabel
         %22 = OpLoad %19 %21
         %26 = OpLoad %23 %25
         %28 = OpImageSampleExplicitLod %11 %22 %26 Lod %27
               OpStore %13 %28
               OpBranch %9
          %9 = OpLabel
               OpReturn
               OpFunctionEnd