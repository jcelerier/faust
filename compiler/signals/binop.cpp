/************************************************************************
 ************************************************************************
    FAUST compiler
	Copyright (C) 2003-2004 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#include "binop.hh"
#include "signals.hh"
#include "compatibility.hh"

static bool noNtrl(const Node& n) { return falsePredicate(n); }

#if LLVM_BUILD

#if defined(LLVM_33) || defined(LLVM_34) || defined(LLVM_35) || defined(LLVM_36) || defined(LLVM_37) || defined(LLVM_38)
#include <llvm/IR/Instructions.h>
#else
#include <llvm/Instructions.h>
#include <llvm/Instruction.h>
#endif

using namespace llvm;

BinOp* gBinOpTable[] = {

	new BinOp("+","add_vec","add_scal", "add nsw", "fadd", Instruction::Add, Instruction::FAdd, 
        FIRInstruction::kAddInt, FIRInstruction::kAddReal, "i32.add", "f32.add", &addNode, &isZero, &isZero, 6),
	new BinOp("-","sub_vec","sub_scal", "sub nsw", "fsub", Instruction::Sub, Instruction::FSub, 
        FIRInstruction::kSubInt, FIRInstruction::kSubReal, "i32.sub", "f32.sub", &subNode, &noNtrl, &isZero, 7),
	new BinOp("*","mul_vec","mul_scal", "mul nsw", "fmul", Instruction::Mul, Instruction::FMul, 
        FIRInstruction::kMultInt, FIRInstruction::kMultReal, "i32.mul", "f32.mul", &mulNode, &isOne, &isOne, 8, isZero, isZero),
	new BinOp("/","div_vec","div_scal", "sdiv", "fdiv", Instruction::SDiv, Instruction::FDiv, 
        FIRInstruction::kDivInt, FIRInstruction::kDivReal, "i32.div_s", "f32.div", &divExtendedNode, &noNtrl, &isOne, 10),
	new BinOp("%","mod_vec","mod_scal", "srem", "frem", Instruction::SRem, Instruction::FRem, 
        FIRInstruction::kRemInt, FIRInstruction::kRemReal, "i32.rem_s", "error", &remNode, &noNtrl, &noNtrl, 9),

	new BinOp("<<","shift_left_vec","shift_left_scal", "","", Instruction::Shl, Instruction::Shl, 
        FIRInstruction::kLshInt, FIRInstruction::kLshInt, "i32.shl", "error", &lshNode, &noNtrl, &isZero, 8),
	new BinOp(">>","shift_right_vec","shift_right_scal","","", Instruction::LShr, Instruction::LShr, 
        FIRInstruction::kRshInt, FIRInstruction::kRshInt, "i32.shr_s", "error", &rshNode, &noNtrl, &isZero, 8),

	new BinOp(">","gt_vec","gt_scal", "icmp sgt", "fcmp sgt", ICmpInst::ICMP_SGT, FCmpInst::FCMP_OGT, 
        FIRInstruction::kGTInt, FIRInstruction::kGTReal, "i32.gt_s", "f32.gt", &gtNode, &noNtrl, &noNtrl, 5),
	new BinOp("<","lt_vec","lt_scal", "icmp slt", "fcmp slt", ICmpInst::ICMP_SLT, FCmpInst::FCMP_OLT, 
        FIRInstruction::kLTInt, FIRInstruction::kLTReal, "i32.lt_s", "f32.lt", &ltNode, &noNtrl, &noNtrl, 5),
	new BinOp(">=","ge_vec","ge_scal", "icmp sge", "fcmp sge", ICmpInst::ICMP_SGE, FCmpInst::FCMP_OGE, 
        FIRInstruction::kGEInt, FIRInstruction::kGEReal, "i32.ge_s", "f32.ge", &geNode, &noNtrl, &noNtrl, 5),
	new BinOp("<=","le_vec","le_scal", "icmp sle", "fcmp sle", ICmpInst::ICMP_SLE, FCmpInst::FCMP_OLE, 
        FIRInstruction::kLEInt, FIRInstruction::kLEReal, "i32.le_s", "f32.le", &leNode, &noNtrl, &noNtrl, 5),
	new BinOp("==","eq_vec","eq_scal", "icmp eq", "fcmp eq", ICmpInst::ICMP_EQ, FCmpInst::FCMP_OEQ, 
        FIRInstruction::kEQInt, FIRInstruction::kEQReal, "i32.eq", "f32.eq", &eqNode, &noNtrl, &noNtrl, 5),
	new BinOp("!=","neq_vec","neq_scal", "icmp ne", "fcmp ne", ICmpInst::ICMP_NE, FCmpInst::FCMP_ONE, 
        FIRInstruction::kNEInt, FIRInstruction::kNEReal, "i32.ne", "f32.ne", &neNode, &noNtrl, &noNtrl, 5),

	new BinOp("&","and_vec","and_scal", "and", "and", Instruction::And, Instruction::And, 
        FIRInstruction::kANDInt, FIRInstruction::kANDInt, "i32.and", "error",  &andNode, &isMinusOne, &isMinusOne, 8, isZero, isZero),
	new BinOp("|","or_vec","or_scal", "or", "or", Instruction::Or, Instruction::Or, 
        FIRInstruction::kORInt, FIRInstruction::kORInt, "i32.or", "error",  &orNode, &isZero, &isZero, 7),
	new BinOp("^","xor_vec","xor_scal", "xor", "xor", Instruction::Xor, Instruction::Xor, 
        FIRInstruction::kXORInt, FIRInstruction::kXORInt, "i32.xor", "error",  &xorNode, &noNtrl, &noNtrl, 8)
};

#else

BinOp* gBinOpTable[] = {
    
	new BinOp("+", "add_vec", "add_scal", "add nsw", "fadd", 0, 0,
        FIRInstruction::kAddInt, FIRInstruction::kAddReal, "i32.add", "f32.add", &addNode, &isZero, &isZero, 6),
	new BinOp("-", "sub_vec", "sub_scal", "sub nsw", "fsub", 0, 0,
        FIRInstruction::kSubInt, FIRInstruction::kSubReal, "i32.sub", "f32.sub", &subNode, &noNtrl, &isZero, 7),
	new BinOp("*", "mul_vec", "mul_scal", "mul nsw", "fmul", 0, 0,
        FIRInstruction::kMultInt, FIRInstruction::kMultReal, "i32.mul", "f32.mul", &mulNode, &isOne, &isOne, 8, isZero, isZero),
	new BinOp("/", "div_vec", "div_scal", "sdiv", "fdiv", 0, 0,
        FIRInstruction::kDivInt, FIRInstruction::kDivReal, "i32.div_s", "f32.div", &divExtendedNode, &noNtrl, &isOne, 10),
	new BinOp("%", "mod_vec", "mod_scal", "srem", "frem", 0, 0,
        FIRInstruction::kRemInt, FIRInstruction::kRemReal, "i32.rem_s", "error", &remNode, &noNtrl, &noNtrl, 9),
    
	new BinOp("<<", "shift_left_vec", "shift_left_scal", "","", 0, 0,
        FIRInstruction::kLshInt, FIRInstruction::kLshInt, "i32.shl", "error", &lshNode, &noNtrl, &isZero, 8),
	new BinOp(">>", "shift_right_vec", "shift_right_scal","","", 0, 0,
        FIRInstruction::kRshInt, FIRInstruction::kRshInt, "i32.shr_s", "error", &rshNode, &noNtrl, &isZero, 8),
    
	new BinOp(">", "gt_vec", "gt_scal", "icmp sgt", "fcmp sgt", 0, 0,
        FIRInstruction::kGTInt, FIRInstruction::kGTReal, "i32.gt_s", "f32.gt", &gtNode, &noNtrl, &noNtrl, 5),
	new BinOp("<", "lt_vec", "lt_scal", "icmp slt", "fcmp slt", 0, 0,
        FIRInstruction::kLTInt, FIRInstruction::kLTReal, "i32.lt_s", "f32.lt", &ltNode, &noNtrl, &noNtrl, 5),
	new BinOp(">=", "ge_vec", "ge_scal", "icmp sge", "fcmp sge", 0, 0,
         FIRInstruction::kGEInt, FIRInstruction::kGEReal, "i32.ge_s", "f32.ge", &geNode, &noNtrl, &noNtrl, 5),
	new BinOp("<=", "le_vec", "le_scal", "icmp sle", "fcmp sle", 0, 0,
        FIRInstruction::kLEInt, FIRInstruction::kLEReal, "i32.le_s", "f32.le", &leNode, &noNtrl, &noNtrl, 5),
	new BinOp("==", "eq_vec", "eq_scal", "icmp eq", "fcmp eq", 0, 0,
        FIRInstruction::kEQInt, FIRInstruction::kEQReal, "i32.eq", "f32.eq", &eqNode, &noNtrl, &noNtrl, 5),
	new BinOp("!=", "neq_vec", "neq_scal", "icmp ne", "fcmp ne", 0, 0,
        FIRInstruction::kNEInt, FIRInstruction::kNEReal, "i32.ne", "f32.ne", &neNode, &noNtrl, &noNtrl, 5),
    
	new BinOp("&", "and_vec", "and_scal", "and", "and", 0, 0,
        FIRInstruction::kANDInt, FIRInstruction::kANDInt, "i32.and", "error",  &andNode, &isMinusOne, &isMinusOne, 8, isZero, isZero),
	new BinOp("|", "or_vec", "or_scal", "or", "or", 0, 0,
        FIRInstruction::kORInt, FIRInstruction::kORInt, "i32.or", "error",  &orNode, &isZero, &isZero, 7),
	new BinOp("^", "xor_vec", "xor_scal", "xor", "xor", 0, 0,
        FIRInstruction::kXORInt, FIRInstruction::kXORInt, "i32.xor", "error",  &xorNode, &noNtrl, &noNtrl, 8)
};

#endif

BinOp* gBinOpLateqTable[] = {

	new BinOp("+","add_vec","add_scal", "add nsw", "fadd", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &addNode, &isZero, &isZero, 6),
	new BinOp("-","sub_vec","sub_scal", "sub nsw", "fsub", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &subNode, &noNtrl, &isZero, 7),
	new BinOp("*","mul_vec","mul_scal", "mul nsw", "fmul", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &mulNode, &isOne, &isOne, 8, isZero, isZero), // \DeclareMathSymbol{*}{\mathbin}{symbols}{"01}
	new BinOp("/","div_vec","div_scal", "sdiv", "fdiv", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &divExtendedNode, &noNtrl, &isOne, 10), // \frac{}{} used in generateBinOp
	new BinOp("\\bmod","mod_vec","mod_scal", "srem", "frem", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &remNode, &noNtrl, &noNtrl, 9),

	new BinOp("\\hiderel{\\ll}","shift_left_vec","shift_left_scal","","", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &lshNode, &noNtrl, &isZero, 8),
	new BinOp("\\hiderel{\\gg}","shift_right_vec","shift_right_scal","","", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &rshNode, &noNtrl, &isZero, 8),

	new BinOp("\\hiderel{>}","gt_vec","gt_scal", "icmp sgt", "fcmp sgt", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &gtNode, &noNtrl, &noNtrl, 5),
	new BinOp("\\hiderel{<}","lt_vec","lt_scal", "icmp slt", "fcmp slt", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &ltNode, &noNtrl, &noNtrl, 5),
	new BinOp("\\hiderel{\\geq}","ge_vec","ge_scal", "icmp sge", "fcmp sge", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &geNode, &noNtrl, &noNtrl, 5),
	new BinOp("\\hiderel{\\leq}","le_vec","le_scal", "icmp sle", "fcmp sle", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &leNode, &noNtrl, &noNtrl, 5),
	new BinOp("\\hiderel{\\equiv}","eq_vec","eq_scal", "icmp eq", "fcmp eq", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &eqNode, &noNtrl, &noNtrl, 5),
	new BinOp("\\hiderel{\\neq}","neq_vec","neq_scal", "icmp ne", "fcmp ne", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &neNode, &noNtrl, &noNtrl, 5),

	new BinOp("\\wedge","and_vec","and_scal", "and", "and", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &andNode, &isMinusOne, &isMinusOne, 8, isZero, isZero),
	new BinOp("\\vee","or_vec","or_scal","or", "or", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &orNode, &isZero, &isZero, 7),
	new BinOp("\\veebar","xor_vec","xor_scal", "xor", "xor", 0, 0, 
        FIRInstruction::Opcode(0), FIRInstruction::Opcode(0), "", "", &xorNode, &noNtrl, &noNtrl, 8)
};
