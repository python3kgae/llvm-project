//===- DXILIntrinsicExpansion.cpp - Prepare LLVM Module for DXIL encoding--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file This file contains DXIL intrinsic expansions for those that don't have
//  opcodes in DirectX Intermediate Language (DXIL).
//===----------------------------------------------------------------------===//

#include "DXILIntrinsicExpansion.h"
#include "DirectX.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsDirectX.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

#define DEBUG_TYPE "dxil-intrinsic-expansion"

using namespace llvm;

static bool isIntrinsicExpansion(Function &F) {
  switch (F.getIntrinsicID()) {
  case Intrinsic::abs:
  case Intrinsic::exp:
  case Intrinsic::log:
  case Intrinsic::log10:
  case Intrinsic::pow:
  case Intrinsic::dx_any:
  case Intrinsic::dx_clamp:
  case Intrinsic::dx_uclamp:
  case Intrinsic::dx_lerp:
  case Intrinsic::dx_sdot:
  case Intrinsic::dx_udot:
    return true;
  }
  return false;
}

static bool expandAbs(CallInst *Orig) {
  Value *X = Orig->getOperand(0);
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);
  Type *Ty = X->getType();
  Type *EltTy = Ty->getScalarType();
  Constant *Zero = Ty->isVectorTy()
                       ? ConstantVector::getSplat(
                             ElementCount::getFixed(
                                 cast<FixedVectorType>(Ty)->getNumElements()),
                             ConstantInt::get(EltTy, 0))
                       : ConstantInt::get(EltTy, 0);
  auto *V = Builder.CreateSub(Zero, X);
  auto *MaxCall =
      Builder.CreateIntrinsic(Ty, Intrinsic::smax, {X, V}, nullptr, "dx.max");
  Orig->replaceAllUsesWith(MaxCall);
  Orig->eraseFromParent();
  return true;
}

static bool expandIntegerDot(CallInst *Orig, Intrinsic::ID DotIntrinsic) {
  assert(DotIntrinsic == Intrinsic::dx_sdot ||
         DotIntrinsic == Intrinsic::dx_udot);
  Intrinsic::ID MadIntrinsic = DotIntrinsic == Intrinsic::dx_sdot
                                   ? Intrinsic::dx_imad
                                   : Intrinsic::dx_umad;
  Value *A = Orig->getOperand(0);
  Value *B = Orig->getOperand(1);
  [[maybe_unused]] Type *ATy = A->getType();
  [[maybe_unused]] Type *BTy = B->getType();
  assert(ATy->isVectorTy() && BTy->isVectorTy());

  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);

  auto *AVec = dyn_cast<FixedVectorType>(A->getType());
  Value *Elt0 = Builder.CreateExtractElement(A, (uint64_t)0);
  Value *Elt1 = Builder.CreateExtractElement(B, (uint64_t)0);
  Value *Result = Builder.CreateMul(Elt0, Elt1);
  for (unsigned I = 1; I < AVec->getNumElements(); I++) {
    Elt0 = Builder.CreateExtractElement(A, I);
    Elt1 = Builder.CreateExtractElement(B, I);
    Result = Builder.CreateIntrinsic(Result->getType(), MadIntrinsic,
                                     ArrayRef<Value *>{Elt0, Elt1, Result},
                                     nullptr, "dx.mad");
  }
  Orig->replaceAllUsesWith(Result);
  Orig->eraseFromParent();
  return true;
}

static bool expandExpIntrinsic(CallInst *Orig) {
  Value *X = Orig->getOperand(0);
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);
  Type *Ty = X->getType();
  Type *EltTy = Ty->getScalarType();
  Constant *Log2eConst =
      Ty->isVectorTy() ? ConstantVector::getSplat(
                             ElementCount::getFixed(
                                 cast<FixedVectorType>(Ty)->getNumElements()),
                             ConstantFP::get(EltTy, numbers::log2ef))
                       : ConstantFP::get(EltTy, numbers::log2ef);
  Value *NewX = Builder.CreateFMul(Log2eConst, X);
  auto *Exp2Call =
      Builder.CreateIntrinsic(Ty, Intrinsic::exp2, {NewX}, nullptr, "dx.exp2");
  Exp2Call->setTailCall(Orig->isTailCall());
  Exp2Call->setAttributes(Orig->getAttributes());
  Orig->replaceAllUsesWith(Exp2Call);
  Orig->eraseFromParent();
  return true;
}

