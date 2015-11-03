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

//===- glsl_glass_manager.h - Mesa customization of PrivateManager -----===//
//
// Customization of gla::PrivateManager for Mesa
//
//===-------------------------------------------------------------------===//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include "Core/PrivateManager.h"
#include "glslang/Public/ShaderLang.h"
#pragma GCC diagnostic pop

namespace gla {

class MesaGlassTranslator;  // forward declare

class MesaGlassManager : public gla::PrivateManager {
public:
   MesaGlassManager(const EShLanguage);
   virtual ~MesaGlassManager();
   virtual void clear();
   void createContext();
   MesaGlassTranslator* getBackendTranslator();

protected:
   void freeNonreusable();
   void createNonreusable();
};

// We provide our own overload of getManager to pass in some data
Manager* getManager(const EShLanguage);

} // namespace gla
