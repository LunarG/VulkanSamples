//===- glsl_glass_backend_translator.h - Mesa customization of gla::BackEndTranslator -----===//
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
// Customization of gla::BackEndTranslator for Mesa
//
//===----------------------------------------------------------------------===//


#include "Core/PrivateManager.h"
#include "Core/Backend.h"
#include "list.h"
#include <map>
#include <list>
#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

// Forward declare to avoid pulling in entire unneeded definitions
struct _mesa_glsl_parse_state;
struct glsl_type;
struct gl_shader;
struct gl_context;
struct ir_instruction;
struct ir_function;
struct ir_rvalue;
struct ir_constant;
struct ir_function_signature;
struct ir_variable;
struct ir_dereference;
struct ir_if;
struct ir_loop;
struct exec_list;

// Forward decls for LLVM
namespace llvm {
   class IntrinsicInst;
};

namespace gla {
    BackEndTranslator* GetMesaGlassTranslator(Manager*);
    void ReleaseMesaGlassTranslator(gla::BackEndTranslator*);

    BackEnd* GetDummyBackEnd();
    void ReleaseMesaGlassBackEnd(gla::BackEnd*);

    class MesaGlassTranslator : public gla::BackEndTranslator {
    public:
        MesaGlassTranslator(Manager* m) :
            BackEndTranslator(m),
            fnReturnType(0),
            fnName(0),
            fnFunction(0),
            fnSignature(0),
            state(0),
            ctx(0)
        { }

        ~MesaGlassTranslator();

        void initializeTranslation(gl_context *, _mesa_glsl_parse_state *, gl_shader *);
        void finalizeTranslation();
        void seedGlobalDeclMap();
        const char* getInfoLog() const { return infoLog.c_str(); }

        static void initSamplerTypes();

    protected:
        bool hoistUndefOperands()  {  return false; }

        // Translation methods from LunarGlass ---------------------------------
        void start(llvm::Module&);
        void end(llvm::Module&) { /* nothing to do */ }

        void addStructType(llvm::StringRef, const llvm::Type*);
        void addGlobal(const llvm::GlobalVariable*);
        void addGlobalConst(const llvm::GlobalVariable*);
        void addIoDeclaration(gla::EVariableQualifier qualifier, const llvm::MDNode* mdNode);
        void startFunctionDeclaration(const llvm::Type* type, llvm::StringRef name);
        void addArgument(const llvm::Value* value, bool last);
        void endFunctionDeclaration();
        void startFunctionBody();
        void endFunctionBody();
        void addInstruction(const llvm::Instruction* llvmInstruction, bool lastBlock, bool referencedOutsideScope=false);
        void declarePhiCopy(const llvm::Value* dst);
        void addPhiCopy(const llvm::Value* dst, const llvm::Value* src);
        void addPhiAlias(const llvm::Value* dst, const llvm::Value* src);
        void addIf(const llvm::Value* cond, bool invert=false);
        void addElse();
        void addEndif();
        void beginConditionalLoop();
        void beginSimpleConditionalLoop(const llvm::CmpInst* cmp, const llvm::Value* op1, const llvm::Value* op2, bool invert=false);
        void beginForLoop(const llvm::PHINode* phi, llvm::ICmpInst::Predicate, unsigned bound, unsigned increment);
        void beginSimpleInductiveLoop(const llvm::PHINode* phi, const llvm::Value* count);
        void beginLoop();
        void endLoop();
        void addLoopExit(const llvm::Value* condition=NULL, bool invert=false);
        void addLoopBack(const llvm::Value* condition=NULL, bool invert=false);
        void addDiscard();
        void print() { }

        // Internal methods ----------------------------------------------------
        void setParseState(_mesa_glsl_parse_state *s) { state = s; }
        void setContext(gl_context *c) { ctx = c; }
        void setShader(gl_shader *s) { shader = s; }
        void resetFnTranslationState();
        const llvm::GetElementPtrInst* getGepAsInst(const llvm::Value* gep);

