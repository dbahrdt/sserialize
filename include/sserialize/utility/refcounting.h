#ifndef SSERIALIZE_REF_COUNTING_H
#define SSERIALIZE_REF_COUNTING_H
#include <sserialize/utility/assert.h>
#include <cstdint>
#include <atomic>
#include <utility>

//enable ref-counting stats
// #define SSERIALIZE_GATHER_STATS_REF_COUNTING

namespace sserialize {
namespace detail {
	template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
	class RCBase;
}

class RefCountObject;

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
class RCPtrWrapper;

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
class RCWrapper;

class RefCountObject {
	template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING> friend class detail::RCBase;
public:
	typedef uint32_t RCBaseType;
#ifdef SSERIALIZE_GATHER_STATS_REF_COUNTING
public:
	static std::atomic<uint64_t> GlobalRc;
	static std::atomic<uint64_t> GlobalRcChanges;
	std::atomic<uint32_t> LocalRcChanges;
#endif
public:
	RefCountObject(const RefCountObject & other) = delete;
	RefCountObject & operator=(const RefCountObject & other) = delete;
	RefCountObject();
	virtual ~RefCountObject();

	RCBaseType rc() const;
public:
	void rcInc();
	void rcDec();
	void rcDecWithoutDelete();
	void disableRC();
	void enableRC();
	///relaxed memory order!
	bool enabledRC() const;
private:
	///the lower bit indicates if ref-counting is enabled or not
	std::atomic<RCBaseType> m_rc;
};
namespace detail {

template<typename RCObj>
class RCBase<RCObj, true > {
public:
	RCBase(const RCBase & other) : RCBase(other.priv()) {}
	RCBase(RCBase && other);
	RCBase(RCObj* p) :
	m_priv(0),
	m_enabled(true)
	{
		reset(p);
	}
	virtual ~RCBase() {
		reset(0);
	}
	bool enabledRC() {
		return m_enabled;
	}
	void reset(RCObj* p) {
		if (p && p->enabledRC()) {
			p->rcInc();
		}
		rcDec();
		m_priv = p;
		m_enabled = !p || p->enabledRC();
	}
	void rcInc() {
		if (priv() && enabledRC()) {
			priv()->rcInc();
		}
	}
	void rcDec() {
		if (priv() && enabledRC()) {
			priv()->rcDec();
		}
	}
	RCObj * priv() const { return m_priv; }
	RCObj * priv() { return m_priv; }
	
	///Enable reference counting, this is NOT thread-safe
	///No other thread is allowed to change either the reference counter or the state of reference counting during this operation
	void enableRC() {
// 		return;
		if (priv() && !enabledRC()) {
			priv()->rcInc();
			priv()->enableRC();
			m_enabled = true;
		}
	}
	///Disable reference counting, this is NOT thread-safe
	///No other thread is allowed to change either the reference counter or the state of reference counting during this operation
	///Warning: this may leave the object without an owner
	void disableRC() {
// 		return;
		if (priv() && enabledRC()) {
			priv()->disableRC();
			priv()->rcDecWithoutDelete();
			m_enabled = false;
		}
	}
private:
	RCObj * m_priv;
	bool m_enabled;
};

template<typename RCObj>
class RCBase<RCObj, false > {
public:
	RCBase(const RCBase & other) : RCBase(other.priv()) {}
	RCBase(RCBase && other);
	RCBase(RCObj* p) : m_priv(0) {
		reset(p);
	}
	virtual ~RCBase() {
		reset(0);
	}
	RCBase & operator=(const RCBase & other) {
		reset(other.priv());
		return *this;
	}
	RCBase & operator=(RCBase && other) {
		if (other.priv() != m_priv) {
			rcDec();
		}
		m_priv = other.priv();
		other.m_priv = 0;
	}
	bool enabledRC() {
		return true;
	}
	void reset(RCObj* p) {
		if (p) {
			p->rcInc();
		}
		rcDec();
		m_priv = p;
	}
	void rcInc() {
		if (priv()) {
			priv()->rcInc();
		}
	}
	void rcDec() {
		if (priv()) {
			priv()->rcDec();
		}
	}
	RCObj * priv() const { return m_priv; }
	RCObj * priv() { return m_priv; }
private:
	RCObj * m_priv;
};

} //end namespace detail

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING = false>
class RCWrapper: public detail::RCBase<RCObj, T_CAN_DISABLE_REFCOUNTING> {
public:
	typedef RCObj element_type;
	friend class RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>;
private:
	typedef detail::RCBase<RCObj, T_CAN_DISABLE_REFCOUNTING> MyBaseClass;
public:
	RCWrapper() : MyBaseClass(0) {};
	RCWrapper(RCObj * data) : MyBaseClass(data) {}
	RCWrapper(const RCWrapper & other) : MyBaseClass(other) {}
	RCWrapper(RCWrapper && other) : MyBaseClass(std::move(other)) {}
	RCWrapper(const RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other);
	RCWrapper(RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> && other);
	virtual ~RCWrapper() {}

