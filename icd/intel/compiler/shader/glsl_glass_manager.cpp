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

//===- glsl_glass_manager.cpp - Mesa customization of PrivateManager -----===//
//
// Customization of gla::PrivateManager for Mesa
//
//===---------------------------------------------------------------------===//

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
