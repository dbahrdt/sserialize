#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateWAH.h>
#include <sserialize/iterator/UDWIterator.h>
#include <sserialize/iterator/UDWConstrainedIterator.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/storage/pack_unpack_functions.h>
#include <iostream>

namespace sserialize {

ItemIndexPrivateWAH::ItemIndexPrivateWAH(const UByteArrayAdapter & data) :
m_curData(data+8, data.getUint32(0)/4),
m_fullData(data, m_curData.size()+2),
m_size(data.getUint32(4)),
m_curId(0),
m_cache(UByteArrayAdapter::createCache(std::min<uint32_t>(1024, m_size*4), sserialize::MM_PROGRAM_MEMORY) )
{}

ItemIndexPrivateWAH::ItemIndexPrivateWAH(UDWIterator data) :
m_curId(0)
{
	uint32_t cpCount = data.next()/4;
	m_size = data.next();
	m_cache = UByteArrayAdapter::createCache(std::min<uint32_t>(1024, m_size*4), sserialize::MM_PROGRAM_MEMORY);
	m_curData = UDWConstrainedIterator(data.getPrivate(), cpCount);
	data.reset();
	m_fullData = UDWConstrainedIterator(data.getPrivate(), cpCount+2);
}

ItemIndexPrivateWAH::ItemIndexPrivateWAH() : m_size(0), m_curId(0){}

ItemIndexPrivateWAH::~ItemIndexPrivateWAH() {}

ItemIndex::Types ItemIndexPrivateWAH::type() const {
	return ItemIndex::T_WAH;
}

uint32_t ItemIndexPrivateWAH::find(uint32_t id) const {
	return sserialize::ItemIndexPrivate::find(id);
}

UDWConstrainedIterator ItemIndexPrivateWAH::dataIterator() const {
	UDWConstrainedIterator tmp(m_fullData);
	tmp.next(); //extract cpCount
	tmp.next(); //extract size
	return tmp;
}

uint32_t ItemIndexPrivateWAH::at(uint32_t pos) const {
	if (!size() || size() <= pos)
		return 0;
	
	for(;m_cache.tellPutPtr()/4 <= pos;) {
// 	while(m_curData.hasNext()) {
		uint32_t val = m_curData.next();
		if (val & 0x1) { //rle encoded
			val >>= 1;//move out indicator bit
			if (val & 0x1) {//all ones, push corresponding ids
				uint32_t count = (val >> 1)*31;
				uint32_t curId = m_curId;
				for(std::size_t i = 0; i < count; ++i) {
					m_cache.putUint32(curId);
					++curId;
				}
			}
			val >>= 1;
			m_curId += val*31;
		}
		else {//no rle encoding, full 31 bits are use to encode values
			val >>= 1; //move out the indicator bit
			uint32_t curId = m_curId;
			while (val) {
				if (val & 0x1) {
					m_cache.putUint32(curId);
				}
				++curId;
				val >>= 1;
			}
			m_curId += 31;
		}
	}
	return m_cache.getUint32(pos*4);
}

uint32_t ItemIndexPrivateWAH::first() const {
	if (size())
		return at(0);
	return 0;
}

uint32_t ItemIndexPrivateWAH::last() const {
	if (size())
		return at(size()-1);
	return 0;
}

uint32_t ItemIndexPrivateWAH::size() const {
	return m_size;
}

uint8_t ItemIndexPrivateWAH::bpn() const { return 0; }

UByteArrayAdapter::SizeType ItemIndexPrivateWAH::getSizeInBytes() const {
	return m_fullData.dataSize();
}

void ItemIndexPrivateWAH::putInto(DynamicBitSet & bitSet) const {
	if (!size()) {
		return;
	}
	UByteArrayAdapter & destData = bitSet.data();
	uint32_t destBitPos = 0;
	
	UDWConstrainedIterator dataIt = dataIterator();
	while(dataIt.hasNext()) {
		uint32_t destBytePos = destBitPos/8;
		uint32_t destInBytePos = destBitPos%8;
		uint32_t val = dataIt.next();
		
		if (val & 0x1) { //rle encoded
			val >>= 1;//move out indicator bit
			if (val & 0x1) {//all ones, push corresponding ids
				uint32_t bitCount = (val >> 1)*31;
				{
					uint32_t fdestBitPos = destBitPos + bitCount;
					uint32_t fdestMaxBytePos = fdestBitPos/8 + (fdestBitPos%8 > 0);
					if (destData.size() <= fdestMaxBytePos) {
						destData.resize(fdestMaxBytePos+1);
					}
				}
				destData[destBytePos] |= ~createMask(destInBytePos);
				destBytePos += 1;
				bitCount -= 8-destInBytePos;
				uint32_t remainingFullBytes = bitCount/8;
				uint32_t remainingBits = bitCount%8;
				
				destData.fill(uint8_t(0xFFFF), destBytePos, remainingFullBytes);
				if (remainingBits) {
					destBytePos += remainingFullBytes;
					destData[destBytePos] |= createMask(remainingBits);
				}
			}
			val >>= 1; //move out sign bit
			destBitPos += 31*val;
		}
		else {//no rle encoding, full 31 bits are used to encode values
			uint32_t bitCount = 31;
			{
				uint32_t fdestBitPos = destBitPos + bitCount;
				uint32_t fdestMaxBytePos = fdestBitPos/8 + (fdestBitPos%8 > 0);
				if (destData.size() <= fdestMaxBytePos) {
					destData.resize(fdestMaxBytePos+1);
				}
			}
			
			val >>= 1; //move out the indicator bit
			destData[destBytePos] |= uint8_t(val << destInBytePos);
			
			destBytePos += 1;
			bitCount -= 8-destInBytePos;
			val >>= 8-destInBytePos;
			
			uint32_t remainingBytes = bitCount/8 + (bitCount%8 > 0);
			for(uint32_t i(0); i < remainingBytes; val >>= 8, destBytePos += 1, ++i) {
				destData[destBytePos] |= val;
			}
			destBitPos += 31;
		}
	}
}

ItemIndexPrivate * ItemIndexPrivateWAH::fromBitSet(const DynamicBitSet & bitSet) {
	const UByteArrayAdapter & bitSetData = bitSet.data();
	UByteArrayAdapter tmpData(UByteArrayAdapter::createCache(bitSet.data().size(), sserialize::MM_PROGRAM_MEMORY));
	tmpData.putUint32(0); //dummy data size
	tmpData.putUint32(0); //dummy count
	
	uint32_t count = 0;
	uint32_t initShift = 0;
	
	uint32_t curEncWord = 0;
	uint32_t bitSetDataOffset = 0;
	uint32_t bitSetDataSize = (uint32_t) bitSetData.size();
	for(;bitSetDataOffset < bitSetDataSize;) {
		uint32_t val = 0;
		{
			val = static_cast<uint32_t>(bitSetData.at(bitSetDataOffset)) >> initShift;
			uint8_t curShift = 8 - initShift;
			++bitSetDataOffset;
			uint8_t bitsRead = 8 - initShift;
			while (bitSetDataOffset < bitSetDataSize) {
				uint32_t newByte = bitSetData.at(bitSetDataOffset);
				newByte = (newByte << (curShift));
				val |= newByte;
				bitsRead += 8;
				if (bitsRead < 31) {
					++bitSetDataOffset;
					curShift += 8;
				}
				else {
					if (bitsRead == 31)
						++bitSetDataOffset;
					break;
				}
			}
			initShift = (initShift + 31) % 8;
		}
		val <<= 1; //moves out a possible false most-signifcant bit
		uint8_t bitCount = popCount(val);
		count += bitCount;
		if (bitCount == 31) {
			if ((curEncWord & 0x1) && !(curEncWord & 0x2)) { //this means there's an one-encoding rle ahead
				tmpData.putUint32(curEncWord);
				curEncWord = 0;
			}
			curEncWord >>= 2;
			++curEncWord;
			curEncWord <<= 2;
			curEncWord  |= 0x3;
		}
		else if (bitCount == 0) {
			if ((curEncWord & 0x1) && (curEncWord & 0x2)) { //this means there's an one-encoding rle ahead
				tmpData.putUint32(curEncWord);
				curEncWord = 0;
			}
			curEncWord >>= 2;
			++curEncWord;
			curEncWord <<= 2;
			curEncWord  |= 0x1;
		}
		else {
			if (curEncWord) { //there is a rle word before
				tmpData.putUint32(curEncWord);
				curEncWord = 0;
			}
			tmpData.putUint32(val); //val is already shifted one to the left
		}
	}
	if (curEncWord) {
		tmpData.putUint32(curEncWord);
	}
	tmpData.putUint32(0, (uint32_t) (tmpData.tellPutPtr()-8) );
	tmpData.putUint32(4, count);
	
	return new ItemIndexPrivateWAH(tmpData);
}

namespace {

struct OutPutHandler {
	UByteArrayAdapter & data;
	uint32_t outPutWord;
	uint32_t idCount;
	OutPutHandler(UByteArrayAdapter & data) : data(data), outPutWord(0), idCount(0) {}
	void push() {
		switch (outPutWord & 0x3) {
		case 0x1: //zero-rle
			break;
		case 0x2: //no-rle
		case 0x0: 
			idCount += popCount(outPutWord >> 1);
			break;
		case 0x3: //one-rle
			idCount += 31*(outPutWord >> 2);
			break;
		}
		data.putUint32(outPutWord);
	}
	
