#ifndef SSERIALIZE_UBYTE_ARRAY_ADAPTER_PRIVATE_H
#define SSERIALIZE_UBYTE_ARRAY_ADAPTER_PRIVATE_H

#include <stdint.h>
#include <sserialize/storage/UByteArrayAdapter.h>

#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
	#define SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL
	#define SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE
#else
	#define SSERIALIZE_UBA_CONTIG_CFG_VIRTUAL virtual
	#define SSERIALIZE_UBA_CONTIG_CFG_OVERRIDE override
#endif

#ifdef SSERIALIZE_UBA_OPTIONAL_REFCOUNTING
	#define SSERIALIZE_UBA_PRIVATE_REFCOUNT_CLASS RefCountObjectWithDisable
#else
	#define SSERIALIZE_UBA_PRIVATE_REFCOUNT_CLASS RefCountObject
#endif

namespace sserialize {
namespace UByteArrayAdapterOnlyContiguous {

class UByteArrayAdapterPrivate: public SSERIALIZE_UBA_PRIVATE_REFCOUNT_CLASS {
protected:
	bool m_deleteOnClose;
public:
	UByteArrayAdapterPrivate() : m_deleteOnClose(false) {}
	virtual ~UByteArrayAdapterPrivate() {}
	virtual UByteArrayAdapter::OffsetType size() const = 0;

//support operations
	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) = 0;
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType size) = 0;

//advise api
	virtual void advice(UByteArrayAdapter::AdviseType /*at*/, UByteArrayAdapter::SizeType /*begin*/, UByteArrayAdapter::SizeType /*end*/) {}

//manipulators
	virtual void setDeleteOnClose(bool del) { m_deleteOnClose = del;}
};

} //end namespace UByteArrayAdapterOnlyContiguous

namespace UByteArrayAdapterNonContiguous {

class UByteArrayAdapterPrivate: public SSERIALIZE_UBA_PRIVATE_REFCOUNT_CLASS {
protected:
	bool m_deleteOnClose;
public:
	UByteArrayAdapterPrivate() : m_deleteOnClose(false) {}
	virtual ~UByteArrayAdapterPrivate() {}
	virtual UByteArrayAdapter::OffsetType size() const = 0;
	virtual bool isContiguous() const { return false; }

//support opertions

	/** Shrink data to size bytes */
	virtual bool shrinkStorage(UByteArrayAdapter::OffsetType size) = 0;
	/** grow data to at least! size bytes */
	virtual bool growStorage(UByteArrayAdapter::OffsetType size) = 0;

//advise api
	virtual void advice(UByteArrayAdapter::AdviseType /*at*/, UByteArrayAdapter::SizeType /*begin*/, UByteArrayAdapter::SizeType /*end*/) {}

//manipulators
	virtual void setDeleteOnClose(bool del) { m_deleteOnClose = del;}

//Access functions
	virtual uint8_t & operator[](UByteArrayAdapter::OffsetType pos) = 0;
	virtual const uint8_t & operator[](UByteArrayAdapter::OffsetType pos) const = 0;

	virtual int64_t getInt64(UByteArrayAdapter::OffsetType pos) const = 0;
	virtual uint64_t getUint64(UByteArrayAdapter::OffsetType pos) const = 0;

	virtual int32_t getInt32(UByteArrayAdapter::OffsetType pos) const = 0;
	virtual uint32_t getUint32(UByteArrayAdapter::OffsetType pos) const = 0;
	virtual uint32_t getUint24(UByteArrayAdapter::OffsetType pos) const = 0;
	virtual uint16_t getUint16(UByteArrayAdapter::OffsetType pos) const = 0;
	virtual uint8_t getUint8(UByteArrayAdapter::OffsetType pos) const = 0;
	
	virtual UByteArrayAdapter::NegativeOffsetType getNegativeOffset(UByteArrayAdapter::OffsetType pos) const = 0;
	virtual UByteArrayAdapter::OffsetType getOffset(UByteArrayAdapter::OffsetType pos) const = 0;
	
	virtual uint64_t getVlPackedUint64(UByteArrayAdapter::OffsetType pos, int * length) const = 0;
	virtual int64_t getVlPackedInt64(UByteArrayAdapter::OffsetType pos, int * length) const = 0;
	virtual uint32_t getVlPackedUint32(UByteArrayAdapter::OffsetType pos, int * length) const = 0;
	virtual int32_t getVlPackedInt32(UByteArrayAdapter::OffsetType pos, int * length) const = 0;
	
	virtual void get(UByteArrayAdapter::OffsetType pos, uint8_t * dest, UByteArrayAdapter::OffsetType len) const = 0;
	
	virtual std::string getString(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType len) const;

	/** If the supplied memory is not writable then you're on your own! **/

	virtual void putInt64(UByteArrayAdapter::OffsetType pos, int64_t value) = 0;
	virtual void putUint64(UByteArrayAdapter::OffsetType pos, uint64_t value) = 0;
	virtual void putInt32(UByteArrayAdapter::OffsetType pos, int32_t value) = 0;
	virtual void putUint32(UByteArrayAdapter::OffsetType pos, uint32_t value) = 0;
	virtual void putUint24(UByteArrayAdapter::OffsetType pos, uint32_t value) = 0;
	virtual void putUint16(UByteArrayAdapter::OffsetType pos, uint16_t value) = 0;
	virtual void putUint8(UByteArrayAdapter::OffsetType pos, uint8_t value) = 0;
	
	
	virtual void putOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::OffsetType value) = 0;
	virtual void putNegativeOffset(UByteArrayAdapter::OffsetType pos, UByteArrayAdapter::NegativeOffsetType value) = 0;
	
	/** @return: Length of the number, -1 on failure **/
	virtual int putVlPackedUint64(UByteArrayAdapter::OffsetType pos, uint64_t value, UByteArrayAdapter::OffsetType maxLen) = 0;
	virtual int putVlPackedInt64(UByteArrayAdapter::OffsetType pos, int64_t value, UByteArrayAdapter::OffsetType maxLen) = 0;
	virtual int putVlPackedUint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) = 0;
	virtual int putVlPackedPad4Uint32(UByteArrayAdapter::OffsetType pos, uint32_t value, UByteArrayAdapter::OffsetType maxLen) = 0;
	virtual int putVlPackedInt32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) = 0;
	virtual int putVlPackedPad4Int32(UByteArrayAdapter::OffsetType pos, int32_t value, UByteArrayAdapter::OffsetType maxLen) = 0;
	
	virtual void put(UByteArrayAdapter::OffsetType pos, const uint8_t * src, UByteArrayAdapter::OffsetType len) = 0;
};

} //end namespace UByteArrayAdapterNonContiguous


#ifdef SSERIALIZE_UBA_ONLY_CONTIGUOUS
using UByteArrayAdapterOnlyContiguous::UByteArrayAdapterPrivate;
#else
using UByteArrayAdapterNonContiguous::UByteArrayAdapterPrivate;
#endif

}//end namespace

#undef SSERIALIZE_UBA_PRIVATE_REFCOUNT_CLASS
#endif