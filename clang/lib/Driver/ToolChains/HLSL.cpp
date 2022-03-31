//===--- HLSL.cpp - HLSL ToolChain Implementations --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HLSL.h"
#include "CommonArgs.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"

using namespace clang::driver;
using namespace clang::driver::tools;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;
using namespace llvm;

namespace {

const unsigned OfflineLibMinor = 0xF;
const unsigned MaxShaderModel6Minor = 7;
bool isLegalShaderModel(Triple &T) {
  if (T.getOS() != Triple::OSType::ShaderModel)
    return false;

  auto Kind = T.getEnvironment();

  switch (Kind) {
  default:
    return false;
  case Triple::EnvironmentType::Vertex:
  case Triple::EnvironmentType::Hull:
  case Triple::EnvironmentType::Domain:
  case Triple::EnvironmentType::Geometry:
  case Triple::EnvironmentType::Pixel:
  case Triple::EnvironmentType::Compute:
  case Triple::EnvironmentType::Library:
  case Triple::EnvironmentType::Amplification:
  case Triple::EnvironmentType::Mesh:
    break;
  }

  auto Version = T.getOSVersion();
  if (Version.getBuild())
    return false;
  if (Version.getSubminor())
    return false;

  auto OMinor = Version.getMinor();
  if (!OMinor.hasValue())
    return false;

  unsigned Minor = OMinor.getValue();
  unsigned Major = Version.getMajor();

  switch (Major) {
  case 4:
  case 5: {
    switch (Minor) {
    case 0:
    case 1:
      switch (Kind) {
      case Triple::EnvironmentType::Vertex:
        break;
      default:
        return false;
      }
      break;
    default:
      return false;
    }
  } break;
  case 6: {
    switch (Kind) {
    default:
      break;
    case Triple::EnvironmentType::Library: {
      if (Minor < 3)
        return false;
    } break;
    case Triple::EnvironmentType::Amplification:
    case Triple::EnvironmentType::Mesh: {
      if (Minor < 5)
        return false;
    } break;
    }
    if (Minor == OfflineLibMinor) {
      if (Kind != Triple::EnvironmentType::Library)
        return false;
    } else if (Minor > MaxShaderModel6Minor) {
      return false;
    }
  } break;
  default:
    return false;
  }

  return true;
}

std::string tryParseProfile(StringRef Profile) {
  // [ps|vs|gs|hs|ds|cs|ms|as]_[major]_[minor]
  SmallVector<StringRef, 3> Parts;
  Profile.split(Parts, "_");
  if (Parts.size() != 3)
    return "";

  Triple::EnvironmentType Kind =
      StringSwitch<Triple::EnvironmentType>(Parts[0])
          .Case("ps", Triple::EnvironmentType::Pixel)
          .Case("vs", Triple::EnvironmentType::Vertex)
          .Case("gs", Triple::EnvironmentType::Geometry)
          .Case("hs", Triple::EnvironmentType::Hull)
          .Case("ds", Triple::EnvironmentType::Domain)
          .Case("cs", Triple::EnvironmentType::Compute)
          .Case("lib", Triple::EnvironmentType::Library)
          .Case("ms", Triple::EnvironmentType::Mesh)
          .Case("as", Triple::EnvironmentType::Amplification)
          .Default(Triple::EnvironmentType::UnknownEnvironment);
  if (Kind == Triple::EnvironmentType::UnknownEnvironment)
    return "";

  unsigned Major = StringSwitch<unsigned>(Parts[1])
                       .Case("4", 4)
                       .Case("5", 5)
                       .Case("6", 6)
                       .Default(0);
  if (Major == 0)
    return "";

  const unsigned InvalidMinor = -1;
  unsigned Minor = StringSwitch<unsigned>(Parts[2])
                       .Case("0", 0)
                       .Case("1", 1)
                       .Case("2", 2)
                       .Case("3", 3)
                       .Case("4", 4)
                       .Case("5", 5)
                       .Case("6", 6)
                       .Case("7", 7)
                       .Case("x", OfflineLibMinor)
                       .Default(InvalidMinor);
  if (Minor == InvalidMinor)
    return "";

  // dxil-unknown-shadermodel-hull
  llvm::Triple T;
  T.setArch(Triple::ArchType::dxil);
  T.setOSName(Triple::getOSTypeName(Triple::OSType::ShaderModel).str() +
              VersionTuple(Major, Minor).getAsString());
  T.setEnvironment(Kind);
  if (isLegalShaderModel(T))
    return T.getTriple();
  else
    return "";
}

} // namespace

/// DirectX Toolchain
HLSLToolChain::HLSLToolChain(const Driver &D, const llvm::Triple &Triple,
                             const ArgList &Args)
    : ToolChain(D, Triple, Args) {}

std::string
HLSLToolChain::ComputeEffectiveClangTriple(const ArgList &Args,
                                           types::ID InputType) const {
  if (Arg *A = Args.getLastArg(options::OPT_target_profile)) {
    StringRef Profile = A->getValue();
    std::string Triple = tryParseProfile(Profile);
    if (Triple == "") {
      getDriver().Diag(diag::err_drv_invalid_directx_shader_module) << Profile;
      Triple = ToolChain::ComputeEffectiveClangTriple(Args, InputType);
    }
    A->claim();
    return Triple;
  } else {
    return ToolChain::ComputeEffectiveClangTriple(Args, InputType);
  }
}