static bool expandAnyIntrinsic(CallInst *Orig) {
  Value *X = Orig->getOperand(0);
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);
  Type *Ty = X->getType();
  Type *EltTy = Ty->getScalarType();

  if (!Ty->isVectorTy()) {
    Value *Cond = EltTy->isFloatingPointTy()
                      ? Builder.CreateFCmpUNE(X, ConstantFP::get(EltTy, 0))
                      : Builder.CreateICmpNE(X, ConstantInt::get(EltTy, 0));
    Orig->replaceAllUsesWith(Cond);
  } else {
    auto *XVec = dyn_cast<FixedVectorType>(Ty);
    Value *Cond =
        EltTy->isFloatingPointTy()
            ? Builder.CreateFCmpUNE(
                  X, ConstantVector::getSplat(
                         ElementCount::getFixed(XVec->getNumElements()),
                         ConstantFP::get(EltTy, 0)))
            : Builder.CreateICmpNE(
                  X, ConstantVector::getSplat(
                         ElementCount::getFixed(XVec->getNumElements()),
                         ConstantInt::get(EltTy, 0)));
    Value *Result = Builder.CreateExtractElement(Cond, (uint64_t)0);
    for (unsigned I = 1; I < XVec->getNumElements(); I++) {
      Value *Elt = Builder.CreateExtractElement(Cond, I);
      Result = Builder.CreateOr(Result, Elt);
    }
    Orig->replaceAllUsesWith(Result);
  }
  Orig->eraseFromParent();
  return true;
}

static bool expandLerpIntrinsic(CallInst *Orig) {
  Value *X = Orig->getOperand(0);
  Value *Y = Orig->getOperand(1);
  Value *S = Orig->getOperand(2);
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);
  auto *V = Builder.CreateFSub(Y, X);
  V = Builder.CreateFMul(S, V);
  auto *Result = Builder.CreateFAdd(X, V, "dx.lerp");
  Orig->replaceAllUsesWith(Result);
  Orig->eraseFromParent();
  return true;
}

static bool expandLogIntrinsic(CallInst *Orig,
                               float LogConstVal = numbers::ln2f) {
  Value *X = Orig->getOperand(0);
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);
  Type *Ty = X->getType();
  Type *EltTy = Ty->getScalarType();
  Constant *Ln2Const =
      Ty->isVectorTy() ? ConstantVector::getSplat(
                             ElementCount::getFixed(
                                 cast<FixedVectorType>(Ty)->getNumElements()),
                             ConstantFP::get(EltTy, LogConstVal))
                       : ConstantFP::get(EltTy, LogConstVal);
  auto *Log2Call =
      Builder.CreateIntrinsic(Ty, Intrinsic::log2, {X}, nullptr, "elt.log2");
  Log2Call->setTailCall(Orig->isTailCall());
  Log2Call->setAttributes(Orig->getAttributes());
  auto *Result = Builder.CreateFMul(Ln2Const, Log2Call);
  Orig->replaceAllUsesWith(Result);
  Orig->eraseFromParent();
  return true;
}
static bool expandLog10Intrinsic(CallInst *Orig) {
  return expandLogIntrinsic(Orig, numbers::ln2f / numbers::ln10f);
}

static bool expandPowIntrinsic(CallInst *Orig) {

  Value *X = Orig->getOperand(0);
  Value *Y = Orig->getOperand(1);
  Type *Ty = X->getType();
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);

  auto *Log2Call =
      Builder.CreateIntrinsic(Ty, Intrinsic::log2, {X}, nullptr, "elt.log2");
  auto *Mul = Builder.CreateFMul(Log2Call, Y);
  auto *Exp2Call =
      Builder.CreateIntrinsic(Ty, Intrinsic::exp2, {Mul}, nullptr, "elt.exp2");
  Exp2Call->setTailCall(Orig->isTailCall());
  Exp2Call->setAttributes(Orig->getAttributes());
  Orig->replaceAllUsesWith(Exp2Call);
  Orig->eraseFromParent();
  return true;
}

static Intrinsic::ID getMaxForClamp(Type *ElemTy,
                                    Intrinsic::ID ClampIntrinsic) {
  if (ClampIntrinsic == Intrinsic::dx_uclamp)
    return Intrinsic::umax;
  assert(ClampIntrinsic == Intrinsic::dx_clamp);
  if (ElemTy->isVectorTy())
    ElemTy = ElemTy->getScalarType();
  if (ElemTy->isIntegerTy())
    return Intrinsic::smax;
  assert(ElemTy->isFloatingPointTy());
  return Intrinsic::maxnum;
}

static Intrinsic::ID getMinForClamp(Type *ElemTy,
                                    Intrinsic::ID ClampIntrinsic) {
  if (ClampIntrinsic == Intrinsic::dx_uclamp)
    return Intrinsic::umin;
  assert(ClampIntrinsic == Intrinsic::dx_clamp);
  if (ElemTy->isVectorTy())
    ElemTy = ElemTy->getScalarType();
  if (ElemTy->isIntegerTy())
    return Intrinsic::smin;
  assert(ElemTy->isFloatingPointTy());
  return Intrinsic::minnum;
}

