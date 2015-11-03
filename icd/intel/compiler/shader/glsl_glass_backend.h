/*
 *
 * Copyright (C) 2015 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Steve K <srk@LunarG.com>
 *
 */

//===- glsl_glass_backend.h - Mesa customization of gla::BackEnd -----===//
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
      decompose[EDiFaceForward]       =
         false;

      // Explicitly mention the ones we want LunarGlass to decompose for us,
      // just for clarity, even though the above loop asks for decomposition
      // by default.
      decompose[EDiClamp]       = false;
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

   bool useLogicalIo() { return true; }

private:
   const EShLanguage language;
};

//
// factory for the Mesa LunarGLASS backend
//
BackEnd* GetMesaGlassBackEnd(const EShLanguage);
void ReleaseMesaGlassBackEnd(BackEnd*);

} // namespace gla
