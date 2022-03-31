// RUN: %clang_dxc -Tvs_6_0 -Fo - %s 2>&1 | FileCheck %s --check-prefix=VS60
// VS60:target triple = "dxil-unknown-shadermodel6.0-vertex"

// RUN: %clang_dxc -Ths_6_1  -Fo - %s 2>&1 | FileCheck %s --check-prefix=HS61
// HS61:target triple = "dxil-unknown-shadermodel6.1-hull"

// RUN: %clang_dxc -Tds_6_2  -Fo - %s 2>&1 | FileCheck %s --check-prefix=DS62
// DS62:target triple = "dxil-unknown-shadermodel6.2-domain"

// RUN: %clang_dxc -Tgs_6_3 -Fo - %s 2>&1 | FileCheck %s --check-prefix=GS63
// GS63:target triple = "dxil-unknown-shadermodel6.3-geometry"

// RUN: %clang_dxc -Tps_6_4 -Fo - %s 2>&1 | FileCheck %s --check-prefix=PS64
// PS64:target triple = "dxil-unknown-shadermodel6.4-pixel"

// RUN: %clang_dxc -Tms_6_5 -Fo - %s 2>&1 | FileCheck %s --check-prefix=MS65
// MS65:target triple = "dxil-unknown-shadermodel6.5-mesh"

// RUN: %clang_dxc -Tas_6_6 -Fo - %s 2>&1 | FileCheck %s --check-prefix=AS66
// AS66:target triple = "dxil-unknown-shadermodel6.6-amplification"

// RUN: %clang_dxc -Tlib_6_x  -Fo - %s 2>&1 | FileCheck %s --check-prefix=LIB6x
// LIB6x:target triple = "dxil-unknown-shadermodel6.15-library"

// RUN: %clang_dxc -###  -Tps_3_1  -Fo - %s 2>&1 | FileCheck %s --check-prefix=INVALID
// INVALID:invalid profile : ps_3_1

// RUN: %clang_dxc -###  -Tlib_6_1  -Fo - %s 2>&1 | FileCheck %s --check-prefix=INVALID2
// INVALID2:invalid profile : lib_6_1

// RUN: %clang_dxc -###  -Tms_6_1  -Fo - %s 2>&1 | FileCheck %s --check-prefix=INVALID3
// INVALID3:invalid profile : ms_6_1

// RUN: %clang_dxc -###  -Tas_6_4  -Fo - %s 2>&1 | FileCheck %s --check-prefix=INVALID4
// INVALID4:invalid profile : as_6_4
