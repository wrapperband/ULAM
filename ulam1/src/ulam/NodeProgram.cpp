#include <stdio.h>
#include <map>
#include "NodeProgram.h"
#include "CompilerState.h"

namespace MFM {

  NodeProgram::NodeProgram(u32 id, CompilerState & state) : Node(state), m_root(NULL), m_compileThisId(id) {}

  NodeProgram::~NodeProgram()
  {
    //m_root deletion handled by m_programDefST;
  }


  void NodeProgram::updateLineage(Node * p)
  {
    setYourParent(p);             //god is null
    m_root->updateLineage(this);  //walk the tree..
  }


  void NodeProgram::setRootNode(NodeBlockClass * root)
  {
    m_root = root;
  }


  void NodeProgram::print(File * fp)
  {
    printNodeLocation(fp);
    UTI myut = getNodeType();
    char id[255];
    if(myut == Nav)
      sprintf(id,"%s<NOTYPE>\n", prettyNodeName().c_str());
    else
      sprintf(id,"%s<%s>\n", prettyNodeName().c_str(), m_state.getUlamTypeNameByIndex(myut).c_str());
    fp->write(id);

    //overrides NodeBlock print, has m_root, no m_node or m_nextNode.
    if(m_root)
      m_root->print(fp);
    else
      fp->write("<NULL>\n");

    sprintf(id,"-----------------%s\n", prettyNodeName().c_str());
    fp->write(id);
  }


  void NodeProgram::printPostfix(File * fp)
  {
    m_state.m_classBlock = m_root;  //reset to compileThis' class block
    m_state.m_currentBlock = m_state.m_classBlock;

    if(m_root)
	m_root->printPostfix(fp);
    else
      fp->write("<NULL>\n");
  }


  const char * NodeProgram::getName()
  {
    return  m_state.m_pool.getDataAsString(m_compileThisId).c_str();
  }


  const std::string NodeProgram::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }


