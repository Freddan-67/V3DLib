#include "SourceTranslate.h"
#include "Support/basics.h"
#include "Source/Translate.h"  // srcReg()
#include "Source/Stmt.h"  // srcReg()
#include "Target/Liveness.h"
#include "Target/Subst.h"
#include "vc4/DMA/DMA.h"

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace {


/**
 * Generate code to add an offset to the uniforms which are pointers.
 *
 * The calculated offset is assumed to be in ACC0
 */
Instr::List add_uniform_pointer_offset(Instr::List &code) {
  using namespace V3DLib::Target::instr;

  Instr::List ret;

  // add the offset to all the uniform pointers
  for (int index = 0; index < code.size(); ++index) {
    auto &instr = code[index];

    if (!instr.isUniformLoad()) {  // Assumption: uniform loads always at top
      break;
    }

    if (instr.ALU.srcA.tag == REG && instr.ALU.srcA.reg.isUniformPtr) {
      ret << add(rf((uint8_t) index), rf((uint8_t) index), ACC0);
    }
  }

  return ret;
}


/**
 * @param seq  list of generated instructions up till now
 */
void storeRequest(Instr::List &seq, Expr::Ptr data, Expr::Ptr addr) {
  using namespace V3DLib::Target::instr;

  if (addr->tag() != Expr::VAR || data->tag() != Expr::VAR) {
    addr = putInVar(&seq, addr);
    data = putInVar(&seq, data);
  }

  Reg srcAddr = srcReg(addr->var());
  Reg srcData = srcReg(data->var());

  seq << mov(TMUD, srcData);
  seq.back().comment("Store request");
  seq << mov(TMUA, srcAddr)
      << tmuwt();
}

}  // anon namespace


namespace v3d {

/**
 * Case: *v := rhs where v is a var and rhs is a var
 */
Instr::List SourceTranslate::deref_var_var(Var lhs, Var rhs) {
  using namespace V3DLib::Target::instr;

  Instr::List ret;

  Reg srcData = srcReg(lhs);
  Reg srcAddr = srcReg(rhs);

  if (rhs.tag() == ELEM_NUM) {
    //TODO: is ACC0 safe here?
    assert(srcAddr == ELEM_ID);
    ret << mov(ACC0, ELEM_ID)
        << mov(TMUD, ACC0);
  } else {
    ret << mov(TMUD, srcAddr);
  }

  ret << mov(TMUA, srcData)
      << tmuwt();

  return ret;
}


/**
 * Case: v := *w where w is a variable
 */
void SourceTranslate::varassign_deref_var(Instr::List* seq, Var &v, Expr &e) {
  using namespace V3DLib::Target::instr;
  assert(seq != nullptr);

  Instr ldtmu_r4;
  ldtmu_r4.tag = TMU0_TO_ACC4;

  Reg src = srcReg(e.deref_ptr()->var());
  *seq << mov(TMU0_S, src)

       // TODO: Do we need NOP's here?
       // TODO: Check if more fields need to be set
       // TODO is r4 safe? Do we need to select an accumulator in some way?
       << Instr::nop()
       << Instr::nop()
       << ldtmu_r4
       << mov(dstReg(v), ACC4);
}


void SourceTranslate::regAlloc(CFG* cfg, Instr::List* instrs) {
  int numVars = getFreshVarCount();

  // Step 0
  // Perform liveness analysis
  Liveness live(*cfg);
  live.compute(*instrs);
  assert(instrs->size() == live.size());

  // Step 2
  // For each variable, determine all variables ever live at the same time
  LiveSets liveWith(numVars);
  liveWith.init(*instrs, live);

  // Step 3
  // Allocate a register to each variable
  std::vector<Reg> alloc(numVars);
  for (int i = 0; i < numVars; i++) alloc[i].tag = NONE;

  // Allocate registers to the variables
  for (int i = 0; i < numVars; i++) {
    auto possible = liveWith.possible_registers(i, alloc);

    alloc[i].tag = REG_A;
    RegId regId = LiveSets::choose_register(possible, false);

    if (regId < 0) {
      std::string buf = "v3d regAlloc(): register allocation failed for target instruction ";
      buf << i << ": " << (*instrs)[i].mnemonic();
      error(buf, true);
    } else {
      alloc[i].regId = regId;
    }
  }

  // Step 4
  // Apply the allocation to the code
  for (int i = 0; i < instrs->size(); i++) {
    auto &useDefSet = liveWith.useDefSet;
    Instr &instr = instrs->get(i);

    useDef(instr, &useDefSet);
    for (int j = 0; j < useDefSet.def.size(); j++) {
      RegId r = useDefSet.def[j];
      renameDest(&instr, REG_A, r, TMP_A, alloc[r].regId);
    }
    for (int j = 0; j < useDefSet.use.size(); j++) {
      RegId r = useDefSet.use[j];
      renameUses(&instr, REG_A, r, TMP_A, alloc[r].regId);
    }
    substRegTag(&instr, TMP_A, REG_A);
  }
}


Instr label(Label in_label) {
  Instr instr;
  instr.tag = LAB;
  instr.label(in_label);

  return instr;
}


/**
 * Add extra initialization code after uniform loads
 */
void add_init(Instr::List &code) {
  using namespace V3DLib::Target::instr;

  int insert_index = code.tag_index(INIT_BEGIN);
  assertq(insert_index >= 0, "Expecting init begin marker");

  Instr::List ret;
  Label endifLabel = freshLabel();

  // Determine the qpu index for 'current' QPU
  // This is derived from the thread index. 
  //
  // Broadly:
  //
  // If (numQPUs() == 8)  // Alternative is 1, then qpu num initalized to 0 is ok
  //   me() = (thread_index() >> 2) & 0b1111;
  // End
  //
  // This works because the thread indexes are consecutive for multiple reserved
  // threads. It's probably also the reason why you can select only 1 or 8 (max)
  // threads, otherwise there would be gaps in the qpu id.
  //
  ret << mov(rf(RSV_QPU_ID), 0)           // not needed, already init'd to 0. Left here to counter future brainfarts
      << sub(ACC0, rf(RSV_NUM_QPUS), 8).pushz()
      << branch(endifLabel).allzc()       // nop()'s added downstream
      << mov(ACC0, QPU_ID)
      << shr(ACC0, ACC0, 2)
      << band(rf(RSV_QPU_ID), ACC0, 15)
      << label(endifLabel)
  ;

  // offset = 4 * (thread_num + 16 * qpu_num);
  ret << shl(ACC1, rf(RSV_QPU_ID), 4) // Avoid ACC0 here, it's used for getting QPU_ID and ELEM_ID (next stmt)
      << mov(ACC0, ELEM_ID)
      << add(ACC1, ACC1, ACC0)
      << shl(ACC0, ACC1, 2)           // Post: offset now in ACC0
      << add_uniform_pointer_offset(code);

  code.insert(insert_index + 1, ret);  // Insert init code after the INIT_BEGIN marker
}


/**
 * @return true if statement handled, false otherwise
 */
bool SourceTranslate::stmt(Instr::List &seq, Stmt::Ptr s) {
  if (DMA::is_dma_tag(s->tag)) {
    fatal("VPM and DMA reads and writes can not be used for v3d");
    return true;
  }

  switch (s->tag) {
    case Stmt::STORE_REQUEST:
      storeRequest(seq, s->storeReq_data(), s->storeReq_addr());
      return true;

    default:
      return false;
  }
}

}  // namespace v3d
}  // namespace V3DLib
