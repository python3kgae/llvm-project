//- DXIL.td - Describe DXIL operation -------------------------*- tablegen -*-//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This is a target description file for DXIL operation.
///
//===----------------------------------------------------------------------===//

include "llvm/IR/Intrinsics.td"

class dxil_class<string _name> {
  string name = _name;
}
class dxil_category<string _name> {
  string name = _name;
}

def Unary : dxil_class<"Unary">;
def Binary : dxil_class<"Binary">;
def FlattenedThreadIdInGroupClass : dxil_class<"FlattenedThreadIdInGroup">;
def ThreadIdInGroupClass : dxil_class<"ThreadIdInGroup">;
def ThreadIdClass : dxil_class<"ThreadId">;
def GroupIdClass : dxil_class<"GroupId">;
def BufferLoadClass : dxil_class<"BufferLoad">;
def BufferStoreClass : dxil_class<"BufferStore">;
def CreateHandleClass : dxil_class<"CreateHandle">;

def binary_uint : dxil_category<"Binary uint">;
def unary_float : dxil_category<"Unary float">;
def ComputeID : dxil_category<"Compute/Mesh/Amplification shader">;
def Resources : dxil_category<"Resources">;

// The parameter description for a DXIL instruction
class dxil_param<int _pos, string type, string _name, string _doc,
                 bit _is_const = 0, string _enum_name = "",
                 int _max_value = 0> {
  int pos = _pos;           // position in parameter list
  string llvm_type = type; // llvm type name, $o for overload, $r for resource
                           // type, $cb for legacy cbuffer, $u4 for u4 struct
  string name = _name;      // short, unique name
  string doc = _doc;        // the documentation description of this parameter
  bit is_const =
      _is_const; // whether this argument requires a constant value in the IR
  string enum_name = _enum_name; // the name of the enum type if applicable
  int max_value =
      _max_value; // the maximum value for this parameter if applicable
}

// A representation for a DXIL instruction
class dxil_inst<string _name> {
  string name = _name; // short, unique name

  string dxil_op = "";       // name of DXIL operation
  int dxil_opid = 0;         // ID of DXIL operation
  dxil_class  op_class;      // name of the opcode class
  dxil_category category;    // classification for this instruction
  string doc = "";           // the documentation description of this instruction
  list<dxil_param> ops = []; // the operands that this instruction takes
  string oload_types = "";   // overload types if applicable
  string fn_attr = "";       // attribute shorthands: rn=does not access
                             // memory,ro=only reads from memory,
  bit is_deriv = 0;          // whether this is some kind of derivative
  bit is_gradient = 0;       // whether this requires a gradient calculation
  bit is_feedback = 0;       // whether this is a sampler feedback op
  bit is_wave = 0; // whether this requires in-wave, cross-lane functionality
  bit requires_uniform_inputs = 0; // whether this operation requires that all
                                   // of its inputs are uniform across the wave
  // Group dxil operation for stats.
  // Like how many atomic/float/uint/int/... instructions used in the program.
  list<string> stats_group = [];
}

class dxil_op<string name, int code_id, dxil_class code_class, dxil_category op_category, string _doc,
              string _oload_types, string _fn_attr, list<dxil_param> op_params,
              list<string> _stats_group = []> : dxil_inst<name> {
  let dxil_op = name;
  let dxil_opid = code_id;
  let doc = _doc;
  let ops = op_params;
  let op_class = code_class;
  let category = op_category;
  let oload_types = _oload_types;
  let fn_attr = _fn_attr;
  let stats_group = _stats_group;
}

// The intrinsic which map directly to this dxil op.
class dxil_map_intrinsic<Intrinsic llvm_intrinsic_> { Intrinsic llvm_intrinsic = llvm_intrinsic_; }

def Sin : dxil_op<"Sin", 13, Unary, unary_float, "returns sine(theta) for theta in radians.",
  "half;float;", "rn",
  [
    dxil_param<0, "$o", "", "operation result">,
    dxil_param<1, "i32", "opcode", "DXIL opcode">,
    dxil_param<2, "$o", "value", "input value">
  ],
  ["floats"]>,
  dxil_map_intrinsic<int_sin>;