#define MAX_ITERATIONS 10
  UTI NodeProgram::checkAndLabelType()
  {
    assert(m_root);
    m_state.m_err.clearCounts();

    m_root->updateLineage(this);

    // type set at parse time (needed for square bracket checkandlabel);
    // so, here we just check for matching arg types.
    m_state.m_programDefST.checkCustomArraysForTableOfClasses();

    // label all the class; sets "current" m_currentClassSymbol in CS
    m_state.m_programDefST.labelTableOfClasses();

    if(m_state.m_err.getErrorCount() == 0)
      {
	u32 infcounter = 0;
	// size all the class; sets "current" m_currentClassSymbol in CS
	while(!m_state.m_programDefST.setBitSizeOfTableOfClasses())
	  {
	    if(++infcounter > MAX_ITERATIONS)
	      {
		std::ostringstream msg;
		msg << "Possible INCOMPLETE class detected during type labeling class <";
		msg << m_state.m_pool.getDataAsString(m_state.m_compileThisId);
		msg << ">, after " << infcounter << " iterations";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		break;
	      }
	  }

	u32 statcounter = 0;
	while(!m_state.statusUnknownBitsizeUTI())
	  {
	    if(++statcounter > MAX_ITERATIONS)
	      {
		std::ostringstream msg;
		msg << "Before bit packing, UNKNOWN types remain in class <";
		msg << m_state.m_pool.getDataAsString(m_state.m_compileThisId);
		msg << ">, after " << statcounter << " iterations";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		break;
	      }
	  }

	statcounter = 0;
	while(!m_state.statusUnknownArraysizeUTI())
	  {
	    if(++statcounter > MAX_ITERATIONS)
	      {
		std::ostringstream msg;
		msg << "Before bit packing, types with UNKNOWN arraysizes remain in class <";
		msg << m_state.m_pool.getDataAsString(m_state.m_compileThisId);
		msg << ">, after " << statcounter << " iterations";
		MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
		break;
	      }
	  }

	// count Nodes with illegal Nav types; walk each class' data members and funcdefs.
	m_state.m_programDefST.countNavNodesAcrossTableOfClasses();

	// must happen after type labeling and before code gen; separate pass.
	m_state.m_programDefST.packBitsForTableOfClasses();

	// let Ulam programmer know the bits used/available (needs infoOn)
	m_state.m_programDefST.printBitSizeOfTableOfClasses();
      }

    UTI rtnType =  m_root->getNodeType();
    setNodeType(rtnType);   //void type. missing?

    // reset m_current class block, for next stage
    m_state.m_classBlock = m_root;  //reset to compileThis' class block
    m_state.m_currentBlock = m_state.m_classBlock;

    u32 warns = m_state.m_err.getWarningCount();
    if(warns > 0)
      {
	std::ostringstream msg;
	msg << warns << " warnings during type labeling";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
      }

    u32 errs = m_state.m_err.getErrorCount();
    if(errs > 0)
      {
	std::ostringstream msg;
	msg << errs << " TOO MANY TYPELABEL ERRORS";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
      }

    return rtnType;
  } //checkAndLabelType


  //performed across classes starting at NodeBlockClass
  void NodeProgram::countNavNodes(u32& cnt)
  {
    assert(0);
    return;
  }


   EvalStatus NodeProgram::eval()
  {
    assert(m_root);
    m_state.m_err.clearCounts();

    m_state.m_classBlock = m_root;  //reset to compileThis' class block
    m_state.m_currentBlock = m_state.m_classBlock;

    setNodeType(Int);     //for testing

    evalNodeProlog(1);    //new current frame pointer for nodeeval stack
    EvalStatus evs = m_root->eval();

    // output informational warning and error counts
    u32 warns = m_state.m_err.getWarningCount();
    if(warns > 0)
      {
	std::ostringstream msg;
	msg << warns << " warnings during eval";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
      }

    u32 errs = m_state.m_err.getErrorCount();
    if(errs > 0)
      {
	std::ostringstream msg;
	msg << errs << " TOO MANY EVAL ERRORS";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), INFO);
      }

    if(evs == NORMAL)
      {
	UlamValue testUV = m_state.m_nodeEvalStack.popArg();
	assignReturnValueToStack(testUV);     //for testProgram in Compiler
      }

    evalNodeEpilog();
    return evs;
  } //eval


  void NodeProgram::generateCode(FileManager * fm)
  {
    assert(m_root);
    m_state.m_err.clearCounts();
    m_state.m_programDefST.genCodeForTableOfClasses(fm);
  } //generateCode


  // moved to Symbol Class!!!!!!!!!!!