	void append(uint32_t nextWord) {
		if ((nextWord & 0x1) && (outPutWord & 0x1)) { //both are run length encodings, check if we can merge them
			if ((nextWord & 0x2) == (outPutWord & 0x2)) { //equal rle type
				//add the lengths
				outPutWord = (outPutWord + (nextWord & (~static_cast<uint32_t>(0x3)))); //BUG: check overflow
			}
			else {
				push();
				outPutWord = nextWord;
			}
		}
		else {
			if (outPutWord) {
				push();
			}
			outPutWord = nextWord;
		}
	}
	
	///@return idCount
	uint32_t flush() {
		if (outPutWord)
			push();
		return idCount;
	}
};

} //end protection namespace

ItemIndexPrivate * ItemIndexPrivateWAH::intersect(const sserialize::ItemIndexPrivate * other) const {
	if (other->type() != ItemIndex::T_WAH)
		return ItemIndexPrivate::doIntersect(other);
	const ItemIndexPrivateWAH * cother = static_cast<const ItemIndexPrivateWAH*>(other);

	UDWConstrainedIterator myIt(dataIterator());
	UDWConstrainedIterator oIt(cother->dataIterator());
	//DO NOT CALL reset() on the iterators as that will rewind them to the beginning of the index, not the data
	
	UByteArrayAdapter oData = UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY);
	oData.putUint32(0); //dummy size
	oData.putUint32(0); //dummy count
	OutPutHandler oHandler(oData);
	
