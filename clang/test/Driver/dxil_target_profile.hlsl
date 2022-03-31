// RUN: %clang_dxc -Tvs_6_0 --target=dxil -Fo - %s 2>&1 | FileCheck %s --check-prefix=VS60
// VS60:target triple = "dxil-unknown-shadermodel6.0-vertex"

// RUN: %clang_dxc -Ths_6_1  --target=dxil -Fo - %s 2>&1 | FileCheck %s --check-prefix=HS61
// HS61:target triple = "dxil-unknown-shadermodel6.1-hull"

// RUN: %clang_dxc -Tps_6_1 --target=dxil -Fo - %s 2>&1 | FileCheck %s --check-prefix=PS61
// PS61:target triple = "dxil-unknown-shadermodel6.1-pixel"

// RUN: %clang_dxc -Tlib_6_x  --target=dxil -Fo - %s 2>&1 | FileCheck %s --check-prefix=LIB63
// LIB63:target triple = "dxil-unknown-shadermodel6.15-library"

// RUN: %clang_dxc -###  -Tps_3_1  --target=dxil -Fo - %s 2>&1 | FileCheck %s --check-prefix=INVALID
// INVALID:invalid profile : ps_3_1