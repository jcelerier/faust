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

#ifndef _BINOP_
#define _BINOP_

#include "node.hh"
#include "fir_opcode.hh"

typedef const Node (*comp) (const Node& a, const Node& b);
typedef bool (*pred) (const Node& a);

static inline bool falsePredicate(Node const& a)
{
    return false;
}

enum {
    kAdd, kSub, kMul, kDiv, kRem,
    kLsh, kRsh,
    kGT, kLT, kGE, kLE, kEQ, kNE,
    kAND, kOR, kXOR
};

// Use in in static table so not Garbageable
struct BinOp {

    std::string	fName;
    
    std::string	fNameVec;
    std::string	fNameScal;
    
    std::string	fNameLLVMInt;
    std::string	fNameLLVMFloat;

    unsigned int fLLVMIntInst;
    unsigned int fLLVMFloatInst;

    FIRInstruction::Opcode fInterpIntInst;
    FIRInstruction::Opcode fInterpFloatInst;
    
    std::string fWASMInt;
    std::string fWASMFloat;

    comp fCompute;
    pred fLeftNeutral;
    pred fRightNeutral;
    pred fLeftAbsorbing;
    pred fRightAbsorbing;
    int fPriority;
	
    BinOp(const std::string& name,
        const std::string& name_vec,
        const std::string& name_scal,
        const std::string& name_llvm_int,
        const std::string& name_llvm_float,
        unsigned int llvm_int,
        unsigned int llvm_float,
        FIRInstruction::Opcode interp_int,
        FIRInstruction::Opcode interp_float,
        const std::string& wasm_int,
        const std::string& wasm_float,
        comp f,
        pred ln,
        pred rn,
        int priority,
        pred la = falsePredicate,
        pred ra = falsePredicate)
        :fName(name), fNameVec(name_vec), fNameScal(name_scal),
        fNameLLVMInt(name_llvm_int), fNameLLVMFloat(name_llvm_float),
        fLLVMIntInst(llvm_int), fLLVMFloatInst(llvm_float),
        fInterpIntInst(interp_int), fInterpFloatInst(interp_float),
        fWASMInt(wasm_int), fWASMFloat(wasm_float),
        fCompute(f), fLeftNeutral(ln), fRightNeutral(rn),
        fLeftAbsorbing(la), fRightAbsorbing(ra), fPriority(priority)
    {}

    Node compute(const Node& a, const Node& b) { return fCompute(a,b); 	}

    bool isRightNeutral(const Node& a)      { return fRightNeutral(a); 	    }
    bool isLeftNeutral(const Node& a)       { return fLeftNeutral(a); 	    }
    bool isLeftAbsorbing(const Node& a)     { return fLeftAbsorbing(a);     }
    bool isRightAbsorbing(const Node& a)    { return fRightAbsorbing(a);    }
};

inline bool isBoolOpcode(int o)
{
    return (o >= kGT && o <= kNE);
}

inline bool isCommutativeOpcode(int o)
{
    return ((o == kAdd) || (o == kMul) || (o == kEQ) || (o == kNE) || (o == kAND) || (o == kOR) || (o == kXOR));
}

inline bool isLogicalOpcode(int o)
{
    return (o >= kAND && o <= kXOR);
}

extern BinOp* gBinOpTable[];
extern BinOp* gBinOpLateqTable[];

#endif
