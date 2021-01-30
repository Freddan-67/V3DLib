#ifndef _V3DLIB_TARGET_SYNTAX_H_
#define _V3DLIB_TARGET_SYNTAX_H_
#include <stdint.h>
#include <string>
#include "Common/Seq.h"  // for check_zeroes()
#include "Support/InstructionComment.h"
#include "Support/debug.h"
#include "Source/Var.h"
#include "Source/BExpr.h"
#include "Reg.h"
#include "instr/ALUOp.h"
#include "instr/Conditions.h"

namespace V3DLib {

// Syntax of the QPU target language.

// This abstract syntax is a balance between a strict and relaxed
// definition of the target language:
// 
//   a "strict" definition would allow only instructions that can run on
//   the target machine to be expressed, whereas a "relaxed" one allows
//   instructions that have no direct mapping to machine instructions.
// 
// A relaxed definition allows the compilation process to be incremental:
// after each pass, the target code gets closer to being executable, by
// transforming away constructs that do not have a direct mapping to
// hardware.  However, we do not want to be too relaxed, otherwise we
// loose scope for the type checker to help us.
// 
// For example, the definition below allows an instruction to read two
// operands from the *same* register file.  In fact, two operands must be
// taken from different register files in the target language. It is the
// job of a compiler pass to enforce such a constraint.

// ============================================================================
// Immediates
// ============================================================================

// Different kinds of immediate
enum ImmTag {
    IMM_INT32    // 32-bit word
  , IMM_FLOAT32  // 32-bit float
  , IMM_MASK     // 1 bit per vector element (0 to 0xffff)
};

struct Imm {
  ImmTag tag;

  union {
    int intVal;
    float floatVal;
  };
};

// Different kinds of small immediates
enum SmallImmTag {
    SMALL_IMM  // Small immediate
  , ROT_ACC    // Rotation amount taken from accumulator 5
  , ROT_IMM    // Rotation amount 1..15
};

struct SmallImm {
  // What kind of small immediate is it?
  SmallImmTag tag;
  
  // Immediate value
  int val;

  bool operator==(SmallImm const &rhs) const {
    return tag == rhs.tag && val == rhs.val;
  }

  bool operator!=(SmallImm const &rhs) const {
    return !(*this == rhs);
  }
};

// A register or a small immediate operand?
enum RegOrImmTag { REG, IMM };

struct RegOrImm {
  // Register id or small immediate?
  RegOrImmTag tag;

  union {
    // A register
    Reg reg;
    
    // A small immediate
    SmallImm smallImm;
  };

  bool operator==(RegOrImm const &rhs) const {
    if (tag != rhs.tag) return false;

    if (tag == REG) {
      return reg == rhs.reg;
    } else {
      return smallImm == rhs.smallImm;
    }
  }

  bool operator!=(RegOrImm const &rhs) const {
    return !(*this == rhs);
  }
};


// ============================================================================
// Branch targets
// ============================================================================

struct BranchTarget {
  bool relative;      // Branch is absolute or relative to PC+4

  bool useRegOffset;  // Plus value from register file A (optional)
  RegId regOffset;

  int immOffset;      // Plus 32-bit immediate value

  std::string to_string() const;
};


// We allow labels for branching, represented by integer identifiers.  These
// will be translated to actual branch targets in a linking phase.

typedef int Label;



// ======================
// Fresh label generation
// ======================

// Obtain a fresh label
Label freshLabel();

// Number of fresh labels used
int getFreshLabelCount();

// Reset fresh label generator
void resetFreshLabelGen();
void resetFreshLabelGen(int val);

// ============================================================================
// Instructions
// ============================================================================

// QPU instruction tags
enum InstrTag {
  LI,             // Load immediate
  ALU,            // ALU operation
  BR,             // Conditional branch to target
  END,            // Program end (halt)

  // ==================================================
  // Intermediate-language constructs
  // ==================================================

  BRL,            // Conditional branch to label
  LAB,            // Label
  NO_OP,          // No-op

  VC4_ONLY,

  DMA_LOAD_WAIT = VC4_ONLY, // Wait for DMA load to complete
  DMA_STORE_WAIT, // Wait for DMA store to complete
  SINC,           // Increment semaphore
  SDEC,           // Decrement semaphore
  IRQ,            // Send IRQ to host

  // Print instructions
  PRS,            // Print string
  PRI,            // Print integer
  PRF,            // Print float

  VPM_STALL,      // Marker for VPM read setup

  END_VC4_ONLY,

  // Load receive via TMU
  RECV = END_VC4_ONLY,
  TMU0_TO_ACC4,

  // Init program block (Currently filled only for v3d)
  INIT_BEGIN,     // Marker for start of init block
  INIT_END,       // Marker for end of init block

  // ==================================================
  // v3d-only instructions
  // ==================================================
  V3D_ONLY,

  TMUWT = V3D_ONLY,
  // TODO Add as required here

  END_V3D_ONLY
};


void check_instruction_tag_for_platform(InstrTag tag, bool for_vc4);

// QPU instructions
struct Instr : public InstructionComment {

  class List : public Seq<Instr> {
    using Parent = Seq<Instr>;
  public:
    List() = default;
    List(int size) : Parent(size) {}

    std::string dump() const;
    std::string mnemonics(bool with_comments = false) const;
		int lastUniformOffset();
		int tag_index(InstrTag tagi, bool ensure_one = true);
		int tag_count(InstrTag tag);
  };

  InstrTag tag;

  union {
    // Load immediate
    struct {
      SetCond    m_setCond;
      AssignCond cond;
      Reg        dest;
      Imm        imm;
    } LI;