static bool expandClampIntrinsic(CallInst *Orig, Intrinsic::ID ClampIntrinsic) {
  Value *X = Orig->getOperand(0);
  Value *Min = Orig->getOperand(1);
  Value *Max = Orig->getOperand(2);
  Type *Ty = X->getType();
  IRBuilder<> Builder(Orig->getParent());
  Builder.SetInsertPoint(Orig);
  auto *MaxCall = Builder.CreateIntrinsic(
      Ty, getMaxForClamp(Ty, ClampIntrinsic), {X, Min}, nullptr, "dx.max");
  auto *MinCall =
      Builder.CreateIntrinsic(Ty, getMinForClamp(Ty, ClampIntrinsic),
                              {MaxCall, Max}, nullptr, "dx.min");

  Orig->replaceAllUsesWith(MinCall);
  Orig->eraseFromParent();
  return true;
}

static bool expandIntrinsic(Function &F, CallInst *Orig) {
  switch (F.getIntrinsicID()) {
  case Intrinsic::abs:
    return expandAbs(Orig);
  case Intrinsic::exp:
    return expandExpIntrinsic(Orig);
  case Intrinsic::log:
    return expandLogIntrinsic(Orig);
  case Intrinsic::log10:
    return expandLog10Intrinsic(Orig);
  case Intrinsic::pow:
    return expandPowIntrinsic(Orig);
  case Intrinsic::dx_any:
    return expandAnyIntrinsic(Orig);
  case Intrinsic::dx_uclamp:
  case Intrinsic::dx_clamp:
    return expandClampIntrinsic(Orig, F.getIntrinsicID());
  case Intrinsic::dx_lerp:
    return expandLerpIntrinsic(Orig);
  case Intrinsic::dx_sdot:
  case Intrinsic::dx_udot:
    return expandIntegerDot(Orig, F.getIntrinsicID());
  }
  return false;
}

static GlobalVariable *getOrInsertExtIntGlobalVariable(Module &M, StringRef Name) {
  if (auto *GV = M.getGlobalVariable(Name))
    return GV;

  auto *GV = new GlobalVariable(
      M, Type::getInt32Ty(M.getContext()), false, GlobalValue::ExternalLinkage,
      ConstantInt::get(Type::getInt32Ty(M.getContext()), 0), Name);
  GV->setDSOLocal(true);
  return GV;
}

