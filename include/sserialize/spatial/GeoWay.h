#ifndef SSERIALIZE_SPATIAL_GEO_WAY_H
#define SSERIALIZE_SPATIAL_GEO_WAY_H
#include <sserialize/spatial/GeoRegion.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/utility/exceptions.h>

namespace sserialize {
namespace spatial {

class GeoWay: public GeoRegion {
public:
	typedef sserialize::spatial::GeoPoint Point;
	typedef std::vector<Point> PointsContainer;
	typedef PointsContainer::iterator PointsIterator;
	typedef PointsContainer::const_iterator ConstPointsIterator;	
	typedef GeoRegion MyBaseClass;
private:
	PointsContainer m_points;
	GeoRect m_boundary;
protected:
	inline const GeoRect & myBoundary() const { return m_boundary; }
public:
	GeoWay();
	GeoWay(const PointsContainer & points);
	virtual ~GeoWay();
	void updateBoundaryRect();
	/** you need to update the boundary rect if you changed anything here! */
	inline PointsContainer & points() { return m_points; }
	inline const PointsContainer & points() const { return m_points; }
	inline ConstPointsIterator cbegin() const { return points().cbegin(); }
	inline ConstPointsIterator cend() const { return points().cend(); }
	

	
	virtual GeoShapeType type() const;
	virtual uint32_t size() const;
	virtual GeoRect boundary() const;
	
	virtual bool contains(const GeoPoint & p) const;
	virtual bool intersects(const sserialize::spatial::GeoRect & rect) const;
	///@return true if the line p1->p2 intersects this region
	virtual bool intersects(const GeoPoint & p1, const GeoPoint & p2) const;
	virtual bool intersects(const GeoRegion & other) const;
	
	virtual UByteArrayAdapter & serializeWithTypeInfo(UByteArrayAdapter & destination) const;
	UByteArrayAdapter & serialize(UByteArrayAdapter & destination) const;
};

}}//end namespace

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoWay & p) ;


#endif