#ifndef _V3DLIB_SOURCE_COND_H_
#define _V3DLIB_SOURCE_COND_H_
#include "Int.h"
#include "Float.h"
#include "CExpr.h"

namespace V3DLib {

// ============================================================================
// Types                   
// ============================================================================

//class CExpr;  //  forward declaration
//class BExpr;  //  forward declaration

struct Cond {
  CExpr::Ptr cexpr() { return m_cexpr; } 

  Cond(CExpr::Ptr c) { m_cexpr = c; }

private:
  CExpr::Ptr m_cexpr; // Abstract syntax tree
};


struct BoolExpr {
  BoolExpr(BExpr::Ptr b) :m_bexpr(b) {}

  BExpr::Ptr bexpr() { return m_bexpr; }

private:
  BExpr::Ptr m_bexpr;  // Abstract syntax tree
};


BExpr::Ptr mkCmp(Expr::Ptr lhs, CmpOp op, Expr::Ptr rhs);


// ============================================================================
// Specific 'Int' comparisons
// ============================================================================

BoolExpr operator==(IntExpr a, IntExpr b);
BoolExpr operator!=(IntExpr a, IntExpr b);
BoolExpr operator<(IntExpr a, IntExpr b);
BoolExpr operator<=(IntExpr a, IntExpr b);
BoolExpr operator>(IntExpr a, IntExpr b);
BoolExpr operator>=(IntExpr a, IntExpr b);


// ============================================================================
// Specific 'Float' comparisons
// ============================================================================

BoolExpr operator==(FloatExpr a, FloatExpr b);
BoolExpr operator!=(FloatExpr a, FloatExpr b);
BoolExpr operator<(FloatExpr a, FloatExpr b);
BoolExpr operator<=(FloatExpr a, FloatExpr b);
BoolExpr operator>(FloatExpr a, FloatExpr b);
BoolExpr operator>=(FloatExpr a, FloatExpr b);


// ============================================================================
// Boolean operators
// ============================================================================

BoolExpr operator!(BoolExpr a);
BoolExpr operator&&(BoolExpr a, BoolExpr b);
BoolExpr operator||(BoolExpr a, BoolExpr b);
BoolExpr operator!=(BoolExpr a, BoolExpr b);

Cond any(BoolExpr a);
Cond all(BoolExpr a);

}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_COND_H_