static bool expansionIntrinsics(Module &M) {
  for (auto &F : make_early_inc_range(M.functions())) {
    if (!isIntrinsicExpansion(F))
      continue;
    bool IntrinsicExpanded = false;
    for (User *U : make_early_inc_range(F.users())) {
      auto *IntrinsicCall = dyn_cast<CallInst>(U);
      if (!IntrinsicCall)
        continue;
      IntrinsicExpanded = expandIntrinsic(F, IntrinsicCall);
    }
    if (F.user_empty() && IntrinsicExpanded)
      F.eraseFromParent();
  }

  SmallVector<std::pair<Function *, bool>> DiffFns;

  for (auto &F : make_early_inc_range(M.functions())) {
    bool IsFwdDiff = F.getName().starts_with("??$fwddiff@");
    bool IsAutoDiff = F.getName().starts_with("??$autodiff@");
    if (!IsFwdDiff && !IsAutoDiff)
      continue;

    if (F.user_empty()) {
      F.eraseFromParent();
      continue;
    }
    DiffFns.emplace_back(std::make_pair(&F, IsAutoDiff));

  }

  for (auto &[F, IsAutoDiff] : DiffFns) {
    SmallVector<CallInst *> Calls;
    for (User *U : make_early_inc_range(F->users())) {

      CallInst *CI = cast<CallInst>(U);
      Function *Fn = dyn_cast<Function>(CI->getArgOperand(0));
      assert(Fn);

      // TODO: Flatten Fn if needed.

      SmallVector<Value *, 4> NewArgs;
      NewArgs.push_back(Fn);

      SmallVector<Value *, 4> Args(CI->arg_begin() + 1, CI->arg_end());
      IRBuilder<> B(CI);

      SmallVector<Type *> OutTypes;
      SmallVector<Value *> OutPtrs;

      std::string EnzymeFnName =
          IsAutoDiff ? "__enzyme_autodiff" : "__enzyme_fwddiff";

      for (Value *Arg : Args) {
        auto *AI = cast<AllocaInst>(Arg);
        Type *Ty = AI->getAllocatedType();
        StructType *ST = cast<StructType>(Ty);
        Type *EltTy = ST->getElementType(0);
        StringRef Name = ST->getName();

        // Mangle the function name.
        EnzymeFnName =
            EnzymeFnName + "_" + Name.str();

        GlobalVariable *EnzymeMode = nullptr;
        // Create global variable for
        //    extern int enzyme_dup;
        //    extern int enzyme_dupnoneed;
        //    extern int enzyme_out;
        //    extern int enzyme_const;

        // TODO: assert in/inout/out with DIFFE_TYPE.

        if (Name.starts_with("struct.hlsl::Duplicated")) {
          EnzymeMode = getOrInsertExtIntGlobalVariable(M, "enzyme_dup");
          NewArgs.push_back(EnzymeMode);

          Value *PtrP =
              B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(0)});
          Value *P = B.CreateLoad(EltTy, PtrP);
          NewArgs.push_back(P);

          Value *PtrD = B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(1)});
          Value *D = B.CreateLoad(EltTy, PtrD);
          NewArgs.push_back(D);
        } else if (Name.starts_with("struct.hlsl::DuplicatedNoNeed")) {
          EnzymeMode = getOrInsertExtIntGlobalVariable(M, "enzyme_dupnoneed");
          NewArgs.push_back(EnzymeMode);

          Value *PtrP = B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(0)});
          Value *P = B.CreateLoad(EltTy, PtrP);
          NewArgs.push_back(P);

          Value *PtrD = B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(1)});
          Value *D = B.CreateLoad(EltTy, PtrD);
          NewArgs.push_back(D);
        } else if (Name.starts_with("struct.hlsl::Active")) {
          assert(IsAutoDiff && "Active mode is only for autodiff");
          EnzymeMode = getOrInsertExtIntGlobalVariable(M, "enzyme_out");
          NewArgs.push_back(EnzymeMode);

          Value *PtrP = B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(0)});
          Value *P = B.CreateLoad(EltTy, PtrP);
          NewArgs.push_back(P);

          // Save type for the output.
          OutTypes.push_back(ST->getElementType(0));

          // Save the pointer for the output.
          Value *PtrD =
              B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(1)});
          OutPtrs.push_back(PtrD);

        } else if (Name.starts_with("struct.hlsl::Const")) {
          EnzymeMode = getOrInsertExtIntGlobalVariable(M, "enzyme_const");

          NewArgs.push_back(EnzymeMode);
          Value *PtrP = B.CreateGEP(ST, Arg, {B.getInt32(0), B.getInt32(0)});
          Value *P = B.CreateLoad(EltTy, PtrP);
          NewArgs.push_back(P);

        } else {
          llvm_unreachable("Unknown Enzyme mode");
        }
      }

      SmallVector<Type *, 4> NewArgTys;
      for (Value *Arg : NewArgs) {
        NewArgTys.push_back(Arg->getType());
      }

      Type *RetTy = Fn->getReturnType();
      if (IsAutoDiff) {
        RetTy = StructType::get(M.getContext(), OutTypes);
      }
      // TODO: add retTy to EnzymeFnName.
      FunctionType *FT = FunctionType::get(RetTy, NewArgTys, false);
      
      FunctionCallee EnzymeFn = M.getOrInsertFunction(EnzymeFnName, FT);

      CallInst *NewCI = B.CreateCall(EnzymeFn, NewArgs);

      // Replace CI with NewCI.
      // Link NewCI result with Active.B;
      for (unsigned I = 0; I < OutPtrs.size(); I++) {
        Value *Ptr = OutPtrs[I];
        Value *Val = B.CreateExtractValue(NewCI, I);
        B.CreateStore(Val, Ptr);
      }

      if (!IsAutoDiff)
        CI->replaceAllUsesWith(NewCI);

      CI->eraseFromParent();
      //Calls.push_back(CI);
      // Lower fwddiff into __enzyme_fwddiff.
      //       autodiff into __enzyme_fwddiff.
      // translate argument and parameter.
      // Duplicated into enzyme_dup,a.x, a.y.
      // Const into enzyme_const, a.x.
      // Active into enzyme_out, a.x.
      // DuplicatedNoNeed, enzyme_dupnoneed, a.x, a.y.
      // reference arg need to change to ref.
      // non-reference arg just use value?
    }

    //for (CallInst *CI : Calls) {
    //  CI->eraseFromParent();
    //}


    F->eraseFromParent();
  }

  return true;
}

PreservedAnalyses DXILIntrinsicExpansion::run(Module &M,
                                              ModuleAnalysisManager &) {
  if (expansionIntrinsics(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}

bool DXILIntrinsicExpansionLegacy::runOnModule(Module &M) {
  return expansionIntrinsics(M);
}

char DXILIntrinsicExpansionLegacy::ID = 0;

INITIALIZE_PASS_BEGIN(DXILIntrinsicExpansionLegacy, DEBUG_TYPE,
                      "DXIL Intrinsic Expansion", false, false)
INITIALIZE_PASS_END(DXILIntrinsicExpansionLegacy, DEBUG_TYPE,
                    "DXIL Intrinsic Expansion", false, false)

ModulePass *llvm::createDXILIntrinsicExpansionLegacyPass() {
  return new DXILIntrinsicExpansionLegacy();
}
