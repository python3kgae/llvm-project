//===----- CGHLSLRuntime.cpp - Interface to HLSL Runtimes -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This provides an abstract class for HLSL code generation.  Concrete
// subclasses of this implement code generation for specific HLSL
// runtime libraries.
//
//===----------------------------------------------------------------------===//

#include "CGHLSLRuntime.h"
#include "CodeGenModule.h"
#include "clang/Basic/CodeGenOptions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

using namespace clang;
using namespace CodeGen;
using namespace llvm;

namespace {
void addDxilValVersion(StringRef ValVersionStr, llvm::Module &M) {
  // The validation of ValVersionStr is done at HLSLToolChain::TranslateArgs.
  // Assume ValVersionStr is legal here.
  auto VerPair = ValVersionStr.split(".");
  llvm::APInt APMajor, APMinor;

  if (VerPair.first.getAsInteger(0, APMajor) ||
      VerPair.second.getAsInteger(0, APMinor)) {
    return;
  }
  uint64_t Major = APMajor.getLimitedValue();
  uint64_t Minor = APMinor.getLimitedValue();
  auto &Ctx = M.getContext();
  IRBuilder<> B(M.getContext());
  MDNode *Val = MDNode::get(Ctx, {ConstantAsMetadata::get(B.getInt32(Major)),
                                  ConstantAsMetadata::get(B.getInt32(Minor))});
  StringRef DxilValKey = "dx.valver";
  M.addModuleFlag(llvm::Module::ModFlagBehavior::AppendUnique, DxilValKey, Val);
}
} // namespace

void CGHLSLRuntime::finishCodeGen() {
  auto &CGOpts = CGM.getCodeGenOpts();
  llvm::Module &M = CGM.getModule();
  addDxilValVersion(CGOpts.DxilValidatorVersion, M);
}