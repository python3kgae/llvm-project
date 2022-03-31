//===--- DirectX.cpp - DirectX ToolChain Implementations ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DirectX.h"
#include "llvm/ADT/Triple.h"
#include "CommonArgs.h"
#include "clang/Driver/DriverDiagnostic.h"

using namespace clang::driver;
using namespace clang::driver::tools;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;
using namespace llvm;

namespace {
std::string tryParseProfile(StringRef profile) {
  // [ps|vs|gs|hs|ds|cs|ms|as]_[major]_[minor]
  Triple::EnvironmentType kind;
  switch (profile[0]) {
  case 'p':
    kind = Triple::EnvironmentType::Pixel;
    break;
  case 'v':
    kind = Triple::EnvironmentType::Vertex;
    break;
  case 'g':
    kind = Triple::EnvironmentType::Geometry;
    break;
  case 'h':
    kind = Triple::EnvironmentType::Hull;
    break;
  case 'd':
    kind = Triple::EnvironmentType::Domain;
    break;
  case 'c':
    kind = Triple::EnvironmentType::Compute;
    break;
  case 'l':
    kind = Triple::EnvironmentType::Library;
    break;
  case 'm':
    kind = Triple::EnvironmentType::Mesh;
    break;
  case 'a':
    kind = Triple::EnvironmentType::Amplification;
    break;
  default:
    return "";
  }
  unsigned Idx = 3;
  if (kind != Triple::EnvironmentType::Library) {
    if (profile[1] != 's' || profile[2] != '_')
      return "";
  } else {
    if (profile[1] != 'i' || profile[2] != 'b' || profile[3] != '_')
      return "";
    Idx = 4;
  }
  Triple::OSType::ShaderModel;
  unsigned Major;
  switch (profile[Idx++]) {
  case '4':
    Major = 4;
    break;
  case '5':
    Major = 5;
    break;
  case '6':
    Major = 6;
    break;
  default:
    return "";
  }
  if (profile[Idx++] != '_')
    return "";

  static const unsigned kOfflineMinor = 0xF;
  unsigned Minor;
  switch (profile[Idx++]) {
  case '0':
    Minor = 0;
    break;
  case '1':
    Minor = 1;
    break;
  case '2':
    if (Major == 6) {
      Minor = 2;
      break;
    } else
      return "";
  case '3':
    if (Major == 6) {
      Minor = 3;
      break;
    } else
      return "";
  case '4':
    if (Major == 6) {
      Minor = 4;
      break;
    } else
      return "";
  case '5':
    if (Major == 6) {
      Minor = 5;
      break;
    } else
      "";
  case '6':
    if (Major == 6) {
      Minor = 6;
      break;
    } else
      return "";
  case '7':
    if (Major == 6) {
      Minor = 7;
      break;
    } else
      return "";
  case 'x':
    if (kind == Triple::EnvironmentType::Library && Major == 6) {
      Minor = kOfflineMinor;
      break;
    } else
      return "";
  default:
    return "";
  }
  if (profile.size() != Idx && profile[Idx++] != 0)
    return "";
  // dxil-unknown-shadermodel-hull
  llvm::Triple T(Twine("dxil"), Twine("unknown"),
                 Twine("shadermodel")
                     .concat(Twine(Major))
                     .concat(".")
                     .concat(Twine(Minor)),
                 Triple::getEnvironmentTypeName(kind));
  return T.getTriple();
}

} // namespace

/// DirectX Toolchain
DirectXToolChain::DirectXToolChain(const Driver &D, const llvm::Triple &Triple,
                                   const ArgList &Args)
    : ToolChain(D, Triple, Args) {}

std::string
DirectXToolChain::ComputeEffectiveClangTriple(const ArgList &Args,
                                              types::ID InputType) const {
  if (Arg *A = Args.getLastArg(options::OPT_target_profile)) {
    StringRef profile = A->getValue();
    std::string triple = tryParseProfile(profile);
    if (triple == "") {
      getDriver().Diag(diag::err_drv_invalid_directx_shader_module) << profile;
      triple = ToolChain::ComputeEffectiveClangTriple(Args, InputType);
    }
    A->claim();
    return triple;
  } else {
    return ToolChain::ComputeEffectiveClangTriple(Args, InputType);
  }
}
