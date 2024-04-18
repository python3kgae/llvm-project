//===- HLSLRootSignature.cpp - HLSL Root Signature objects ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file This file contains helper objects for HLSL root signature.
///
//===----------------------------------------------------------------------===//

#include "llvm/Frontend/HLSL/HLSLRootSignature.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace llvm {

namespace hlsl {
StringRef printRootSignature(const ParsedRootSignature &RS) {
  std::string TempString;
  raw_string_ostream OS(TempString);
  // TODO: Implement this.
  OS.flush();
  return TempString;
}

llvm::SmallVector<uint32_t, 64>
serializeRootSignature(const ParsedRootSignature &RS) {
  // TODO: Implement this.
  llvm::SmallVector<uint32_t, 64> Data;
  return Data;
}

void deserializeRootSignature(const llvm::SmallVectorImpl<uint32_t> &Data,
                              ParsedRootSignature &RS) {
  // TODO: Implement this.
}

} // namespace hlsl

} // namespace llvm