    // ALU operation
    struct {
      SetCond    m_setCond;
      AssignCond cond;
      Reg        dest;
      RegOrImm   srcA;
      ALUOp      op;
      RegOrImm   srcB;
    } ALU;

    // Conditional branch (to target)
    struct {
      BranchCond cond;
      BranchTarget target;
    } BR;

    // ==================================================
    // Intermediate-language constructs
    // ==================================================

    // Conditional branch (to label)
    struct {
      BranchCond cond;
      Label label;
    } BRL;

    // Labels, denoting branch targets
    Label m_label;  // Renamed during debugging
                    // TODO perhaps revert

    // Semaphores
    int semaId;                 // Semaphore id (range 0..15)

    // Load receive via TMU
    struct { Reg dest; } RECV;  // Destination register for load receive

    // Print instructions
    const char* PRS;            // Print string
    Reg PRI;                    // Print integer
    Reg PRF;                    // Print float
  };


  Instr() : tag(NO_OP) {} 
  Instr(InstrTag in_tag);


  // ==================================================
  // Helper methods
  // ==================================================
  Instr &setCondFlag(Flag flag);
  Instr &setCondOp(CmpOp const &cmp_op);
  Instr &cond(AssignCond in_cond);
  bool isCondAssign() const;
  bool hasImm() const { return ALU.srcA.tag == IMM || ALU.srcB.tag == IMM; }
  bool isUniformLoad() const;
  bool isTMUAWrite() const;
  bool isZero() const;
  bool isLast() const;

  SetCond const &setCond() const;
  std::string mnemonic(bool with_comments = false, std::string const &pref = "") const;
  std::string dump() const;

  bool operator==(Instr const &rhs) const {
    // Cheat by comparing the string representation,
    // to avoid having to check the union members separately, and to skip unused fields
    return this->mnemonic() == rhs.mnemonic();
  }


  static Instr nop();

  /////////////////////////////////////
  // Label support
  /////////////////////////////////////

  bool is_label() const { return tag == InstrTag::LAB; }
  bool is_branch_label() const { return tag == InstrTag::BRL; }

  Label branch_label() const {
    assert(tag == InstrTag::BRL);
    return BRL.label;
  }

  void label_to_target(int offset);
    
  void label(Label val) {
    assert(tag == InstrTag::LAB);
    m_label = val;
  }

  Label label() const {
    assert(tag == InstrTag::LAB);
    return m_label;
  }

  // ==================================================
  // v3d-specific  methods
  // ==================================================
  Instr &pushz();

  Instr &allzc() {
    assert(tag == InstrTag::BRL);
    BRL.cond.tag  = COND_ALL;
    BRL.cond.flag = Flag::ZC;
    return *this;
  }

private:
  SetCond &setCond();
};


// Instruction id: also the index of an instruction
// in the main instruction sequence
typedef int InstrId;


// ============================================================================
// Handy functions
// ============================================================================

void check_zeroes(Seq<Instr> const &instrs);

namespace Target {
namespace instr {

extern Reg const None;
extern Reg const ACC0;
extern Reg const ACC1;
extern Reg const ACC2;
extern Reg const ACC3;
extern Reg const ACC4;
extern Reg const QPU_ID;
extern Reg const ELEM_ID;
extern Reg const TMU0_S;
extern Reg const VPM_WRITE;
extern Reg const VPM_READ;
extern Reg const WR_SETUP;
extern Reg const RD_SETUP;
extern Reg const DMA_LD_WAIT;
extern Reg const DMA_ST_WAIT;
extern Reg const DMA_LD_ADDR;
extern Reg const DMA_ST_ADDR;
extern Reg const SFU_RECIP;
extern Reg const SFU_RECIPSQRT;
extern Reg const SFU_EXP;
extern Reg const SFU_LOG;

// Following registers are synonyms for v3d code generation,
// to better indicate the intent. Definitions of vc4 concepts
// are reused here, in order to prevent the code getting into a mess.
extern Reg const TMUD;
extern Reg const TMUA;

Reg rf(uint8_t index);

Instr bor(Reg dst, Reg srcA, Reg srcB);
Instr band(Reg dst, Reg srcA, Reg srcB);
Instr band(Var dst, Var srcA, Var srcB);
Instr band(Reg dst, Reg srcA, int n);
Instr bxor(Var dst, Var srcA, int n);
Instr mov(Var dst, Var src);
Instr mov(Var dst, Reg src);
Instr mov(Var dst, int n);
Instr mov(Reg dst, Var src);
Instr mov(Reg dst, int n);
Instr mov(Reg dst, Reg src);
Instr shl(Reg dst, Reg srcA, int val);
Instr add(Reg dst, Reg srcA, Reg srcB);
Instr add(Reg dst, Reg srcA, int n);
Instr sub(Reg dst, Reg srcA, int n);
Instr shr(Reg dst, Reg srcA, int n);
Instr li(Reg dst, int i);
Instr li(Var v, int i);
Instr li(Var v, float f);
Instr branch(Label label);
Instr branch(BranchCond cond, Label label);
Instr label(Label in_label);

// SFU functions
Seq<Instr> recip(Var dst, Var srcA);
Seq<Instr> recipsqrt(Var dst, Var srcA);
Seq<Instr> bexp(Var dst, Var srcA);
Seq<Instr> blog(Var dst, Var srcA);

// v3d only
Instr tmuwt();

}  // namespace instr
}  // namespace Target

}  // namespace V3DLib

#endif  // _V3DLIB_TARGET_SYNTAX_H_
