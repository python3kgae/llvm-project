//===--- HLSLExternalSemaSource.h - HLSL Sema Source ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the HLSLExternalSemaSource interface.
//
//===----------------------------------------------------------------------===//
#ifndef CLANG_SEMA_HLSLEXTERNALSEMASOURCE_H
#define CLANG_SEMA_HLSLEXTERNALSEMASOURCE_H

#include "llvm/ADT/DenseMap.h"

#include "clang/Sema/ExternalSemaSource.h"
#include "clang/Sema/MultiplexExternalSemaSource.h"

namespace clang {
class NamespaceDecl;
class Sema;

class HLSLExternalSemaSource : public ExternalSemaSource {
  Sema *SemaPtr = nullptr;
  NamespaceDecl *HLSLNamespace;
  CXXRecordDecl *ResourceDecl;
  ExternalSemaSource *ExternalSema = nullptr;

  using CompletionFunction = std::function<void(CXXRecordDecl *)>;
  llvm::DenseMap<CXXRecordDecl *, CompletionFunction> Completions;

  void defineHLSLVectorAlias();
  void defineTrivialHLSLTypes();
  void forwardDeclareHLSLTypes();

  void completeBufferType(CXXRecordDecl *Record);

public:
  ~HLSLExternalSemaSource() override;

  /// Initialize the semantic source with the Sema instance
  /// being used to perform semantic analysis on the abstract syntax
  /// tree.
  void InitializeSema(Sema &S) override;

  /// Inform the semantic consumer that Sema is no longer available.
  void ForgetSema() override { SemaPtr = nullptr; }

  using ExternalASTSource::CompleteType;
  /// Complete an incomplete HLSL builtin type
  void CompleteType(TagDecl *Tag) override;
  void SetExternalSema(ExternalSemaSource *ExtSema) { ExternalSema = ExtSema; }
};

/// Members of ChainedHLSLExternalSemaSource, factored out so we can initialize
/// them before we initialize the ExternalSemaSource base class.
struct ChainedHLSLExternalSemaSourceMembers {
  ChainedHLSLExternalSemaSourceMembers(ExternalSemaSource *ExtSema)
      : ExternalSema(ExtSema) {
    HLSLSema.SetExternalSema(ExtSema);
  }
  HLSLExternalSemaSource HLSLSema;
  IntrusiveRefCntPtr<ExternalSemaSource> ExternalSema;
};

class ChainedHLSLExternalSemaSource
    : private ChainedHLSLExternalSemaSourceMembers,
      public MultiplexExternalSemaSource {
public:
  ChainedHLSLExternalSemaSource(ExternalSemaSource *ExtSema)
      : ChainedHLSLExternalSemaSourceMembers(ExtSema),
        // NOTE: HLSLSema after ExternalSema so HLSLSema initialize after
        // ExternalSema.
        // When initialize HLSLSema, reuse hlsl decls if already in
        //  ExternalSema.
        MultiplexExternalSemaSource(*ExternalSema.get(), HLSLSema) {}
};

} // namespace clang

#endif // CLANG_SEMA_HLSLEXTERNALSEMASOURCE_H