	uint32_t myVal = 0;
	uint32_t oVal = 0;
	while (true) {
		if ( ! (myVal >> 2) ) {
			if (!myIt.hasNext()) {
				break;
			}
			myVal = myIt.next();
		}
		if ( !(oVal >> 2) ) {
			if (!oIt.hasNext()) {
				break;
			}
			oVal = oIt.next();
		}
		
		uint32_t pushType = 0x1; //default to 1 zero-rle
		uint8_t types = ((myVal & 0x3) << 2) | (oVal & 0x3);
		switch (types) {
		//myval with zero-rle
		case ((0x4 | 0x8) | 0x3): //myval is 1-rle and oVal is one-rle => pushType = one-rle
			//only set pushType here and fall through to handling of the rest
			pushType = 0x3;
			[[fallthrough]];
		case (0x4 | 0x1) ://myVal is zero-rle and oVal is zero-rle => pushType = 0-rle
		case (0x4 | 0x3) ://myVal is zero-rle and oVal is one-rle => pushType = 0-rle
		case ((0x4 | 0x8) | 0x1): //myval is 1-rle and oVal is zero-rle => pushType = 0-rle
		{
			if ((oVal & (~static_cast<uint32_t>(0x3))) <= (myVal & (~static_cast<uint32_t>(0x3)))) {
				oHandler.append((oVal & ~static_cast<uint32_t>(0x3)) | pushType);
				myVal -= (oVal & ~static_cast<uint32_t>(0x3));
				oVal = 0;
			}
			else {
				oHandler.append((myVal & ~static_cast<uint32_t>(0x3)) | pushType);
				oVal -= (myVal & ~static_cast<uint32_t>(0x3));
				myVal = 0;
			}
			break;
		}
		case (0x4 | 0x0): //myval is zero-rle and oval is no rle
		case (0x4 | 0x2):
		{
			oVal = 0;
			myVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append((0x4 | 0x1)); //1 zero-rle
			break;
		}
		case ((0x4 | 0x8) | 0x0): //myval is 1-rle and oVal is no rle
		case ((0x4 | 0x8) | 0x2):
		{
			oHandler.append(oVal);
			myVal -= static_cast<uint32_t>(0x1) << 2;
			oVal = 0;
			break;
		}
		//myval no rle
		case (0x8 | 0x1): //myval no rle, oVal zero-rle
		case (0x0 | 0x1):
		{
			oVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append((0x4 | 0x1));
			myVal = 0;
			break;
		}
		case(0x0 | 0x3): //myval no rle, oVal one-rle
		case(0x8 | 0x3):
		{
			oHandler.append(myVal);
			myVal = 0;
			oVal -= static_cast<uint32_t>(0x1) << 2;
			break;
		}
		case(0x0 | 0x0): //myval no-rle oval no-rle
		case(0x0 | 0x2):
		case(0x8 | 0x0):
		case(0x8 | 0x2):
		{
			uint32_t pushVal = myVal & oVal;
			pushVal = (pushVal ? pushVal : (0x4 | 0x1));
			oHandler.append(pushVal);
			myVal = 0;
			oVal = 0;
			break;
		}
		}; //end switch
	}
	
