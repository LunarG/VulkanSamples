//===- glsl_glass_manager.cpp - Mesa customization of PrivateManager -----===//
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
// Customization of gla::PrivateManager for Mesa
//
//===----------------------------------------------------------------------===//

#include "glsl_glass_manager.h"
#include "glsl_glass_backend.h"
#include "glsl_glass_backend_translator.h"

gla::MesaGlassManager::MesaGlassManager(const EShLanguage language)
{
   createNonreusable();
   backEnd = gla::GetMesaGlassBackEnd(language);
}

gla::MesaGlassManager::~MesaGlassManager()
{
   freeNonreusable();
   gla::ReleaseMesaGlassBackEnd(backEnd);
}

void gla::MesaGlassManager::clear()
{
   freeNonreusable();
   createNonreusable();
}

void gla::MesaGlassManager::createContext()
{
   delete context;
   context = new llvm::LLVMContext;
}

void gla::MesaGlassManager::createNonreusable()
{
   backEndTranslator = gla::GetMesaGlassTranslator(this);
}

void gla::MesaGlassManager::freeNonreusable()
{
   gla::ReleaseMesaGlassTranslator(backEndTranslator);
   while (! freeList.empty()) {
      delete freeList.back();
      freeList.pop_back();
   }
   delete module;
   module = 0;
   delete context;
   context = 0;
}

gla::Manager* gla::getManager(const EShLanguage language)
{
   return new gla::MesaGlassManager(language);
}

gla::MesaGlassTranslator* gla::MesaGlassManager::getBackendTranslator()
{
   // We know this must be a MesaGlassTranslator, because we only
   // get them from our factory, which only makes MesaGlassTranslators.
   return static_cast<gla::MesaGlassTranslator*>(backEndTranslator);
}
