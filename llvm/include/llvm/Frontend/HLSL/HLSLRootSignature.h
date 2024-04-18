//===- HLSLRootSignature.h - HLSL Root Signature objects ------------------===//
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

#ifndef LLVM_FRONTEND_HLSL_HLSLROOTSIGNATURE_H
#define LLVM_FRONTEND_HLSL_HLSLROOTSIGNATURE_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <variant>

namespace llvm {

namespace hlsl {

namespace RootSignature {

// Constant values.
static const uint32_t DescriptorRangeOffsetAppend = 0xffffffff;
static const uint32_t SystemReservedRegisterSpaceValuesStart = 0xfffffff0;
static const uint32_t SystemReservedRegisterSpaceValuesEnd = 0xffffffff;
static const float MipLodBiasMax = 15.99f;
static const float MipLodBiasMin = -16.0f;
static const float Float32Max = 3.402823466e+38f;
static const uint32_t MipLodFractionalBitCount = 8;
static const uint32_t MapAnisotropy = 16;

// Enumerations and flags.
enum class DescriptorRangeFlags : uint32_t {
  None = 0,
  DescriptorsVolatile = 0x1,
  DataVolatile = 0x2,
  DataStaticWhileSetAtExecute = 0x4,
  DataStatic = 0x8,
  DescriptorsStaticKeepingBufferBoundsChecks = 0x10000,
  ValidFlags = 0x1000f,
  ValidSamplerFlags = DescriptorsVolatile
};
enum class DescriptorRangeType : uint32_t {
  SRV = 0,
  UAV = 1,
  CBV = 2,
  Sampler = 3,
  MaxValue = 3
};
enum class RootDescriptorFlags : uint32_t {
  None = 0,
  DataVolatile = 0x2,
  DataStaticWhileSetAtExecute = 0x4,
  DataStatic = 0x8,
  ValidFlags = 0xe
};
enum class RootSignatureVersion : uint32_t {
  Version_1 = 1,
  Version_1_0 = 1,
  Version_1_1 = 2
};

enum class RootSignatureCompilationFlags {
  None = 0x0,
  LocalRootSignature = 0x1,
  GlobalRootSignature = 0x2,
};

enum class RootSignatureFlags : uint32_t {
  None = 0,
  AllowInputAssemblerInputLayout = 0x1,
  DenyVertexShaderRootAccess = 0x2,
  DenyHullShaderRootAccess = 0x4,
  DenyDomainShaderRootAccess = 0x8,
  DenyGeometryShaderRootAccess = 0x10,
  DenyPixelShaderRootAccess = 0x20,
  AllowStreamOutput = 0x40,
  LocalRootSignature = 0x80,
  DenyAmplificationShaderRootAccess = 0x100,
  DenyMeshShaderRootAccess = 0x200,
  CBVSRVUAVHeapDirectlyIndexed = 0x400,
  SamplerHeapDirectlyIndexed = 0x800,
  AllowLowTierReservedHwCbLimit = 0x80000000,
  ValidFlags = 0x80000fff
};
enum class RootParameterType : uint32_t {
  DescriptorTable = 0,
  Constants32Bit = 1,
  CBV = 2,
  SRV = 3,
  UAV = 4,
  MaxValue = 4
};

enum class ComparisonFunc : uint32_t {
  Never = 1,
  Less = 2,
  Equal = 3,
  LessEqual = 4,
  Greater = 5,
  NotEqual = 6,
  GreaterEqual = 7,
  Always = 8
};
enum class Filter : uint32_t {
  MIN_MAG_MIP_POINT = 0,
  MIN_MAG_POINT_MIP_LINEAR = 0x1,
  MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4,
  MIN_POINT_MAG_MIP_LINEAR = 0x5,
  MIN_LINEAR_MAG_MIP_POINT = 0x10,
  MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
  MIN_MAG_LINEAR_MIP_POINT = 0x14,
  MIN_MAG_MIP_LINEAR = 0x15,
  ANISOTROPIC = 0x55,
  COMPARISON_MIN_MAG_MIP_POINT = 0x80,
  COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81,
  COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84,
  COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85,
  COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90,
  COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
  COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94,
  COMPARISON_MIN_MAG_MIP_LINEAR = 0x95,
  COMPARISON_ANISOTROPIC = 0xd5,
  MINIMUM_MIN_MAG_MIP_POINT = 0x100,
  MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101,
  MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104,
  MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105,
  MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110,
  MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111,
  MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114,
  MINIMUM_MIN_MAG_MIP_LINEAR = 0x115,
  MINIMUM_ANISOTROPIC = 0x155,
  MAXIMUM_MIN_MAG_MIP_POINT = 0x180,
  MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181,
  MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184,
  MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185,
  MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190,
  MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191,
  MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194,
  MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195,
  MAXIMUM_ANISOTROPIC = 0x1d5
};
enum class ShaderVisibility : uint32_t {
  All = 0,
  Vertex = 1,
  Hull = 2,
  Domain = 3,
  Geometry = 4,
  Pixel = 5,
  Amplification = 6,
  Mesh = 7,
  MaxValue = 7
};
enum class StaticBorderColor : uint32_t {
  TransparentBlack = 0,
  OpaqueBlack = 1,
  OpaqueWhite = 2,
  OpaqueBlackUint = 3,
  OpaqueWhiteUint = 4
};
enum class TextureAddressMode : uint32_t {
  Wrap = 1,
  Mirror = 2,
  Clamp = 3,
  Border = 4,
  MirrorOnce = 5
};

} // namespace RootSignature
} // namespace hlsl

namespace dxbc {

namespace RootSignature {

namespace v_1_0 {

struct ContainerRootDescriptor {
  uint32_t ShaderRegister;
  uint32_t RegisterSpace;
};

struct ContainerDescriptorRange {
  hlsl::RootSignature::DescriptorRangeType RangeType;
  uint32_t NumDescriptors;
  uint32_t BaseShaderRegister;
  uint32_t RegisterSpace;
  uint32_t OffsetInDescriptorsFromTableStart;
};

} // namespace v_1_0

namespace v_1_1 {

struct ContainerRootDescriptor {
  uint32_t ShaderRegister;
  uint32_t RegisterSpace = 0;
  uint32_t Flags;
};

struct ContainerDescriptorRange {
  hlsl::RootSignature::DescriptorRangeType RangeType;
  uint32_t NumDescriptors;
  uint32_t BaseShaderRegister;
  uint32_t RegisterSpace;
  uint32_t Flags;
  uint32_t OffsetInDescriptorsFromTableStart;
};
} // namespace v_1_1

struct ContainerRootDescriptorTable {
  uint32_t NumDescriptorRanges;
  uint32_t DescriptorRangesOffset;
};

struct ContainerRootConstants {
  uint32_t ShaderRegister;
  uint32_t RegisterSpace = 0;
  uint32_t Num32BitValues;
};

struct ContainerRootParameter {
  hlsl::RootSignature::RootParameterType ParameterType;
  hlsl::RootSignature::ShaderVisibility ShaderVisibility =
      hlsl::RootSignature::ShaderVisibility::All;
  uint32_t PayloadOffset;
};

struct StaticSamplerDesc {
  hlsl::RootSignature::Filter Filter = hlsl::RootSignature::Filter::ANISOTROPIC;
  hlsl::RootSignature::TextureAddressMode AddressU =
      hlsl::RootSignature::TextureAddressMode::Wrap;
  hlsl::RootSignature::TextureAddressMode AddressV =
      hlsl::RootSignature::TextureAddressMode::Wrap;
  hlsl::RootSignature::TextureAddressMode AddressW =
      hlsl::RootSignature::TextureAddressMode::Wrap;
  float MipLODBias = 0;
  uint32_t MaxAnisotropy = 16;
  hlsl::RootSignature::ComparisonFunc ComparisonFunc =
      hlsl::RootSignature::ComparisonFunc::LessEqual;
  hlsl::RootSignature::StaticBorderColor BorderColor =
      hlsl::RootSignature::StaticBorderColor::OpaqueWhite;
  float MinLOD = 0;
  float MaxLOD = hlsl::RootSignature::Float32Max;
  uint32_t ShaderRegister;
  uint32_t RegisterSpace = 0;
  hlsl::RootSignature::ShaderVisibility ShaderVisibility =
      hlsl::RootSignature::ShaderVisibility::All;
};

struct ContainerRootSignatureDesc {
  hlsl::RootSignature::RootSignatureVersion Version;
  uint32_t NumParameters;
  uint32_t RootParametersOffset;
  uint32_t NumStaticSamplers;
  uint32_t StaticSamplersOffset;
  uint32_t Flags;
};

} // namespace RootSignature

} // namespace dxbc

namespace hlsl {

struct ParsedRootSignature {
  dxbc::RootSignature::ContainerRootSignatureDesc RSDesc;
  llvm::SmallVector<dxbc::RootSignature::ContainerRootParameter, 8>
      RSParameters;
  llvm::SmallVector<dxbc::RootSignature::StaticSamplerDesc, 8> StaticSamplers;
  llvm::SmallVector<dxbc::RootSignature::v_1_1::ContainerDescriptorRange, 8>
      DescriptorRanges;
  llvm::SmallVector<
      std::variant<dxbc::RootSignature::ContainerRootConstants,
                   dxbc::RootSignature::v_1_1::ContainerRootDescriptor,
                   dxbc::RootSignature::ContainerRootDescriptorTable>,
      8>
      RSParamExtras;
};

llvm::StringRef printRootSignature(const ParsedRootSignature &RS);

llvm::SmallVector<uint32_t, 64>
serializeRootSignature(const ParsedRootSignature &RS);
void deserializeRootSignature(const llvm::SmallVectorImpl<uint32_t> &Data,
                              ParsedRootSignature &RS);

} // namespace hlsl

} // namespace llvm

#endif // LLVM_FRONTEND_HLSL_HLSLROOTSIGNATURE_H
