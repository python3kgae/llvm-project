//===- DXILMetadataResourceAnalysis.h   - DXIL Metadata Resource analysis--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file This file contains Analysis which collect DXIL resources from DXIL
/// metadata.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_DIRECTX_DXILMETADATARESOURCEANALYSIS_H
#define LLVM_LIB_TARGET_DIRECTX_DXILMETADATARESOURCEANALYSIS_H

#include "DXILResource.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <memory>

namespace llvm {
/// Analysis pass that exposes the \c DXILResource for a module from dxil
/// metadata.
class DXILMetadataResourceAnalysis
    : public AnalysisInfoMixin<DXILMetadataResourceAnalysis> {
  friend AnalysisInfoMixin<DXILMetadataResourceAnalysis>;
  static AnalysisKey Key;

public:
  typedef dxil::Resources Result;
  dxil::Resources run(Module &M, ModuleAnalysisManager &AM);
};

/// Printer pass for the \c DXILMetadataResourceAnalysis results.
class DXILMetadataResourcePrinterPass
    : public PassInfoMixin<DXILMetadataResourcePrinterPass> {
  raw_ostream &OS;

public:
  explicit DXILMetadataResourcePrinterPass(raw_ostream &OS) : OS(OS) {}
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

/// The legacy pass manager's analysis pass to compute DXIL resource
/// information from DXIL metadata.
class DXILMetadataResourceWrapper : public ModulePass {
  dxil::Resources Resources;

public:
  static char ID; // Pass identification, replacement for typeid

  DXILMetadataResourceWrapper();

  dxil::Resources &getDXILResource() { return Resources; }
  const dxil::Resources &getDXILResource() const { return Resources; }

  /// Calculate the DXILResource for the module.
  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }
  void print(raw_ostream &O, const Module *M = nullptr) const override;
};
} // namespace llvm

#endif
