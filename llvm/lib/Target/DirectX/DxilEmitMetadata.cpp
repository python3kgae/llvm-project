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
const StringRef DxilVerKey = "dx.version";
const unsigned DxilVersionNumFields = 2;
const unsigned DxilVersionMajorIdx = 0; // DXIL version major.
const unsigned DxilVersionMinorIdx = 1; // DXIL version minor.

const unsigned OfflineLibMinor = 0xF;
const unsigned MaxDXILMinor = 7;

void emitVersion(NamedMDNode *MD, VersionTuple &Ver, LLVMContext &Ctx) {
  Metadata *MDVals[DxilVersionNumFields];
  MDVals[DxilVersionMajorIdx] = Uint32ToConstMD(Ver.getMajor(), Ctx);
  MDVals[DxilVersionMinorIdx] =
      Uint32ToConstMD(Ver.getMinor().getValueOr(0), Ctx);

  MD->addOperand(MDNode::get(Ctx, MDVals));
}

void emitDxilVersion(Module &M, StringRef Key, VersionTuple &DxilVer) {
  NamedMDNode *DxilVersionMD = M.getNamedMetadata(Key);
  // Clear if already exist.
  if (DxilVersionMD)
    M.eraseNamedMetadata(DxilVersionMD);

  DxilVersionMD = M.getOrInsertNamedMetadata(Key);

  auto &Ctx = M.getContext();
  emitVersion(DxilVersionMD, DxilVer, Ctx);
}

VersionTuple loadDxilValidatorVersion(MDNode *ValVerMD) {
  if (ValVerMD->getNumOperands() != DxilVersionNumFields)
    return VersionTuple();

  unsigned Major = ConstMDToUint32(ValVerMD->getOperand(DxilVersionMajorIdx));
  unsigned Minor = ConstMDToUint32(ValVerMD->getOperand(DxilVersionMinorIdx));
  return VersionTuple(Major, Minor);
}

VersionTuple getDxilVersion(VersionTuple &ShaderModel) {
  unsigned Major = 1;
  unsigned Minor = ShaderModel.getMinor().getValueOr(0);
  if (Minor == OfflineLibMinor) {
    Minor = MaxDXILMinor;
  }
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
  explicit DxilEmitMetadata() : ModulePass(ID) {}

  StringRef getPassName() const override { return "HLSL DXIL Metadata Emit"; }

  bool runOnModule(Module &M) override;

private:
  void emitDXILVersion(Module &M);
};

bool DxilEmitMetadata::runOnModule(Module &M) {
  Triple Triple(M.getTargetTriple());
  VersionTuple ShaderModel = Triple.getOSVersion();
  VersionTuple DxilVer = getDxilVersion(ShaderModel);
  emitDxilVersion(M, DxilVerKey, DxilVer);
  VersionTuple ValidatorVer(1, 0);
  if (MDNode *ValVerMD = cast_or_null<MDNode>(M.getModuleFlag(ValVerKey))) {
    auto ValVer = loadDxilValidatorVersion(ValVerMD);
    if (!ValVer.empty())
      ValidatorVer = ValVer;
  }
  unsigned ShaderModelMinor = ShaderModel.getMinor().getValueOr(0);
  if (ShaderModelMinor == OfflineLibMinor) {
    ValidatorVer = VersionTuple(0, 0);
  } else if (ValidatorVer < DxilVer) {
    ValidatorVer = DxilVer;
  }
  emitDxilVersion(M, ValVerKey, ValidatorVer);
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
