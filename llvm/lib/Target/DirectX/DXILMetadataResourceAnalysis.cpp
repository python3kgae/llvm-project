//===- DXILMetadataResourceAnalysis.cpp - DXIL Metadata Resource analysis--===//
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

#include "DXILMetadataResourceAnalysis.h"
#include "DirectX.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

#define DEBUG_TYPE "dxil-metadata-resource-analysis"

dxil::Resources DXILMetadataResourceAnalysis::run(Module &M,
                                                  ModuleAnalysisManager &AM) {
  dxil::Resources R;
  R.collectFromDXILMetadata(M);
  return R;
}

AnalysisKey DXILMetadataResourceAnalysis::Key;

PreservedAnalyses
DXILMetadataResourcePrinterPass::run(Module &M, ModuleAnalysisManager &AM) {
  dxil::Resources Res = AM.getResult<DXILMetadataResourceAnalysis>(M);
  Res.print(OS);
  return PreservedAnalyses::all();
}

char DXILMetadataResourceWrapper::ID = 0;
INITIALIZE_PASS_BEGIN(DXILMetadataResourceWrapper, DEBUG_TYPE,
                      "DXIL resource Information from metadata", true, true)
INITIALIZE_PASS_END(DXILMetadataResourceWrapper, DEBUG_TYPE,
                    "DXIL resource Information from metadata", true, true)

bool DXILMetadataResourceWrapper::runOnModule(Module &M) {
  Resources.collectFromDXILMetadata(M);
  return false;
}

DXILMetadataResourceWrapper::DXILMetadataResourceWrapper() : ModulePass(ID) {}

void DXILMetadataResourceWrapper::print(raw_ostream &OS, const Module *) const {
  Resources.print(OS);
}