#if 0
  void NodeProgram::generateCode(FileManager * fm)
  {
    assert(m_root);
    m_state.m_err.clearCounts();

    m_state.m_classBlock = m_root;  //reset to compileThis' class block
    m_state.m_currentBlock = m_state.m_classBlock;

    // mangled types and forward class declarations
    genMangledTypesHeaderFile(fm);

    // this class header
    {
      File * fp = fm->open(m_state.getFileNameForThisClassHeader(WSUBDIR).c_str(), WRITE);
      assert(fp);

      generateHeaderPreamble(fp);
      genAllCapsIfndefForHeaderFile(fp);
      generateHeaderIncludes(fp);

      UlamValue uvpass;
      m_root->genCode(fp, uvpass);      //compileThisId only, class block

      // include this .tcc
      m_state.indent(fp);
      fp->write("#include \"");
      fp->write(m_state.getFileNameForThisClassBody().c_str());
      fp->write("\"\n\n");

      // include native .tcc for this class if any declared
      if(m_root->countNativeFuncDecls() > 0)
	{
	  m_state.indent(fp);
	  fp->write("#include \"");
	  fp->write(m_state.getFileNameForThisClassBodyNative().c_str());
	  fp->write("\"\n\n");
	}

      genAllCapsEndifForHeaderFile(fp);

      delete fp; //close
    }

    // this class body
    {
      File * fp = fm->open(m_state.getFileNameForThisClassBody(WSUBDIR).c_str(), WRITE);
      assert(fp);

      m_state.m_currentIndentLevel = 0;
      fp->write(CModeForHeaderFiles);  //needed for .tcc files too

      UlamValue uvpass;
      m_root->genCodeBody(fp, uvpass);  //compileThisId only, MFM namespace

      delete fp;  //close
    }

    // "stub" .cpp includes .h (unlike the .tcc body)
    {
      File * fp = fm->open(m_state.getFileNameForThisClassCPP(WSUBDIR).c_str(), WRITE);
      assert(fp);

      m_state.m_currentIndentLevel = 0;

      // include .h in the .cpp
      m_state.indent(fp);
      fp->write("#include \"");
      fp->write(m_state.getFileNameForThisClassHeader().c_str());
      fp->write("\"\n");
      fp->write("\n");

      delete fp; //close
    }

    //separate main.cpp for elements only; that have the test method.
    if(m_state.getUlamTypeByIndex(m_root->getNodeType())->getUlamClass() == UC_ELEMENT)
      {
	if(m_state.thisClassHasTheTestMethod())
	  generateMain(fm);
      }
  } //generateCode


  void NodeProgram::generateHeaderPreamble(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    fp->write(CModeForHeaderFiles);
    fp->write("/***********************         DO NOT EDIT        ******************************\n");
    fp->write("*\n");
    fp->write("* ");
    fp->write(m_state.m_pool.getDataAsString(m_state.m_compileThisId).c_str());
    fp->write(".h - ");
    ULAMCLASSTYPE classtype = m_state.getUlamTypeByIndex(m_root->getNodeType())->getUlamClass();
    if(classtype == UC_ELEMENT)
      fp->write("Element");
    else if(classtype == UC_QUARK)
      fp->write("Quark");
    else
      assert(0);

    fp->write(" header for ULAM\n");

    fp->write(CopyrightAndLicenseForUlamHeader);
  } //generateHeaderPreamble


  void NodeProgram::genAllCapsIfndefForHeaderFile(File * fp)
  {
    UlamType * cut = m_state.getUlamTypeByIndex(m_root->getNodeType());
    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(Node::allCAPS(cut->getUlamTypeMangledName(&m_state).c_str()).c_str());
    fp->write("_H\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(Node::allCAPS(cut->getUlamTypeMangledName(&m_state).c_str()).c_str());
    fp->write("_H\n\n");
  } //genAllCapsIfndefForHeaderFile


  void NodeProgram::genAllCapsEndifForHeaderFile(File * fp)
  {
    UlamType * cut = m_state.getUlamTypeByIndex(m_root->getNodeType());
    fp->write("#endif //");
    fp->write(Node::allCAPS(cut->getUlamTypeMangledName(&m_state).c_str()).c_str());
    fp->write("_H\n");
  }


  void NodeProgram::generateHeaderIncludes(File * fp)
  {
    m_state.indent(fp);
    fp->write("#include \"UlamDefs.h\"\n\n");

    //using the _Types.h file
    m_state.indent(fp);
    fp->write("#include \"");
    fp->write(m_state.getFileNameForThisTypesHeader().c_str());
    fp->write("\"\n");
    fp->write("\n");

    //generate includes for all the other classes that have appeared
    m_state.m_programDefST.generateForwardDefsForTableOfClasses(fp);
  } //generateHeaderIncludes


  // create structs with BV, as storage, and typedef
  // for primitive types; useful as args and local variables;
  // important for overloading functions
  void NodeProgram::genMangledTypesHeaderFile(FileManager * fm)
  {
    File * fp = fm->open(m_state.getFileNameForThisTypesHeader(WSUBDIR).c_str(), WRITE);
    assert(fp);

    m_state.m_currentIndentLevel = 0;
    fp->write(CModeForHeaderFiles);

    m_state.indent(fp);
    //use -I ../../../include in g++ command
    fp->write("//#include \"itype.h\"\n");
    fp->write("//#include \"BitVector.h\"\n");
    fp->write("//#include \"BitField.h\"\n");
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#include \"UlamDefs.h\"\n\n");

    // do primitive types before classes so that immediate
    // Quarks/Elements can use them (e.g. immediate index for aref)

    std::map<UlamKeyTypeSignature, UlamType *, less_than_key>::iterator it = m_state.m_definedUlamTypes.begin();
    while(it != m_state.m_definedUlamTypes.end())
      {
	UlamType * ut = it->second;
	if(ut->needsImmediateType() && ut->getUlamClass() == UC_NOTACLASS)   //e.g. skip constants, incl atom
	  ut->genUlamTypeMangledDefinitionForC(fp, &m_state);
	it++;
      }

    //same except now for user defined Class types
    it = m_state.m_definedUlamTypes.begin();
    while(it != m_state.m_definedUlamTypes.end())
      {
	UlamType * ut = it->second;
	ULAMCLASSTYPE classtype = ut->getUlamClass();
	if(ut->needsImmediateType() && classtype != UC_NOTACLASS)
	  {
	    ut->genUlamTypeMangledDefinitionForC(fp, &m_state);
	    if(classtype == UC_QUARK)
	      ut->genUlamTypeMangledAutoDefinitionForC(fp, &m_state);
	  }
	it++;
      }
    delete fp; //close
  } //genMangledTypeHeaderFile


  // append main to .cpp for debug useage
  // outside the MFM namespace !!!
  void NodeProgram::generateMain(FileManager * fm)
  {
    File * fp = fm->open(m_state.getFileNameForThisClassMain(WSUBDIR).c_str(), WRITE);
    assert(fp);

    m_state.m_currentIndentLevel = 0;

    m_state.indent(fp);
    fp->write("#include <stdio.h>\n\n");

    m_state.indent(fp);
    fp->write("#include \"UlamDefs.h\"\n\n");

    m_state.indent(fp);
    fp->write("//includes Element.h\n");
    m_state.indent(fp);
    fp->write("#include \"");
    fp->write(m_state.getFileNameForThisClassHeader().c_str());
    fp->write("\"\n");

    m_state.m_programDefST.generateIncludesForTableOfClasses(fp); //the other classes

    //MAIN STARTS HERE !!!
    fp->write("\n");
    m_state.indent(fp);
    fp->write("int main()\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("enum { SIZE = ");
    fp->write_decimal(BITSPERATOM);
    fp->write(" };\n");

    m_state.indent(fp);
    fp->write("typedef MFM::ParamConfig<SIZE> OurParamConfig;\n");

    m_state.indent(fp);
    fp->write("typedef MFM::P3Atom<OurParamConfig> OurAtom;\n");

    m_state.indent(fp);
    fp->write("typedef MFM::CoreConfig<OurAtom, OurParamConfig> OurCoreConfig;\n");
    m_state.indent(fp);
    fp->write("typedef MFM::UlamContext<OurCoreConfig> OurUlamContext;\n");
    m_state.indent(fp);
    fp->write("typedef MFM::Tile<OurCoreConfig> OurTile;\n");
    m_state.indent(fp);
    fp->write("OurTile theTile;\n");

    m_state.indent(fp);
    fp->write("OurUlamContext uc;\n");
    m_state.indent(fp);
    fp->write("uc.SetTile(theTile);\n");

    //declare an instance of all element classes; supports immediate types constructors
    std::string runThisTest = m_state.m_programDefST.generateTestInstancesForTableOfClasses(fp);

    m_state.indent(fp);
    fp->write("MFM::Ui_Ut_102323Int rtn;\n");

    m_state.indent(fp);
    fp->write("rtn = ");
    fp->write(runThisTest.c_str());  //uses hardcoded mangled test name
    fp->write(";\n");

#if 0
    // output for t3200 (before System native), compile gen code & run: ./main
    m_state.indent(fp);
    fp->write("printf(\"Bar1 toInt = %d\\n\", OurFoo::Ut_Um_4bar1::Uf_5toInt(uc, fooAtom).read());\n");
    m_state.indent(fp);
    fp->write("printf(\"Bar2 toInt = %d\\n\", OurFoo::Ut_Um_4bar2::Uf_5toInt(uc, fooAtom).read());\n");
    //Int(4) maxes out at 7, not 12.
#endif

    m_state.indent(fp);
    //fp->write("return 0;\n");
    fp->write("return rtn.read();\n");         // useful to return result of test

    m_state.m_currentIndentLevel--;

    m_state.indent(fp);
    fp->write("}\n");
    delete fp; //close
  } //generateMain
#endif

} //end MFM
