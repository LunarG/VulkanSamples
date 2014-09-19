//===- glsl_glass_backend.h - Mesa customization of gla::BackEnd -----===//
//
// LunarGLASS: An Open Modular Shader Compiler Architecture
// Copyright (C) 2010-2014 LunarG, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
//     Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
// 
//     Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following
//     disclaimer in the documentation and/or other materials provided
//     with the distribution.
// 
//     Neither the name of LunarG Inc. nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//===----------------------------------------------------------------------===//
//
// Author: LunarG
//
// Customization of gla::BackEnd for Mesa
//
//===----------------------------------------------------------------------===//

#include "Core/Backend.h"
#include "glslang/Public/ShaderLang.h"

namespace gla {

class MesaGlassBackEnd : public gla::BackEnd {
public:
   MesaGlassBackEnd(const EShLanguage l) :
       language(l)
   {
      // LunarGLASS decomposition.  Ask for everything except the below.
      for (int d = 0; d < EDiCount; ++d)
         decompose[d] = true;

      // Turn off some decompositions we don't need
      decompose[EDiDot]               =
      decompose[EDiMin]               =
      decompose[EDiMax]               =
      decompose[EDiExp]               =
      decompose[EDiLog]               =
      decompose[EDiSign]              =
      decompose[EDiAny]               =
      decompose[EDiAll]               =
      decompose[EDiNot]               =
      decompose[EDiFraction]          =
      decompose[EDiInverseSqrt]       =
      decompose[EDiFma]               =
      decompose[EDiModF]              =
      decompose[EDiMix]               =
      decompose[EDiFixedTransform]    =
      decompose[EDiPackUnorm2x16]     =
      decompose[EDiPackUnorm4x8]      =
      decompose[EDiPackSnorm4x8]      =
      decompose[EDiUnpackUnorm2x16]   =
      decompose[EDiUnpackUnorm4x8]    =
      decompose[EDiUnpackSnorm4x8]    =
      decompose[EDiPackDouble2x32]    =
      decompose[EDiUnpackDouble2x32]  =
      decompose[EDiPowi]              =
      decompose[EDiAsin]              =
      decompose[EDiAcos]              =
      decompose[EDiAtan]              =
      decompose[EDiAtan2]             =
      decompose[EDiSinh]              =
      decompose[EDiCosh]              =
      decompose[EDiTanh]              =
      decompose[EDiASinh]             =
      decompose[EDiACosh]             =
      decompose[EDiATanh]             =
      decompose[EDiIsNan]             =
      decompose[EDiTextureProjection] = 
      decompose[EDiRefract]           = 
         false;

      // Explicitly mention the ones we want LunarGlass to decompose for us,
      // just for clarity, even though the above loop asks for decomposition
      // by default.
      decompose[EDiClamp]       = 
      decompose[EDiFilterWidth] = true;
   }
   
   void getRegisterForm(int& outerSoA, int& innerAoS)
   {
       switch (language)
       {
           case EShLangVertex:
           case EShLangTessControl:
           case EShLangTessEvaluation:
           case EShLangGeometry:
           case EShLangFragment:
           case EShLangCompute:
               outerSoA = 1;
               innerAoS = 4;
               break;
           default:
               assert(0); // TODO: error handling here
       }
   }

   // We don't want phi functions
   bool getRemovePhiFunctions() { return true; }

   
   bool getDeclarePhiCopies() { return true; }

   // Not all backends yet support conditional discards.  Ask LunarGLASS to
   // avoid them.
   bool hoistDiscards() { return false; }

   // Ask LunarGlass for mat*vec & vec*mat intrinsics
   bool useColumnBasedMatrixIntrinsics() { return true; }

private:
   const EShLanguage language;
};

//
// factory for the Mesa LunarGLASS backend
//
BackEnd* GetMesaGlassBackEnd(const EShLanguage);
void ReleaseMesaGlassBackEnd(BackEnd*);

} // namespace gla