        int irCmpOp(int) const; // use ints to avoid lack of forward decls of enums in C++

        // For loop with ir_rvalue inputs
        void beginForLoop(const llvm::PHINode* phi, llvm::ICmpInst::Predicate, ir_rvalue* bound, ir_rvalue* increment);

        // IR flavor of simple inductive loop
        void beginSimpleInductiveLoop(const llvm::PHINode* phi, ir_rvalue*);

        // Add IR if statement from ir_rvalue
        inline void addIf(ir_rvalue* cond, bool invert=false);

        // Add IR loop exit statement from ir_rvalue
        inline void addIRLoopExit(ir_rvalue* condition=NULL, bool invert=false);

        // Convert structure types (also used for blocks)
        const glsl_type* convertStructType(const llvm::Type*, llvm::StringRef name, const llvm::MDNode*,
                                           gla::EMdTypeLayout, gla::EVariableQualifier, bool isBlock);

        // Convert an LLVM type to an HIR type
        const glsl_type* llvmTypeToHirType(const llvm::Type*, const llvm::MDNode* = 0, const llvm::Value* = 0);

        // Emit vertex
        inline void emitIREmitVertex(const llvm::Instruction*);

        // End primitive
        inline void emitIREndPrimitive(const llvm::Instruction*);

        // Add IR sign extension
        inline void emitIRSext(const llvm::Instruction*);

        // Add alloc
        inline void emitIRalloca(const llvm::Instruction*);

        // Add a binary op
        template <int ops> inline void emitOp(int /* ir_expression_operation */, const llvm::Instruction*);

        // Add a binary op of either logical or bitwise type
        template <int ops> inline void emitOpBit(int /* logical_op */, int /* bitwise_op */, const llvm::Instruction*);

        // Add a builtin function call
        inline void emitFn(const char* name, const llvm::Instruction*);

        // Add a saturation.  TODO: Want to ask for a LunarGlass
        // decomposition of this, which doesn't exist yet.
        inline void emitIRSaturate(const llvm::CallInst*);

        // Add a clamp.  TODO: We don't really want to do this here; better
        // for LunarGlass to do it, but there's a defect in the LunarGlass decomposition
        // exposed by taiji-shaders/shader_32.frag.  This is a workaround until
        // that's sorted.
        inline void emitIRClamp(const llvm::CallInst*);

        // Add a conditional discard
        inline void emitIRDiscardCond(const llvm::CallInst*);

        // Add fixed transform
        inline void emitIRFTransform(const llvm::CallInst*);

        // Add a comparison op
        inline void emitCmp(const llvm::Instruction*);

        // Add a function call or intrinsic
        inline void emitIRCall(const llvm::CallInst*);

        // Add a function call or intrinsic
        inline void emitIRReturn(const llvm::Instruction*, bool lastBlock);

        // Determine glsl matrix type
        inline const glsl_type* glslMatType(int numCols, int numRows) const;

        // Create an IR matrix object by direct read from a uniform matrix,
        // or moves of column vectors into a temp matrix.
        inline ir_rvalue* intrinsicMat(const llvm::Instruction*,
                                       int firstColumn, int numCols, int numRows);

        // Add IR mat*vec or vec*mat intrinsic
        inline void emitIRMatMul(const llvm::Instruction*, int numCols, bool matLeft);

        // Add IR mat*mat intrinsic
        inline void emitIRMatTimesMat(const llvm::Instruction*, int numLeftCols, int numrightCols);

        // IR intrinsics
        inline void emitIRIntrinsic(const llvm::IntrinsicInst*);

        // Track maximum array value used
        inline void trackMaxArrayElement(ir_rvalue* deref, int index) const;

        // IR texture intrinsics
        inline void emitIRTexture(const llvm::IntrinsicInst*, bool gather);

        // IR insertion intrinsics
        inline void emitIRMultiInsert(const llvm::IntrinsicInst*);

        // IR swizzle intrinsics
        inline void emitIRSwizzle(const llvm::IntrinsicInst*);

