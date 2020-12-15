#include "Andersen.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// CollectConstraints - This stage scans the program, adding a constraint to the
// Constraints list for each instruction in the program that induces a
// constraint, and setting up the initial points-to graph.

void Andersen::collectConstraints(const Module &M)
{
  // First, the universal ptr points to universal obj, and the universal obj
  // points to itself
  constraints.emplace_back(AndersConstraint::ADDR_OF,
                           nodeFactory.getUniversalPtrNode(),
                           nodeFactory.getUniversalObjNode());
  constraints.emplace_back(AndersConstraint::STORE,
                           nodeFactory.getUniversalObjNode(),
                           nodeFactory.getUniversalObjNode());

  // Next, the null pointer points to the null object.
  constraints.emplace_back(AndersConstraint::ADDR_OF,
                           nodeFactory.getNullPtrNode(),
                           nodeFactory.getNullObjectNode());

  for (auto const &f : M)
  {
    if (f.isDeclaration() || f.isIntrinsic())
      continue;

    // Scan the function body
    // First, create a value node for each instruction with pointer type. 
    // PHI
    for (const_inst_iterator itr = inst_begin(f), ite = inst_end(f); itr != ite;
         ++itr)
    {
      auto inst = &*itr.getInstructionIterator();
      if (inst->getType()->isPointerTy())
        nodeFactory.createValueNode(inst);
    }

    // Now, collect constraint for each relevant instruction
    for (const_inst_iterator itr = inst_begin(f), ite = inst_end(f); itr != ite;
         ++itr)
    {
      auto inst = &*itr.getInstructionIterator();
      collectConstraintsForInstruction(inst);
    }
  }
}

void Andersen::collectConstraintsForInstruction(const Instruction *inst)
{
  switch (inst->getOpcode())
  {
  case Instruction::Alloca:
  {
    NodeIndex valNode = nodeFactory.getValueNodeFor(inst);
    assert(valNode != AndersNodeFactory::InvalidIndex &&
           "Failed to find alloca value node");
    NodeIndex objNode = nodeFactory.createObjectNode(inst);
    constraints.emplace_back(AndersConstraint::ADDR_OF, valNode, objNode);
    break;
  }
  case Instruction::Call:
  case Instruction::Invoke:
  {
    ImmutableCallSite cs(inst);
    assert(cs && "Something wrong with callsite?");

    addConstraintForCall(cs);

    break;
  }
  case Instruction::Ret:
  {
    if (inst->getNumOperands() > 0 &&
        inst->getOperand(0)->getType()->isPointerTy())
    {
      NodeIndex retIndex =
          nodeFactory.getReturnNodeFor(inst->getParent()->getParent());
      assert(retIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find return node");
      NodeIndex valIndex = nodeFactory.getValueNodeFor(inst->getOperand(0));
      assert(valIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find return value node");
      constraints.emplace_back(AndersConstraint::COPY, retIndex, valIndex);
    }
    break;
  }
  case Instruction::Load:
  {
    if (inst->getType()->isPointerTy())
    {
      NodeIndex opIndex = nodeFactory.getValueNodeFor(inst->getOperand(0));
      assert(opIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find load operand node");
      NodeIndex valIndex = nodeFactory.getValueNodeFor(inst);
      assert(valIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find load value node");
      constraints.emplace_back(AndersConstraint::LOAD, valIndex, opIndex);
    }
    break;
  }
  case Instruction::Store:
  {
    if (inst->getOperand(0)->getType()->isPointerTy())
    {
      NodeIndex srcIndex = nodeFactory.getValueNodeFor(inst->getOperand(0));
      assert(srcIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find store src node");
      NodeIndex dstIndex = nodeFactory.getValueNodeFor(inst->getOperand(1));
      assert(dstIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find store dst node");
      constraints.emplace_back(AndersConstraint::STORE, dstIndex, srcIndex);
    }
    break;
  }
  case Instruction::GetElementPtr:
  {
    assert(inst->getType()->isPointerTy());

    // P1 = getelementptr P2, ... --> <Copy/P1/P2>
    NodeIndex srcIndex = nodeFactory.getValueNodeFor(inst->getOperand(0));
    assert(srcIndex != AndersNodeFactory::InvalidIndex &&
           "Failed to find gep src node");
    NodeIndex dstIndex = nodeFactory.getValueNodeFor(inst);
    assert(dstIndex != AndersNodeFactory::InvalidIndex &&
           "Failed to find gep dst node");

    constraints.emplace_back(AndersConstraint::COPY, dstIndex, srcIndex);

    break;
  }
  case Instruction::PHI:
  {
    if (inst->getType()->isPointerTy())
    {
      const PHINode *phiInst = cast<PHINode>(inst);
      NodeIndex dstIndex = nodeFactory.getValueNodeFor(phiInst);
      assert(dstIndex != AndersNodeFactory::InvalidIndex &&
             "Failed to find phi dst node");
      for (unsigned i = 0, e = phiInst->getNumIncomingValues(); i != e; ++i)
      {
        NodeIndex srcIndex =
            nodeFactory.getValueNodeFor(phiInst->getIncomingValue(i));
        assert(srcIndex != AndersNodeFactory::InvalidIndex &&
               "Failed to find phi src node");
        constraints.emplace_back(AndersConstraint::COPY, dstIndex, srcIndex);
      }
    }
    break;
  }
  case Instruction::BitCast:
  case Instruction::IntToPtr:
  case Instruction::Select:
  case Instruction::VAArg:
  case Instruction::ExtractValue:
  case Instruction::InsertValue:
  {
    if (!inst->getType()->isPointerTy())
      break;
  }
  // exception-handling
  case Instruction::LandingPad:
  case Instruction::Resume:
  // Atomic instructions
  case Instruction::AtomicRMW:
  case Instruction::AtomicCmpXchg:
  {
    errs() << *inst << "\n";
    errs() << "not implemented yet" << "\n";
  }
  default:
  {
    if (inst->getType()->isPointerTy())
    {
      errs() << *inst << "\n";
      llvm_unreachable("pointer-related inst not handled!");
    }
    break;
  }
  }
}
