#include "formula.h"
#include <ostream>
#include <stdexcept>

std::vector<std::string> collectAtoms(const Node*) { throw std::logic_error("not implemented"); }
bool evaluateRec(const Node*, const Assignment&) { throw std::logic_error("not implemented"); }
bool evaluateIter(const Node*, const Assignment&) { throw std::logic_error("not implemented"); }
void printTruthTable(const Node*, std::ostream&, int) { throw std::logic_error("not implemented"); }
std::string truthTableColumn(const Node*, int) { throw std::logic_error("not implemented"); }