        // Load operation (but don't add to instruction stream)
        inline ir_rvalue* makeIRLoad(const llvm::Instruction*, const glsl_type* = 0);

        // Load operation
        inline void emitIRLoad(const llvm::Instruction*);

        // Store operation
        inline void emitIRStore(const llvm::Instruction*);

        // Add a function call or intrinsic
        inline void emitIRCallOrIntrinsic(const llvm::Instruction*);

        // Vector component extraction
        inline void emitIRExtractElement(const llvm::Instruction*);

        // ternary operator
        inline void emitIRSelect(const llvm::Instruction*);

        // Vector component insertion
        inline void emitIRInsertElement(const llvm::Instruction*);

        // Array component extraction
        inline void emitIRExtractValue(const llvm::Instruction*);

        inline void FindGepType(const llvm::Instruction*,
                                const llvm::Type*&,
                                const llvm::Type*&,
                                const llvm::MDNode*&);

        // Deference GEP
        inline ir_rvalue* dereferenceGep(const llvm::Type*&, ir_rvalue*, llvm::Value*,
                                         int index, const llvm::MDNode*&, EMdTypeLayout* = 0);

        // Traverse GEP instruction
        inline ir_rvalue* traverseGEP(const llvm::Instruction*,
                                      ir_rvalue* aggregate,
                                      EMdTypeLayout* = 0);

        // Array component insertion
        inline void emitIRInsertValue(const llvm::Instruction*);

        // See if this constant is a zero
        inline bool isConstantZero(const llvm::Constant*) const;

        // Create a simple scalar constant
        template <typename T> inline T newIRScalarConstant(const llvm::Constant*) const;

        // Add a constant value for this LLVM value
        inline ir_constant* newIRConstant(const llvm::Value*);

        // Add a global for this LLVM value
        inline ir_rvalue* newIRGlobal(const llvm::Value*, const char* name = 0);

        // Add type-safe undefined value in case someone looks up a not-defined value
        inline ir_constant* addIRUndefined(const llvm::Type*);

        // Add variable from LLVM value
        inline ir_rvalue* getIRValue(const llvm::Value*, ir_instruction* = 0);

        // Add instruction to top of instruction list stack
        inline void addIRInstruction(const llvm::Value*, ir_instruction*);

        // raw add instruction: don't add map entry, just append to inst list
        inline void addIRInstruction(ir_instruction*, bool global = false);

        // Return ref count of an rvalue
        inline unsigned getRefCount(const llvm::Value*) const;

        // Return true if we have a valueMap entry for this LLVM value
        inline bool valueEntryExists(const llvm::Value*) const;

        // Encapsulate creation of variables.  Ideally the int would be
        // ir_variable_mode, but we can't forward declare an enum until C++11
        inline ir_variable* newIRVariable(const glsl_type*, const char*, int mode, bool declare = false);
        inline ir_variable* newIRVariable(const glsl_type*, const std::string&, int mode, bool declare = false);
        inline ir_variable* newIRVariable(const llvm::Type* type, const llvm::Value*, const char*, int mode, bool declare = false);
        inline ir_variable* newIRVariable(const llvm::Type* type, const llvm::Value*, const std::string&, int mode, bool declare = false);

        // Encapsulate creation of variable dereference.  Ideally the int would be
        // ir_variable_mode, but we can't forward declare an enum until C++11
        inline ir_dereference* newIRVariableDeref(const glsl_type*, const char*, int mode, bool declare = false);
        inline ir_dereference* newIRVariableDeref(const glsl_type*, const std::string&, int mode, bool declare = false);
        inline ir_dereference* newIRVariableDeref(const llvm::Type* type, const llvm::Value*, const char*, int mode, bool declare = false);
        inline ir_dereference* newIRVariableDeref(const llvm::Type* type, const llvm::Value*, const std::string&, int mode, bool declare = false);

        // Fix up IR Lvalues (see comment in C++ code)
        ir_instruction* fixIRLValue(ir_rvalue* lhs, ir_rvalue* rhs);

