; RUN: llc %s -mtriple dxil-pc-shadermodel6.15-library --filetype=asm -o - | FileCheck %s
target datalayout = "e-m:e-p:32:32-i1:32-i8:8-i16:16-i32:32-i64:64-f16:16-f32:32-f64:64-n8:16:32:64"

; CHECK:!dx.version = !{![[ver:[0-9]+]]}
; CHECK:!dx.valver = !{![[valver:[0-9]+]]}
; CHECK-DAG:![[ver]] = !{i32 1, i32 7}
; CHECK-DAG:![[valver]] = !{i32 0, i32 0}

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 6, !"dx.valver", !2}
!2 = !{i32 1, i32 5}
!3 = !{!"clang version 15.0.0 (https://github.com/llvm/llvm-project 71de12113a0661649ecb2f533fba4a2818a1ad68)"}