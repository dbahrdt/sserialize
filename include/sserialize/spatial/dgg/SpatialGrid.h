#pragma once
#include <sserialize/utility/refcounting.h>
#include <sserialize/containers/AbstractArray.h>
#include <sserialize/spatial/GeoRect.h>

namespace sserialize::spatial::dgg::exceptions {

class InvalidPixelId: public sserialize::Exception {
public:
	InvalidPixelId(std::string const & what) : sserialize::Exception(what) {}
	virtual ~InvalidPixelId() {}
};

}//end namespace sserialize::spatial::dgg::exceptions

namespace sserialize::spatial::dgg::interface {

///A hierarchical spatial grid with the following properties:
///Level 0 contains a single root node
///Each node is further divisible though the number of splits may differ with each node
class SpatialGrid: public sserialize::RefCountObject {
public:
	class CompressedPixelId {
	public:
		CompressedPixelId(CompressedPixelId const &) = default;
		explicit CompressedPixelId(uint32_t value) : m_v(value) {}
		CompressedPixelId & operator=(CompressedPixelId const &) = default;
	public:
		inline uint32_t value() const { return m_v; }
		inline void value(uint32_t v) { m_v = v; }
	private:
		uint32_t m_v;
	};
	using PixelId = uint64_t;
	using Level = int32_t;
	using Size = uint32_t;
	class TreeNode {
		PixelId cellId;
		std::vector< std::unique_ptr<TreeNode> > children; 
	};
	using CellIterator = sserialize::AbstractArrayIterator<PixelId>;
	static constexpr PixelId NoPixelId = std::numeric_limits<PixelId>::max();
public: //global stuff
	virtual std::string name() const = 0;
	//by default typeId() == name()
	virtual std::string typeId() const;
public:
	virtual Level maxLevel() const = 0;
	virtual Level defaultLevel() const = 0;
	virtual PixelId rootPixelId() const = 0;
public:
	virtual Level level(PixelId pixelId) const = 0;
	virtual bool isAncestor(PixelId ancestor, PixelId decendant) const = 0;
public: //look-up
	virtual PixelId index(double lat, double lon, Level level) const = 0;
	virtual PixelId index(double lat, double lon) const = 0;
public: //tree navigation
	virtual PixelId index(PixelId parent, uint32_t childNumber) const = 0;
	virtual PixelId parent(PixelId child) const = 0;
	virtual Size childPosition(PixelId parent, PixelId child) const;
	virtual Size childrenCount(PixelId pixel) const = 0;
public:
	virtual std::unique_ptr<TreeNode> tree(CellIterator begin, CellIterator end) const = 0;
public: //info
	///in square kilometers
	virtual double area(PixelId pixel) const = 0;
	virtual sserialize::spatial::GeoRect bbox(PixelId pixel) const = 0;
	virtual std::string to_string(PixelId pixel) const;
protected:
	SpatialGrid();
	virtual ~SpatialGrid();
};
	
}//end namespace sserialize::spatial::dgg::interface
