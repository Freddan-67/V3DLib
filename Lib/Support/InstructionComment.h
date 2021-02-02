#ifndef _LIB_COMMON_INSTRUCTIONCOMMENT_H
#define _LIB_COMMON_INSTRUCTIONCOMMENT_H
#include <string>

namespace V3DLib {

/**
 * Mixin for instruction comments
 */
class InstructionComment {
public:
  void transfer_comments(InstructionComment const &rhs);
  void clear_comments();
  std::string const &header() const { return m_header; }
  std::string const &comment() const { return m_comment; }

  std::string emit_header() const;
  std::string emit_comment(int instr_size) const;

protected:
  void header(std::string const &msg);
  void comment(std::string msg);

private:
  std::string m_header;
  std::string m_comment;
};

}  // namespace V3DLib

#endif  // _LIB_COMMON_INSTRUCTIONCOMMENT_H
