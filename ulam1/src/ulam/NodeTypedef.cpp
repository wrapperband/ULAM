#include <stdlib.h>
#include "NodeTypedef.h"
#include "CompilerState.h"


namespace MFM {

  NodeTypedef::NodeTypedef(SymbolTypedef * sym, CompilerState & state) : Node(state), m_typedefSymbol(sym) {}

  NodeTypedef::~NodeTypedef(){}

  void NodeTypedef::printPostfix(File * fp)
  {
    fp->write(" typedef");

    fp->write(" ");
    fp->write(m_state.getUlamTypeNameBriefByIndex(m_typedefSymbol->getUlamTypeIdx()).c_str());
    fp->write(" ");
    fp->write(getName());

    s32 arraysize = m_state.getArraySize(m_typedefSymbol->getUlamTypeIdx());
    if(arraysize > NONARRAYSIZE)
      {
	fp->write("[");
	fp->write_decimal(arraysize);
	fp->write("]");
      }

    fp->write("; ");
  }


  const char * NodeTypedef::getName()
  {
    //same as m_typedefSymbol->getUlamType()->getUlamTypeNameBrief()); //short type name
    return m_state.m_pool.getDataAsString(m_typedefSymbol->getId()).c_str();
  }


  const std::string NodeTypedef::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }


  UTI NodeTypedef::checkAndLabelType()
  {
    UTI it;
    if(!m_typedefSymbol)
      {
	MSG("","Typedef symbol is missing",ERR);
	it = Nav;
      }
    else
      {
	it = m_typedefSymbol->getUlamTypeIdx();
	//check for incomplete Classes
	UlamType * tdut = m_state.getUlamTypeByIndex(it);
	ULAMCLASSTYPE tdclasstype = tdut->getUlamClass();
	//	if(tdclasstype == UC_INCOMPLETE || (tdclasstype != UC_NOTACLASS && !tdut->isComplete()))
	if(tdclasstype == UC_INCOMPLETE)
	  {
	    if(!m_state.completeIncompleteClassSymbol(it))
	      {
		std::ostringstream msg;
		msg << "Incomplete Typedef for class type: " << m_state.getUlamTypeNameByIndex(it).c_str() << " used with variable symbol name <" << getName() << ">";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
	      }
	  }
	else if(!tdut->isComplete())
	  {
	    m_state.constantFoldIncompleteUTI(it); //update if possible
	    tdut = m_state.getUlamTypeByIndex(it); //reload
	    if(!tdut->isComplete())
	      {
		std::ostringstream msg;
		msg << "Incomplete Typedef for type: " << m_state.getUlamTypeNameByIndex(it).c_str() << " used with variable symbol name <" << getName() << ">";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
	      }
	  }
      }
    setNodeType(it);
    return getNodeType();
  } //checkAndLabelType


  EvalStatus NodeTypedef::eval()
  {
    assert(m_typedefSymbol);
    return NORMAL;
  }


  bool NodeTypedef::getSymbolPtr(Symbol *& symptrref)
  {
    symptrref = m_typedefSymbol;
    return true;
  }


  void NodeTypedef::packBitsInOrderOfDeclaration(u32& offset)
  {
    //do nothing ???
  }


  void NodeTypedef::genCode(File * fp, UlamValue& uvpass)
  {
#if 0
    m_state.indent(fp);
    fp->write("typedef ");

    fp->write(m_state.getUlamTypeByIndex(m_typedefSymbol->getUlamTypeIdx())->getUlamTypeMangledName().c_str()); //for C++
    fp->write(" ");
    fp->write(getName());

    s32 arraysize = m_state.getArraySize(m_typedefSymbol->getUlamTypeIdx());
    if(arraysize > NONARRAYSIZE)
      {
	fp->write("[");
	fp->write_decimal(arraysize);
	fp->write("]");
      }
    fp->write(";\n");
#endif
  }


} //end MFM


