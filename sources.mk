#
# This file is generated!  Editing it directly is a bad idea.
#
# Generated on: Sun 17 Jan 2021 04:37:23 AM CET
#
###############################################################################

# Library Object files - only used for LIB
OBJ := \
  Target/Subst.o  \
  Target/BufferObject.o  \
  Target/Pretty.o  \
  Target/instr/ALUOp.o  \
  Target/instr/Conditions.o  \
  Target/Reg.o  \
  Target/SmallLiteral.o  \
  Target/EmuSupport.o  \
  Target/Liveness.o  \
  Target/Emulator.o  \
  Target/Satisfy.o  \
  Target/Instr.o  \
  Target/CFG.o  \
  Target/Syntax.o  \
  Source/Lang.o  \
  Source/Cond.o  \
  Source/Interpreter.o  \
  Source/Translate.o  \
  Source/Pretty.o  \
  Source/BExpr.o  \
  Source/Int.o  \
  Source/gather.o  \
  Source/Op.o  \
  Source/Expr.o  \
  Source/StmtStack.o  \
  Source/CExpr.o  \
  Source/Float.o  \
  Source/Var.o  \
  Source/Stmt.o  \
  Support/debug.o  \
  Support/InstructionComment.o  \
  Support/Platform.o  \
  Support/HeapManager.o  \
  SourceTranslate.o  \
  Common/BufferObject.o  \
  v3d/PerformanceCounters.o  \
  v3d/v3d.o  \
  v3d/BufferObject.o  \
  v3d/SourceTranslate.o  \
  v3d/instr/RFAddress.o  \
  v3d/instr/Snippets.o  \
  v3d/instr/SmallImm.o  \
  v3d/instr/Register.o  \
  v3d/instr/Instr.o  \
  v3d/Driver.o  \
  v3d/Invoke.o  \
  v3d/RegisterMapping.o  \
  v3d/KernelDriver.o  \
  vc4/PerformanceCounters.o  \
  vc4/Mailbox.o  \
  vc4/BufferObject.o  \
  vc4/Translate.o  \
  vc4/SourceTranslate.o  \
  vc4/RegAlloc.o  \
  vc4/Invoke.o  \
  vc4/RegisterMap.o  \
  vc4/Encode.o  \
  vc4/DMA.o  \
  vc4/LoadStore.o  \
  vc4/vc4.o  \
  vc4/KernelDriver.o  \
  Kernel.o  \
  KernelDriver.o  \
  v3d/instr/dump_instr.o  \
  vc4/dump_instr.o  \

# All programs in the Examples *and Tools* directory
EXAMPLES := \
  HeatMap  \
  OET  \
  Print  \
  Tri  \
  ID  \
  ReqRecv  \
  GCD  \
  Hello  \
  Mandelbrot  \
  DMA  \
  Rot3D  \
  detectPlatform  \

# support files for examples
EXAMPLES_EXTRA := \
  Examples/Rot3DLib/Rot3DKernels.o  \
  Examples/Support/Timer.o  \
  Examples/Support/Settings.o  \
  
# support files for tests
TESTS_FILES := \
  Tests/testRegMap.o  \
  Tests/testBO.o  \
  Tests/testV3d.o  \
  Tests/testRot3D.o  \
  Tests/support/disasm_kernel.o  \
  Tests/support/rotate_kernel.o  \
  Tests/support/Gen.o  \
  Tests/support/support.o  \
  Tests/support/summation_kernel.o  \
  Tests/testSFU.o  \
  Tests/testConditionCodes.o  \
  Tests/testMain.o  \
  Tests/testDSL.o  \
  Tests/testAutoTest.o  \
  Tests/testCmdLine.o  \
  Tests/support/qpu_disasm.o  \