	uint32_t idCount = oHandler.flush();
	oData.putUint32(0, (uint32_t) (oData.tellPutPtr()-8));
	oData.putUint32(4, idCount);
	oData.resetPtrs();
	return new ItemIndexPrivateWAH(oData);
}

ItemIndexPrivate * ItemIndexPrivateWAH::unite(const sserialize::ItemIndexPrivate * other) const {
	if (other->type() != ItemIndex::T_WAH)
		return ItemIndexPrivate::doUnite(other);
	const ItemIndexPrivateWAH * cother = static_cast<const ItemIndexPrivateWAH*>(other);

	UDWConstrainedIterator myIt(dataIterator());
	UDWConstrainedIterator oIt(cother->dataIterator());
	//DO NOT CALL reset() on the iterators as that will rewind them to the beginning of the index, not the data
	
	UByteArrayAdapter oData = UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY);
	oData.putUint32(0); //dummy size
	oData.putUint32(0); //dummy count
	OutPutHandler oHandler(oData);
	
	uint32_t myVal = 0;
	uint32_t oVal = 0;
	while(true) {
		if ( ! (myVal >> 2) ) {
			if (!myIt.hasNext()) {
				break;
			}
			myVal = myIt.next();
		}
		if ( !(oVal >> 2) ) {
			if (!oIt.hasNext()) {
				break;
			}
			oVal = oIt.next();
		}

		uint8_t types = ((myVal & 0x3) << 2) | (oVal & 0x3);
		switch (types) {
		//myval with zero-rle
		case ((0x4 | 0x8) | 0x3): //myval is 1-rle and oVal is one-rle => pushType = myVal | oval
		case (0x4 | 0x1) ://myVal is zero-rle and oVal is zero-rle => pushType = myVal | oval
		case (0x4 | 0x3) ://myVal is zero-rle and oVal is one-rle => pushType = myVal | oval
		case ((0x4 | 0x8) | 0x1): //myval is 1-rle and oVal is zero-rle => pushType = myVal | oval
		{
			uint32_t pushType = (myVal & 0x3) | (oVal & 0x3);
			if ((oVal & (~static_cast<uint32_t>(0x3))) <= (myVal & (~static_cast<uint32_t>(0x3)))) {
				oHandler.append((oVal & ~static_cast<uint32_t>(0x3)) | pushType);
				myVal -= (oVal & ~static_cast<uint32_t>(0x3));
				oVal = 0;
			}
			else {
				oHandler.append((myVal & ~static_cast<uint32_t>(0x3)) | pushType);
				oVal -= (myVal & ~static_cast<uint32_t>(0x3));
				myVal = 0;
			}
			break;
		}
		case (0x4 | 0x0): //myval is zero-rle and oval is no rle
		case (0x4 | 0x2):
		{
			myVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append(oVal); //1-rle zero
			oVal = 0;
			break;
		}
		case ((0x4 | 0x8) | 0x0): //myval is 1-rle and oVal is no rle
		case ((0x4 | 0x8) | 0x2):
		{
			oHandler.append((0x4 | 0x3));
			myVal -= static_cast<uint32_t>(0x1) << 2;
			oVal = 0;
			break;
		}
		//myval no rle
		case (0x8 | 0x1): //myval no rle, oVal zero-rle
		case (0x0 | 0x1):
		{
			oVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append(myVal);
			myVal = 0;
			break;
		}
		case(0x0 | 0x3): //myval no rle, oVal one-rle
		case(0x8 | 0x3):
		{
			oHandler.append((0x4 | 0x3)); //one one-rle
			myVal = 0;
			oVal -= static_cast<uint32_t>(0x1) << 2;
			break;
		}
		case(0x0 | 0x0): //myval no-rle oval no-rle
		case(0x0 | 0x2):
		case(0x8 | 0x0):
		case(0x8 | 0x2):
		{
			uint32_t pushVal = myVal | oVal;
			pushVal = (~pushVal == 0x1 ? (0x4 | 0x3) : pushVal );
			oHandler.append(pushVal);
			myVal = 0;
			oVal = 0;
			break;
		}
		}; //end switch
	}
	
	if (myVal >> 2 || ((myVal & 0x1) == 0 && myVal >> 1)) { //myVal can be an rle or a non-rle
		oHandler.append(myVal);
	}
	while(myIt.hasNext()) {
		myVal = myIt.next();
		oHandler.append(myVal);
	}
	
	if (oVal >> 2 || ((oVal & 0x1) == 0 && oVal >> 1)) {
		oHandler.append(oVal);
	}
	while(oIt.hasNext()) {
		oVal = oIt.next();
		oHandler.append(oVal);
	}
	
	uint32_t idCount = oHandler.flush();
	oData.putUint32(0, (uint32_t) (oData.tellPutPtr()-8));
	oData.putUint32(4, idCount);
	oData.resetPtrs();
	return new ItemIndexPrivateWAH(oData);
}

