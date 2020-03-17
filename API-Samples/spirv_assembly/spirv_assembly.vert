; SPIR-V
; Version: 1.0
; Generator: Khronos Glslang Reference Front End; 1
; Bound: 35
; Schema: 0
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Vertex %main "main" %texcoord %inTexCoords %_ %pos
               OpSource GLSL 400
               OpSourceExtension "GL_ARB_separate_shader_objects"
               OpSourceExtension "GL_ARB_shading_language_420pack"
               OpName %main "main"
               OpName %texcoord "texcoord"
               OpName %inTexCoords "inTexCoords"
               OpName %gl_PerVertex "gl_PerVertex"
               OpMemberName %gl_PerVertex 0 "gl_Position"
               OpMemberName %gl_PerVertex 1 "gl_PointSize"
               OpMemberName %gl_PerVertex 2 "gl_ClipDistance"
               OpName %_ ""
               OpName %buf "buf"
               OpMemberName %buf 0 "mvp"
               OpName %ubuf "ubuf"
               OpName %pos "pos"
               OpDecorate %texcoord Location 0
               OpDecorate %inTexCoords Location 1
               OpMemberDecorate %gl_PerVertex 0 BuiltIn Position
               OpMemberDecorate %gl_PerVertex 1 BuiltIn PointSize
               OpMemberDecorate %gl_PerVertex 2 BuiltIn ClipDistance
               OpDecorate %gl_PerVertex Block
               OpMemberDecorate %buf 0 ColMajor
               OpMemberDecorate %buf 0 Offset 0
               OpMemberDecorate %buf 0 MatrixStride 16
               OpDecorate %buf Block
               OpDecorate %ubuf DescriptorSet 0
               OpDecorate %ubuf Binding 0
               OpDecorate %pos Location 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
    %v2float = OpTypeVector %float 2
%_ptr_Output_v2float = OpTypePointer Output %v2float
   %texcoord = OpVariable %_ptr_Output_v2float Output
%_ptr_Input_v2float = OpTypePointer Input %v2float
%inTexCoords = OpVariable %_ptr_Input_v2float Input
    %v4float = OpTypeVector %float 4
       %uint = OpTypeInt 32 0
     %uint_1 = OpConstant %uint 1
%_arr_float_uint_1 = OpTypeArray %float %uint_1
%gl_PerVertex = OpTypeStruct %v4float %float %_arr_float_uint_1
%_ptr_Output_gl_PerVertex = OpTypePointer Output %gl_PerVertex
          %_ = OpVariable %_ptr_Output_gl_PerVertex Output
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
%mat4v4float = OpTypeMatrix %v4float 4
        %buf = OpTypeStruct %mat4v4float
%_ptr_Uniform_buf = OpTypePointer Uniform %buf
       %ubuf = OpVariable %_ptr_Uniform_buf Uniform
%_ptr_Uniform_mat4v4float = OpTypePointer Uniform %mat4v4float
%_ptr_Input_v4float = OpTypePointer Input %v4float
        %pos = OpVariable %_ptr_Input_v4float Input
%_ptr_Output_v4float = OpTypePointer Output %v4float
       %main = OpFunction %void None %3
          %5 = OpLabel
         %12 = OpLoad %v2float %inTexCoords
               OpStore %texcoord %12
         %27 = OpAccessChain %_ptr_Uniform_mat4v4float %ubuf %int_0
         %28 = OpLoad %mat4v4float %27
         %31 = OpLoad %v4float %pos
         %32 = OpMatrixTimesVector %v4float %28 %31
         %34 = OpAccessChain %_ptr_Output_v4float %_ %int_0
               OpStore %34 %32
               OpReturn
               OpFunctionEnd