def UMax :dxil_op< "UMax", 39,  Binary,  binary_uint, "unsigned integer maximum. UMax(a,b) = a > b ? a : b",
    "i16;i32;i64;",  "rn",
  [
    dxil_param<0,  "$o",  "",  "operation result">,
    dxil_param<1,  "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2,  "$o",  "a",  "input value">,
    dxil_param<3,  "$o",  "b",  "input value">
  ],
  ["uints"]>,
  dxil_map_intrinsic<int_umax>;

def ThreadId :dxil_op< "ThreadId", 93,  ThreadIdClass, ComputeID, "reads the thread ID", "i32;",  "rn",
  [
    dxil_param<0,  "i32",  "",  "thread ID component">,
    dxil_param<1,  "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2,  "i32",  "component",  "component to read (x,y,z)">
  ]>,
  dxil_map_intrinsic<int_dxil_thread_id>;

def GroupId :dxil_op< "GroupId", 94,  GroupIdClass, ComputeID, "reads the group ID (SV_GroupID)", "i32;",  "rn",
  [
    dxil_param<0,  "i32",  "",  "group ID component">,
    dxil_param<1,  "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2,  "i32",  "component",  "component to read">
  ]>,
  dxil_map_intrinsic<int_dxil_group_id>;

def ThreadIdInGroup :dxil_op< "ThreadIdInGroup", 95,  ThreadIdInGroupClass, ComputeID,
  "reads the thread ID within the group (SV_GroupThreadID)", "i32;",  "rn",
  [
    dxil_param<0,  "i32",  "",  "thread ID in group component">,
    dxil_param<1,  "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2,  "i32",  "component",  "component to read (x,y,z)">
  ]>,
  dxil_map_intrinsic<int_dxil_thread_id_in_group>;

def FlattenedThreadIdInGroup :dxil_op< "FlattenedThreadIdInGroup", 96,  FlattenedThreadIdInGroupClass, ComputeID,
   "provides a flattened index for a given thread within a given group (SV_GroupIndex)", "i32;",  "rn",
  [
    dxil_param<0,  "i32",  "",  "result">,
    dxil_param<1,  "i32",  "opcode",  "DXIL opcode">
  ]>,
  dxil_map_intrinsic<int_dxil_flattened_thread_id_in_group>;

def BufferLoad : dxil_op< "BufferLoad", 68, BufferLoadClass,Resources, "reads from a TypedBuffer", "half;float;i16;i32;", "ro",
  [
    dxil_param<0, "dx.types.ResRet", "", "the loaded value">,
    dxil_param<1, "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2, "dx.types.Handle", "srv", "handle of TypedBuffer SRV to sample">,
    dxil_param<3, "i32", "index", "element index">,
    dxil_param<4, "i32", "wot", "coordinate">
  ],
  ["tex_load"]>;

def BufferStore : dxil_op< "BufferStore", 69, BufferStoreClass, Resources, "writes to a RWTypedBuffer", "half;float;i16;i32;", "",
  [
    dxil_param<0, "void", "", "">,
    dxil_param<1, "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2, "dx.types.Handle", "uav", "handle of UAV to store to">,
    dxil_param<3, "i32", "coord0", "coordinate in elements">,
    dxil_param<4, "i32", "coord1", "coordinate (unused?)">,
    dxil_param<5, "$o", "value0", "value">,
    dxil_param<6, "$o", "value1", "value">,
    dxil_param<7, "$o", "value2", "value">,
    dxil_param<8, "$o", "value3", "value">,
    dxil_param<9, "i8", "mask", "written value mask">
  ],
  ["tex_store"]>;

def CreateHandle : dxil_op< "CreateHandle", 57, CreateHandleClass, Resources, "creates the handle to a resource",
  "void;", "ro",
  [
    dxil_param<0, "dx.types.Handle", "", "the handle to the resource">,
    dxil_param<1, "i32",  "opcode",  "DXIL opcode">,
    dxil_param<2, "i8", "resourceClass", "the class of resource to create (SRV, UAV, CBuffer, Sampler)", 1>, // maps to DxilResourceBase::Class
    dxil_param<3, "i32", "rangeId", "range identifier for resource", 1>,
    dxil_param<4, "i32", "index", "zero-based index into range">,
    dxil_param<5, "i1", "nonUniformIndex", "non-uniform resource index", 1>
  ]>;