ItemIndexPrivate * ItemIndexPrivateWAH::difference(const sserialize::ItemIndexPrivate * other) const {
	if (other->type() != ItemIndex::T_WAH)
		return ItemIndexPrivate::doDifference(other);
	const ItemIndexPrivateWAH * cother = static_cast<const ItemIndexPrivateWAH*>(other);


	UDWConstrainedIterator myIt(dataIterator());
	UDWConstrainedIterator oIt(cother->dataIterator());
	//DO NOT CALL reset() on the iterators as that will rewind them to the beginning of the index, not the data
	
	UByteArrayAdapter oData = UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY);
	oData.putUint32(0); //dummy size
	oData.putUint32(0); //dummy count
	OutPutHandler oHandler(oData);
	
	uint32_t myVal = 0;
	uint32_t oVal = 0;
	while (true) {
		if ( ! (myVal >> 2) ) {
			if (!myIt.hasNext()) {
				break;
			}
			myVal = myIt.next();
		}
		if ( !(oVal >> 2) ) {
			if (!oIt.hasNext()) {
				break;
			}
			oVal = oIt.next();
		}
		
		uint32_t pushType = myVal & 0x3; //default to myVal
		uint8_t types = ((myVal & 0x3) << 2) | (oVal & 0x3);
		switch (types) {
		//myval with zero-rle
		case ((0x4 | 0x8) | 0x3): //myval is 1-rle and oVal is one-rle => pushType = zero-rle
			//only set pushType here and fall through to handling of the rest
			pushType = 0x1;
			[[fallthrough]];
		case (0x4 | 0x1) ://myVal is zero-rle and oVal is zero-rle => pushType = myVal
		case (0x4 | 0x3) ://myVal is zero-rle and oVal is one-rle => pushType = myVal
		case ((0x4 | 0x8) | 0x1): //myval is 1-rle and oVal is zero-rle => pushType = myVal
		{
			if ((oVal & (~static_cast<uint32_t>(0x3))) <= (myVal & (~static_cast<uint32_t>(0x3)))) {
				oHandler.append((oVal & ~static_cast<uint32_t>(0x3)) | pushType);
				myVal -= (oVal & ~static_cast<uint32_t>(0x3));
				oVal = 0;
			}
			else {
				oHandler.append((myVal & ~static_cast<uint32_t>(0x3)) | pushType);
				oVal -= (myVal & ~static_cast<uint32_t>(0x3));
				myVal = 0;
			}
			break;
		}
		case (0x4 | 0x0): //myval is zero-rle and oval is no rle
		case (0x4 | 0x2):
		{
			oVal = 0;
			myVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append((0x4 | 0x1)); //1-rle zero
			break;
		}
		case ((0x4 | 0x8) | 0x0): //myval is 1-rle and oVal is no rle
		case ((0x4 | 0x8) | 0x2):
		{
			oHandler.append(~oVal & (~static_cast<uint32_t>(0x1)));
			myVal -= static_cast<uint32_t>(0x1) << 2;
			oVal = 0;
			break;
		}
		//myval no rle
		case (0x8 | 0x1): //myval no rle, oVal zero-rle
		case (0x0 | 0x1):
		{
			oVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append(myVal);
			myVal = 0;
			break;
		}
		case(0x0 | 0x3): //myval no rle, oVal one-rle
		case(0x8 | 0x3):
		{
			oHandler.append((0x4 | 0x1)); //one zero-rle
			myVal = 0;
			oVal -= static_cast<uint32_t>(0x1) << 2;
			break;
		}
		case(0x0 | 0x0): //myval no-rle oval no-rle
		case(0x0 | 0x2):
		case(0x8 | 0x0):
		case(0x8 | 0x2):
		{
			uint32_t pushVal = myVal & (~oVal);
			pushVal = (pushVal ? pushVal : (0x4 | 0x1));
			oHandler.append(pushVal); //push either a 1-rle-zero or a non-rle
			myVal = 0;
			oVal = 0;
			break;
		}
		};
	}
	if (myVal >> 2 || ((myVal & 0x1) == 0 && myVal >> 1)) { //myVal can be an rle or a non-rle
		oHandler.append(myVal);
	}
	while(myIt.hasNext()) {
		myVal = myIt.next();
		oHandler.append(myVal);
	}
	
	uint32_t idCount = oHandler.flush();
	oData.putUint32(0, (uint32_t) (oData.tellPutPtr()-8));
	oData.putUint32(4, idCount);
	oData.resetPtrs();
	return new ItemIndexPrivateWAH(oData);
}

