#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/algorithm/utilmath.h>
#include <cmath>
#include <vendor/libs/geographiclib/legacy/C/geodesic.h>

namespace sserialize {
namespace spatial {

GeoPoint::GeoPoint() :
m_lat(1337.0),
m_lon(-1337.0)
{}

GeoPoint::GeoPoint(double lat, double lon) :
m_lat(lat),
m_lon(lon)
{}

GeoPoint::GeoPoint(std::pair<double, double> p) :
m_lat(p.first),
m_lon(p.second)
{}

GeoPoint::GeoPoint(std::pair<float, float> p) :
m_lat(p.first),
m_lon(p.second)
{}


GeoPoint::GeoPoint(const UByteArrayAdapter & data) :
m_lat(toDoubleLat(data.getUint32(0))),
m_lon(toDoubleLon(data.getUint32(4)))
{}

GeoPoint::GeoPoint(uint32_t lat, uint32_t lon) :
m_lat(toDoubleLat(lat)),
m_lon(toDoubleLon(lon))
{}

GeoPoint::GeoPoint(const GeoPoint & other) :
m_lat(other.m_lat),
m_lon(other.m_lon)
{}


GeoPoint& GeoPoint::operator=(const GeoPoint & other) {
	m_lat = other.m_lat;
	m_lon = other.m_lon;
	return *this;
}

GeoPoint::~GeoPoint() {}

GeoShapeType GeoPoint::type() const {
	return GS_POINT;
}

bool GeoPoint::valid() const {
	return -90.0 <= lat() && lat() <= 90.0 && -180.0 <= lon() && lon() <= 180.0;
}

void GeoPoint::normalize() {
	if (lat() < -90.0 || lat() > 90.0)
		lat() = fmod(lat()+90.0, 180.0)-90.0;
	if (lon() < -180.0 || lon() > 180.0) {
		lon() = fmod(lon()+180.0, 360.0)-180.0;
	}
}

GeoRect GeoPoint::boundary() const {
	return GeoRect(lat(), lat(), lon(), lon());
}
uint32_t GeoPoint::size() const {
	return 1;
}

bool GeoPoint::intersects(const GeoRect & boundary) const {
	return boundary.contains(lat(), lon());
}

void GeoPoint::recalculateBoundary() {}

UByteArrayAdapter & GeoPoint::append(UByteArrayAdapter & destination) const {
	destination.putUint32(sserialize::spatial::GeoPoint::toIntLat(lat()));
	destination.putUint32(sserialize::spatial::GeoPoint::toIntLon( lon()));
	return destination;
}

bool GeoPoint::intersect(const GeoPoint & p , const GeoPoint & q, const GeoPoint & r, const GeoPoint & s) {
	double tl1, tl2;
	double t1_denom = (q.lon()-p.lon())*(s.lat()-r.lat())+(q.lat()-p.lat())*(r.lon()-s.lon());
	if (std::abs(t1_denom) <= EPSILON)
		return false;
	double t1_nom = (r.lon()-p.lon())*(s.lat()-r.lat())+(r.lat()-p.lat())*(r.lon()-s.lon());
	
	if (sserialize::sgn(t1_nom)*sserialize::sgn(t1_denom) < 0)
		return false;
	tl1 = t1_nom/t1_denom;
	if (tl1 > 1)
		return false;
	tl2 = (tl1*(q.lat()-p.lat())-r.lat()+p.lat())/(s.lat()-r.lat());
	return (0.0 <= tl2 && 1.0 >= tl2);
}

double GeoPoint::distance(const sserialize::spatial::GeoShape& other, const sserialize::spatial::DistanceCalculator& distanceCalculator) const {
	if (other.type() == sserialize::spatial::GS_POINT) {
		const sserialize::spatial::GeoPoint op = *static_cast<const sserialize::spatial::GeoPoint*>(&other);
		return distanceCalculator.calc(m_lat, m_lon, op.m_lat, op.m_lon);
	}
	else if (other.type() > GS_POINT) {
		return other.distance(*this, distanceCalculator);
	}
	return std::numeric_limits<double>::quiet_NaN();
}

sserialize::spatial::GeoShape * GeoPoint::copy() const {
	return new sserialize::spatial::GeoPoint(*this);
}

std::ostream & GeoPoint::asString(std::ostream & out) const {
	return out << "GeoPoint(" << lat() << ", " << lon() << ")";
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoPoint & point) {
	return point.append(destination);
}

sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & destination, sserialize::spatial::GeoPoint & p) {
	p.lat() = sserialize::spatial::GeoPoint::toDoubleLat(destination.getUint32());
	p.lon() = sserialize::spatial::GeoPoint::toDoubleLon(destination.getUint32());
	return destination;
}

}}//end namespace