	RCWrapper & operator=(const RCWrapper & other) {
		MyBaseClass::operator=(other);
		return *this;
	}
	
	RCWrapper & operator=(RCWrapper && other) {
		MyBaseClass::operator=(std::move(other));
		return *this;
	}
	
	bool operator==(const RCWrapper & other) { return priv() == other.priv(); }
	
	RefCountObject::RCBaseType privRc() const { return (priv() ? priv()->rc() : 0);}
public:
	using MyBaseClass::priv;
	using MyBaseClass::reset;
protected: 
	void setPrivate(RCObj * data) {
		MyBaseClass::reset(data);
	}
};

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING = false>
class RCPtrWrapper final : public detail::RCBase<RCObj, T_CAN_DISABLE_REFCOUNTING> {
public:
	typedef RCObj element_type;
	friend class RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>;
private:
	typedef detail::RCBase<RCObj,T_CAN_DISABLE_REFCOUNTING> MyBaseClass;
public:
	void safe_bool_func() {}
	typedef void (RCPtrWrapper<RCObj>:: * safe_bool_type) ();
public:
	RCPtrWrapper() : MyBaseClass(0) {};
	explicit RCPtrWrapper(RCObj * data) : MyBaseClass(data) {}
	RCPtrWrapper(const RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other) : MyBaseClass(other.priv()) {}
	RCPtrWrapper(RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> && other) : MyBaseClass(std::move(other)) {}
	RCPtrWrapper(const RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other) : MyBaseClass(other.priv()) {}
	RCPtrWrapper(RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> && other) : MyBaseClass(std::move(other)) {}
	virtual ~RCPtrWrapper() {}

	RCPtrWrapper & operator=(const RCPtrWrapper & other) {
		reset(other.priv());
		return *this;
	}

	bool operator==(const RCPtrWrapper & other) { return priv() == other.priv(); }

	RefCountObject::RCBaseType privRc() const { return (priv() ? priv()->rc() : 0);}

	RCObj & operator*() { return *priv();}
	const RCObj & operator*() const { return *priv();}

	RCObj * operator->() { return priv();}
	const RCObj * operator->() const { return priv();}

	RCObj * get() { return priv(); }
	const RCObj * get() const { return priv(); }
	
	operator safe_bool_type() const {
		return priv() ? &RCPtrWrapper<RCObj>::safe_bool_func : 0;
	}
public:
	using MyBaseClass::reset;
	using MyBaseClass::priv;
};

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>::RCWrapper(const RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> & other) :
MyBaseClass(other)
{}

template<typename RCObj, bool T_CAN_DISABLE_REFCOUNTING>
RCWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING>::RCWrapper(RCPtrWrapper<RCObj, T_CAN_DISABLE_REFCOUNTING> && other) :
MyBaseClass(std::move(other))
{}

}//end namespace 

#endif