ItemIndexPrivate * ItemIndexPrivateWAH::symmetricDifference(const sserialize::ItemIndexPrivate * other) const {
	if (other->type() != ItemIndex::T_WAH)
		return ItemIndexPrivate::doSymmetricDifference(other);
	const ItemIndexPrivateWAH * cother = static_cast<const ItemIndexPrivateWAH*>(other);


	UDWConstrainedIterator myIt(dataIterator());
	UDWConstrainedIterator oIt(cother->dataIterator());
	//DO NOT CALL reset() on the iterators as that will rewind them to the beginning of the index, not the data
	
	UByteArrayAdapter oData = UByteArrayAdapter::createCache(8, sserialize::MM_PROGRAM_MEMORY);
	oData.putUint32(0); //dummy size
	oData.putUint32(0); //dummy count
	OutPutHandler oHandler(oData);
	
	uint32_t myVal = 0;
	uint32_t oVal = 0;
	while (true) {
		if ( ! (myVal >> 2) ) {
			if (!myIt.hasNext()) {
				break;
			}
			myVal = myIt.next();
		}
		if ( !(oVal >> 2) ) {
			if (!oIt.hasNext()) {
				break;
			}
			oVal = oIt.next();
		}
		
		uint8_t types = ((myVal & 0x3) << 2) | (oVal & 0x3);
		switch (types) {
		case ((0x4 | 0x8) | 0x3): //myval is 1-rle and oVal is one-rle => pushType = zero-rle
		case (0x4 | 0x1) ://myVal is zero-rle and oVal is zero-rle => pushType = zero-rle
		case (0x4 | 0x3) ://myVal is zero-rle and oVal is one-rle => pushType = one-rle
		case ((0x4 | 0x8) | 0x1): //myval is 1-rle and oVal is zero-rle => pushType = one-rle
		{
			uint32_t pushType = ((myVal xor oVal) & 0x2) | 0x1;
			if ((oVal & (~static_cast<uint32_t>(0x3))) <= (myVal & (~static_cast<uint32_t>(0x3)))) {
				oHandler.append((oVal & ~static_cast<uint32_t>(0x3)) | pushType);
				myVal -= (oVal & ~static_cast<uint32_t>(0x3));
				oVal = 0;
			}
			else {
				oHandler.append((myVal & ~static_cast<uint32_t>(0x3)) | pushType);
				oVal -= (myVal & ~static_cast<uint32_t>(0x3));
				myVal = 0;
			}
			break;
		}
		case (0x4 | 0x0): //myval is zero-rle and oval is no rle
		case (0x4 | 0x2):
		{
			myVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append(oVal);
			oVal = 0;
			break;
		}
		case ((0x4 | 0x8) | 0x0): //myval is 1-rle and oVal is no rle
		case ((0x4 | 0x8) | 0x2):
		{
			oHandler.append(~(oVal | 0x1));
			myVal -= static_cast<uint32_t>(0x1) << 2;
			oVal = 0;
			break;
		}
		//myval no rle
		case (0x8 | 0x1): //myval no rle, oVal zero-rle
		case (0x0 | 0x1):
		{
			oVal -= (static_cast<uint32_t>(1) << 2);
			oHandler.append(myVal);
			myVal = 0;
			break;
		}
		case(0x0 | 0x3): //myval no rle, oVal one-rle
		case(0x8 | 0x3):
		{
			oHandler.append(~(myVal | 0x1)); //one zero-rle
			myVal = 0;
			oVal -= static_cast<uint32_t>(0x1) << 2;
			break;
		}
		case(0x0 | 0x0): //myval no-rle oval no-rle
		case(0x0 | 0x2):
		case(0x8 | 0x0):
		case(0x8 | 0x2):
		{
			uint32_t pushVal = myVal xor oVal;
			if (pushVal == 0xFFFFFFFE)
				pushVal = (0x4 | 0x3);
			else if (!pushVal)
				pushVal = (0x4 | 0x1);
			oHandler.append(pushVal); //push either a 1-rle-zero or a non-rle
			myVal = 0;
			oVal = 0;
			break;
		}
		}; //end switch
	}
	
	if (myVal >> 2 || ((myVal & 0x1) == 0 && myVal >> 1)) { //myVal can be an rle or a non-rle
		oHandler.append(myVal);
	}
	while(myIt.hasNext()) {
		myVal = myIt.next();
		oHandler.append(myVal);
	}
	
	if (oVal >> 2 || ((oVal & 0x1) == 0 && oVal >> 1)) {
		oHandler.append(oVal);
	}
	while(oIt.hasNext()) {
		oVal = oIt.next();
		oHandler.append(oVal);
	}
	
	uint32_t idCount = oHandler.flush();
	oData.putUint32(0, (uint32_t) (oData.tellPutPtr()-8));
	oData.putUint32(4, idCount);
	oData.resetPtrs();
	return new ItemIndexPrivateWAH(oData);
}

