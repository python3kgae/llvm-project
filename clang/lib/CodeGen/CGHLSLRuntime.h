//===----- CGHLSLRuntime.h - Interface to HLSL Runtimes -----*- C++ -*-===//
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

#pragma once

namespace clang {

namespace CodeGen {

class CodeGenModule;

class CGHLSLRuntime {
protected:
  CodeGenModule &CGM;

public:
  CGHLSLRuntime(CodeGenModule &CGM);
  virtual ~CGHLSLRuntime();
  void finishCodeGen();

};

}
}
