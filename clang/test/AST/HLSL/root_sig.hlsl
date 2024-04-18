// RUN: %clang_cc1 -triple dxil-pc-shadermodel6.3-compute -x hlsl -ast-dump -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple dxil-pc-shadermodel6.3-library -x hlsl -ast-dump -o - %s | FileCheck %s

#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                         "DENY_VERTEX_SHADER_ROOT_ACCESS), " \
              "CBV(b0, space = 1, flags = DATA_STATIC), " \
              "SRV(t0), " \
              "UAV(u0), " \
              "DescriptorTable( CBV(b1), " \
                               "SRV(t1, numDescriptors = 8, " \
                               "        flags = DESCRIPTORS_VOLATILE), " \
                               "UAV(u1, numDescriptors = unbounded, " \
                               "        flags = DESCRIPTORS_VOLATILE)), " \
              "DescriptorTable(Sampler(s0, space=1, numDescriptors = 4)), " \
              "RootConstants(num32BitConstants=3, b10), " \
              "StaticSampler(s1)," \
              "StaticSampler(s2, " \
                             "addressU = TEXTURE_ADDRESS_CLAMP, " \
                             "filter = FILTER_MIN_MAG_MIP_LINEAR )"

// CHECK: FunctionDecl {{.*}} main 'void ()'
// CHECK-NEXT:  CompoundStmt
// CHECK-NEXT:  HLSLNumThreadsAttr {{.*}} 8 8 1
// CHECK-NEXT:  HLSLEntryRootSignatureAttr {{.*}} "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_VERTEX_SHADER_ROOT_ACCESS), CBV(b0, space = 1, flags = DATA_STATIC), SRV(t0), UAV(u0), DescriptorTable( CBV(b1), SRV(t1, numDescriptors = 8, flags = DESCRIPTORS_VOLATILE), UAV(u1, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE)), DescriptorTable(Sampler(s0, space=1, numDescriptors = 4)), RootConstants(num32BitConstants=3, b10), StaticSampler(s1),StaticSampler(s2, addressU = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR )" HLSLRootSignature {{.*}} 'main.RS'

[numthreads(8,8,1)]
[RootSignature(MyRS1)]
void main()
{

}