//Optimilize:
//if an subtract index is at the end, remove it

ItemIndex ItemIndexPrivateWAH::fusedIntersectDifference(const std::vector< ItemIndexPrivateWAH * >& intersect, const std::vector< ItemIndexPrivateWAH * >& subtract, uint32_t count, ItemIndex::ItemFilter * filter) {
	std::vector<uint32_t> resultIds;
	resultIds.reserve(count);
	
	uint32_t intersectSize = (uint32_t) intersect.size();
	uint32_t subtractSize = (uint32_t) subtract.size();
	
	std::vector<UDWConstrainedIterator> intersectIts;
	intersectIts.reserve(intersectSize);
	std::vector<UDWConstrainedIterator> subtractIts;
	subtractIts.reserve(subtractSize);
	std::vector<uint32_t> intersectEncWords(intersectSize, 0);
	std::vector<uint32_t> subtractEncWords(subtractSize, 0);
	uint64_t bitOffset = 0;
	uint32_t currentWorkingWord = 0;
	
	//initialize variables
	for(uint32_t i = 0; i < intersectSize; ++i) {
		intersectIts.push_back(intersect[i]->dataIterator());
		intersectEncWords[i] = intersectIts[i].next();
	}
	for(uint32_t i = 0; i < subtractSize; ++i) {
		subtractIts.push_back(subtract[i]->dataIterator());
		subtractEncWords[i] = subtractIts[i].next();
	}

	bool allIntersectsHaveNext = true;
	while (resultIds.size() < count && allIntersectsHaveNext) {
		//process 31 Bits in  each round until either one intersect index is at the end or we have count elements in our result set
		currentWorkingWord = 0x7FFFFFFF; //31 bits set
		//do the intersects
		for(uint32_t i = 0; i < intersectSize; ++i) {
			bool loadNext = false;
			uint32_t rleFlag = intersectEncWords[i] & 0x3;
			switch (rleFlag) {
			case 0x1: //0s rle encoded
				currentWorkingWord = 0;
				//fall through to check for word
				[[fallthrough]];
			case 0x3: //1s rle encoded, currentWorkingWord stays the smae
				intersectEncWords[i] = ((intersectEncWords[i] >> 2) - 1); //move out flags 
				loadNext = !intersectEncWords[i]; //check if no rle anymore
				intersectEncWords[i] = (intersectEncWords[i] << 2) | rleFlag; //restore flags
				break;
			default:
				currentWorkingWord &= (intersectEncWords[i] >> 1);
				loadNext = true;
				break;
			}
			if (loadNext) {
				if (intersectIts[i].hasNext())
					intersectEncWords[i] = intersectIts[i].next();
				else
					allIntersectsHaveNext = false;
			}
		}
		
		//do the substracts
		for(uint32_t i = 0; i < subtractSize; ++i) {
			bool loadNext = false;
			uint32_t rleFlag = subtractEncWords [i] & 0x3;
			switch (rleFlag) {
			case 0x3: //1s rle encoded, zero word
				currentWorkingWord = 0;
				//fall through to check for word
				[[fallthrough]];
			case 0x1: //0s rle encoded, currentWorkingWord stays the same
				subtractEncWords[i] = ((subtractEncWords[i] >> 2) - 1); //move out flags 
				loadNext = !intersectEncWords[i]; //check if no rle anymore
				subtractEncWords[i] = (subtractEncWords[i] << 2) | rleFlag; //restore flags
				break;
			default:
				currentWorkingWord &= ~ (subtractEncWords[i] >> 1);
				loadNext = true;
				break;
			}
			if (loadNext) {
				if (subtractIts[i].hasNext())
					subtractEncWords[i] = subtractIts[i].next();
				else
					subtractEncWords[i] = 0;
			}
		}
		
		for(uint32_t i = (uint32_t) bitOffset; currentWorkingWord && i < bitOffset+31; ++i) {
			if ((currentWorkingWord & 0x1) && (!filter || filter->operator()(i))) {
				resultIds.push_back(i);
			}
			currentWorkingWord >>= 1;
		}
		bitOffset += 31;
	}
	
	return ItemIndex(std::move(resultIds));
}

