//===- DxilEmitMetadata.cpp - Pass to emit dxil metadata --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "DirectX.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace {
unsigned ConstMDToUint32(const MDOperand &MDO) {
  ConstantInt *pConst = mdconst::extract<ConstantInt>(MDO);
  return (unsigned)pConst->getZExtValue();
}
ConstantAsMetadata *Uint32ToConstMD(unsigned v, LLVMContext &Ctx) {
  return ConstantAsMetadata::get(
      Constant::getIntegerValue(IntegerType::get(Ctx, 32), APInt(32, v)));
}
const StringRef ValVerKey = "dx.valver";
const unsigned DxilVersionNumFields = 2;
const unsigned DxilVersionMajorIdx = 0; // DXIL version major.
const unsigned DxilVersionMinorIdx = 1; // DXIL version minor.

void emitDxilValidatorVersion(Module &M, VersionTuple &ValidatorVer) {
  NamedMDNode *DxilValidatorVersionMD = M.getNamedMetadata(ValVerKey);

  // Allow re-writing the validator version, since this can be changed at
  // later points.
  if (DxilValidatorVersionMD)
    M.eraseNamedMetadata(DxilValidatorVersionMD);

  DxilValidatorVersionMD = M.getOrInsertNamedMetadata(ValVerKey);

  auto &Ctx = M.getContext();
  Metadata *MDVals[DxilVersionNumFields];
  MDVals[DxilVersionMajorIdx] = Uint32ToConstMD(ValidatorVer.getMajor(), Ctx);
  MDVals[DxilVersionMinorIdx] =
      Uint32ToConstMD(ValidatorVer.getMinor().getValueOr(0), Ctx);

  DxilValidatorVersionMD->addOperand(MDNode::get(Ctx, MDVals));
}

VersionTuple loadDxilValidatorVersion(MDNode *ValVerMD) {
  if (ValVerMD->getNumOperands() != DxilVersionNumFields)
    return VersionTuple();

  unsigned Major = ConstMDToUint32(ValVerMD->getOperand(DxilVersionMajorIdx));
  unsigned Minor = ConstMDToUint32(ValVerMD->getOperand(DxilVersionMinorIdx));
  return VersionTuple(Major, Minor);
}

void cleanModule(Module &M) {
  M.getOrInsertModuleFlagsMetadata()->eraseFromParent();
}
} // namespace

namespace {
class DxilEmitMetadata : public ModulePass {
public:
  static char ID; // Pass identification, replacement for typeid
  explicit DxilEmitMetadata() : ModulePass(ID), ValidatorVer(1, 0) {}

  StringRef getPassName() const override { return "HLSL DXIL Metadata Emit"; }

  bool runOnModule(Module &M) override;

private:
  VersionTuple ValidatorVer;
  void emitDXILVersion(Module &M);
};

bool DxilEmitMetadata::runOnModule(Module &M) {
  if (MDNode *ValVerMD = cast_or_null<MDNode>(M.getModuleFlag(ValVerKey))) {
    auto ValVer = loadDxilValidatorVersion(ValVerMD);
    if (!ValVer.empty())
      ValidatorVer = ValVer;
  }
  emitDxilValidatorVersion(M, ValidatorVer);
  cleanModule(M);
  return false;
}

void DxilEmitMetadata::emitDXILVersion(Module &M) {}

} // namespace

char DxilEmitMetadata::ID = 0;

ModulePass *llvm::createDxilEmitMetadataPass() {
  return new DxilEmitMetadata();
}

INITIALIZE_PASS(DxilEmitMetadata, "hlsl-dxilemit", "HLSL DXIL Metadata Emit",
                false, false)
