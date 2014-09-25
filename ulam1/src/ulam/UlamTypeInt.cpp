#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "UlamTypeInt.h"
#include "UlamValue.h"
#include "CompilerState.h"

namespace MFM {

  UlamTypeInt::UlamTypeInt(const UlamKeyTypeSignature key, const UTI uti) : UlamType(key, uti)
  {}


   ULAMTYPE UlamTypeInt::getUlamTypeEnum()
   {
     return Int;
   }


  const std::string UlamTypeInt::getUlamTypeAsStringForC()
  {
    //std::ostringstream ctype;
    //ctype <<  "s" << m_key.getUlamKeyTypeSignatureBitSize(); 
    //return ctype.str();
    return "int";
  }


  const char * UlamTypeInt::getUlamTypeAsSingleLowercaseLetter()
  {
    return "i";
  }


  bool UlamTypeInt::cast(UlamValue & val, CompilerState& state)
  {
    bool brtn = true;
    UTI typidx = getUlamTypeIndex();
    UTI valtypidx = val.getUlamValueTypeIdx();    
    s32 arraysize = getArraySize();
    if(arraysize != state.getArraySize(valtypidx))
      {
	std::ostringstream msg;
	msg << "Casting different Array sizes; " << arraysize << ", Value Type and size was: " << valtypidx << "," << state.getArraySize(valtypidx);
	state.m_err.buildMessage("", msg.str().c_str(),__FILE__, __func__, __LINE__, MSG_ERR);
	return false;
      }
    
    //change the size first of tobe, if necessary
    s32 bitsize = getBitSize();
    s32 valbitsize = state.getBitSize(valtypidx);

    //base types e.g. Int, Bool, Unary, Foo, Bar..
    ULAMTYPE typEnum = getUlamTypeEnum();
    ULAMTYPE valtypEnum = state.getUlamTypeByIndex(valtypidx)->getUlamTypeEnum();

    if((bitsize != valbitsize) && (typEnum != valtypEnum))
      {
	//change to val's size, within the TOBE current type; 
	//get string index for TOBE enum type string
	u32 enumStrIdx = state.m_pool.getIndexForDataString(UlamType::getUlamTypeEnumAsString(typEnum));
	UlamKeyTypeSignature vkey1(enumStrIdx, valbitsize, arraysize);
	UTI vtype1 = state.makeUlamType(vkey1, typEnum); //may not exist yet, create  
	
	if(!(state.getUlamTypeByIndex(vtype1)->cast(val,state))) //val changes!!!
	  {
	    //error! 
	    return false;
	  }
	
	valtypidx = val.getUlamValueTypeIdx();  //reload
	valtypEnum = state.getUlamTypeByIndex(valtypidx)->getUlamTypeEnum();
      }
	
    if(state.isConstant(typidx))
      {
	// avoid using out-of-band value as bitsize
	bitsize = state.getDefaultBitSize(typidx);
      }

    u32 data = val.getImmediateData(state);
    switch(valtypEnum)
      {
      case Bool:
	{
	  if(state.isConstant(valtypidx))  // bitsize is misleading
	    {
	      if(data != 0) //signed or unsigned
		val = UlamValue::makeImmediate(typidx, 1, state); //overwrite val
	      else
		val = UlamValue::makeImmediate(typidx, 0, state); //overwrite val
	    }
	  else
	    {
	      s32 count1s = PopCount(data);
	      if(count1s > (s32) (valbitsize - count1s))
		val = UlamValue::makeImmediate(typidx, 1, state); //overwrite val
	      else
		val = UlamValue::makeImmediate(typidx, 0, state); //overwrite val
	    }
	}
	break;
      case Unary:
	{
	  u32 count1s = PopCount(data);
	  val = UlamValue::makeImmediate(typidx, count1s, state); //overwrite val
	}
	break;
      case Int:
      case Unsigned:
      case Bits:
	// casting Int to Int to change bits size
	// casting Unsigned to Int to change type
	// casting Bits to Int to change type
	val = UlamValue::makeImmediate(typidx, data, state); //overwrite val
	break;
      default:
	assert(0);
	//std::cerr << "UlamTypeInt (cast) error! Value Type was: " << valtypidx << std::endl;
	brtn = false;
      };
    return brtn;
  } //end cast


  void UlamTypeInt::getDataAsString(const u32 data, char * valstr, char prefix, CompilerState& state)
  {
    if(prefix == 'z')
      sprintf(valstr,"%d", (s32) data);
    else
      sprintf(valstr,"%c%d", prefix, (s32) data);
  }
  
} //end MFM