ItemIndex ItemIndexPrivateWAH::constrainedIntersect(const std::vector< sserialize::ItemIndexPrivateWAH * >& intersect, uint32_t count, sserialize::ItemIndex::ItemFilter * filter) {
	std::vector<uint32_t> resultIds;
	resultIds.reserve(count);
	
	const uint32_t intersectSize = (uint32_t) intersect.size();
	
	std::vector<uint32_t> intersectEncWords(intersectSize, 0);
	std::vector<UDWConstrainedIterator> intersectIts;
	intersectIts.reserve(intersectSize);
	uint64_t bitOffset = 0;
	uint32_t currentWorkingWord = 0;
	
	//initialize variables
	for(uint32_t i = 0; i < intersectSize; ++i) {
		intersectIts.push_back(intersect[i]->dataIterator());
		intersectEncWords[i] = intersectIts[i].next();
	}

	bool allIntersectsHaveNext = true;
	while (resultIds.size() < count && allIntersectsHaveNext) {
		//process 31 Bits in  each round until either one intersect index is at the end or we have count elements in our result set
		currentWorkingWord = std::numeric_limits<uint32_t>::max();

		//fast-forward support if all indices have rle encoding
		//TODO:reimplement
		
		//do the intersects
		for(uint32_t i = 0; i < intersectSize; ++i) {
			bool loadNext = false;
			uint32_t rleFlag = intersectEncWords[i] & 0x3;
			switch (rleFlag) {
			case 0x1: //0s rle encoded
				currentWorkingWord = 0;
				//fall through to check for word
				[[fallthrough]];
			case 0x3: //1s rle encoded, currentWorkingWord stays the smae
				intersectEncWords[i] = ((intersectEncWords[i] >> 2) - 1); //move out flags 
				loadNext = !(intersectEncWords[i]); //check if no rle anymore
				intersectEncWords[i] = (intersectEncWords[i] << 2) | rleFlag; //restore flags
				break;
			default:
				currentWorkingWord &= (intersectEncWords[i] >> 1);
				loadNext = true;
				break;
			};
			if (loadNext) {
				if (intersectIts[i].hasNext())
					intersectEncWords[i] = intersectIts[i].next();
				else
					allIntersectsHaveNext = false;
			}
		}
		
		for(uint32_t i = (uint32_t) bitOffset; currentWorkingWord && i < bitOffset+31; ++i) {
			if ((currentWorkingWord & 0x1) && (!filter || filter->operator()(i))) {
				resultIds.push_back(i);
			}
			currentWorkingWord >>= 1;
		}
		bitOffset += 31;
	}
	
	return ItemIndex(std::move(resultIds));
}



}//end namespace