        // Add error message
        void error(const char* msg) const;

        void setIoParameters(ir_variable* ioVar, const llvm::MDNode*, bool isOutput);

        // Data ----------------------------------------------------------------

        // Map LLVM values to IR values
        // We never need an ordered traversal.  Use unordered map for performance.
        typedef std::tr1::unordered_map<const llvm::Value*, ir_instruction*> tValueMap;
        tValueMap valueMap;

        // structure to count rvalues per lvalue
        typedef std::tr1::unordered_map<const llvm::Value*, unsigned> tRefCountMap;
        tRefCountMap refCountMap;

        // map from type names to the mdAggregate nodes that describe their types
        typedef std::tr1::unordered_map<const llvm::Type*, const llvm::MDNode*> tMDMap;
        tMDMap typeMdAggregateMap;

        // Map from type names to metadata nodes
        typedef std::tr1::unordered_map<std::string, const llvm::MDNode*> tTypenameMdMap;
        tTypenameMdMap typenameMdMap;

        typedef std::pair<const llvm::Type*, const llvm::MDNode*> tTypeData;

        struct TypePairHash {
           size_t operator()(const tTypeData& p) const { return size_t(p.first) ^ size_t(p.second); }
        };

        // map to track mapping from LLVM to HIR types
        typedef std::tr1::unordered_map<tTypeData, const glsl_type*, TypePairHash> tTypeMap;
        tTypeMap typeMap;

        // This is the declaration map.  We map uses of globals to their declarations
        // using rendezvous-by-name.  HIR requires use of the exact ir_variable node.
        typedef std::tr1::unordered_map<std::string, ir_variable*> tGlobalDeclMap;
        tGlobalDeclMap globalDeclMap;

        // For globals, we remember an ir_variable_mode from the global declaration.
        // Alas, we store it as an int here since we can't forward declare an enum.
        typedef std::tr1::unordered_map<std::string, int> tGlobalVarModeMap;
        tGlobalVarModeMap globalVarModeMap;

        // Certain HIR opcodes require proper sint/uint types, and that information is
        // not preserved in LLVM.  LunarGlass provides it in metadata for IO and uniform
        // data, but not arbitrary temp values.  This set tracks temporaries that must
        // be sints.
        typedef std::tr1::unordered_set<const llvm::Value*> tSintSet;
        tSintSet sintValues;

        // These are anonymous structs where we must hoist the members to the global scope,
        // because HIR form doesn't match BottomIR form.  BottomIR holds them in a structure,
        // while HIR makes each mmeber a globally scoped variable.  This is awkward.
        std::tr1::unordered_set<std::string> anonBlocks;

        // Stack of instruction lists for creation of HIR tree
        std::vector<exec_list *> instructionStack;

        // Stack of if statements.  Used to find else clause instruction list
        std::vector<ir_if *> ifStack;

        // Stack of loop statements.
        std::vector<ir_loop *> loopStack;

        // Stack of loop terminator statements (e.g, for handling ++index).
        std::vector<ir_instruction *> loopTerminatorStack;

        // Global initializers, etc we'll add as a main() prologue
        std::list<ir_instruction *> prologue;

        // We count the references of each lvalue, to know when to generate
        // assignments and when to directly create tree nodes
        void countReferences(const llvm::Module&);

        // Make up a name for a new variable
        const char* newName(const llvm::Value*);

        // list of llvm Values to free on exit
        std::vector<const llvm::Value*> toDelete;

        mutable std::string infoLog;

        // Function translation state: we need these because the traversal of LLVM
        // functions is distributed across multiple methods.
        const glsl_type*         fnReturnType;
        const char*              fnName;
        ir_function*             fnFunction;
        ir_function_signature*   fnSignature;
        exec_list                fnParameters;

        // Mesa state and context
        _mesa_glsl_parse_state*  state;
        gl_shader*               shader;
        gl_context*              ctx;
    }; // class MesaGlassTranslator
} // namespace gla

