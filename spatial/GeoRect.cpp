#include <sserialize/spatial/GeoRect.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sstream>
#include <algorithm>

namespace sserialize {
namespace spatial {


GeoRect::GeoRect() {
	m_lat[0] = 0;
	m_lat[1] = 0;
	m_lon[0] = 0;
	m_lon[1] = 0;
}

GeoRect::GeoRect(const GeoRect & other) :
m_lat{other.m_lat[0], other.m_lat[1]},
m_lon{other.m_lon[0], other.m_lon[1]}
{}

GeoRect::GeoRect(double lat1, double lat2, double lon1, double lon2) :
m_lat{lat1, lat2},
m_lon{lon1, lon2}
{
	if (m_lat[0] > m_lat[1])
		std::swap(m_lat[0], m_lat[1]);
	if (m_lon[0] > m_lon[1])
		std::swap(m_lon[0], m_lon[1]);
}

GeoRect::GeoRect(const std::string & str, bool fromLeafletBBox) {
	if (fromLeafletBBox) {
		std::string tempStr = str;
		std::replace(tempStr.begin(), tempStr.end(), ',', ' ');
		std::stringstream ss(tempStr);
		ss >> m_lon[0] >> m_lat[0] >> m_lon[1] >> m_lat[1];
	}
	else {
		std::stringstream ss(str);
		ss >> m_lat[0] >> m_lat[1] >> m_lon[0] >> m_lon[1];
	}
}

GeoRect::GeoRect(const UByteArrayAdapter & data) {
	sserialize::Static::spatial::GeoPoint bL(data);
	sserialize::Static::spatial::GeoPoint tR(data+sserialize::SerializationInfo<sserialize::Static::spatial::GeoPoint>::length);
	m_lat[0] = bL.lat();
	m_lat[1] = tR.lat();
	m_lon[0] = bL.lon();
	m_lon[1] = tR.lon();
}

GeoRect::~GeoRect() {}
double* GeoRect::lat() { return m_lat; }
const double* GeoRect::lat() const { return m_lat; }
double* GeoRect::lon() { return m_lon; }
const double* GeoRect::lon() const { return m_lon; }

double GeoRect::minLat() const { return m_lat[0]; }
double GeoRect::maxLat() const { return m_lat[1]; }

double GeoRect::minLon() const { return m_lon[0]; }
double GeoRect::maxLon() const { return m_lon[1]; }

double & GeoRect::minLat() { return m_lat[0]; }
double & GeoRect::maxLat() { return m_lat[1]; }

double & GeoRect::minLon() { return m_lon[0]; }
double & GeoRect::maxLon() { return m_lon[1]; }

///TODO:check how fast it would be to calculate the real length
double GeoRect::length() const {
	return 2*(m_lat[0]-m_lat[1])+2*(m_lon[0]-m_lon[1]);
}

bool GeoRect::overlap(const GeoRect & other) const {
	if ((m_lat[0] > other.m_lat[1]) || // this is left of other
		(m_lat[1] < other.m_lat[0]) || // this is right of other
		(m_lon[0] > other.m_lon[1]) ||
		(m_lon[1] < other.m_lon[0])) {
			return false;
	}
	return true;
}

bool GeoRect::contains(double lat, double lon) const {
	return (m_lat[0] <= lat && lat <= m_lat[1] && m_lon[0] <= lon && lon <= m_lon[1]);
}

bool GeoRect::contains(const GeoRect & other) const {
	return contains(other.m_lat[0], other.m_lon[0]) && contains(other.m_lat[1], other.m_lon[1]);
}

bool GeoRect::operator==(const GeoRect & other) const {
	for(size_t i = 0; i < 2; i++) {
		if (m_lat[i] != other.m_lat[i] || m_lon[i] != other.m_lon[i])
			return false;
	}
	return true;
}

/** clip this rect at other
	* @return: returns true if clipping worked => rects overlapped, false if they did not overlap */
bool GeoRect::clip(const GeoRect & other) {
	if (!overlap(other))
		return false;
	m_lat[0] = std::max(m_lat[0], other.m_lat[0]);
	m_lat[1] = std::min(m_lat[1], other.m_lat[1]);
	
	m_lon[0] = std::max(m_lon[0], other.m_lon[0]);
	m_lon[1] = std::min(m_lon[1], other.m_lon[1]);
	
	return true;
}

/** Enlarge this rect so that other will fit into it */
void GeoRect::enlarge(const GeoRect & other) {
	m_lat[0] = std::min(m_lat[0], other.m_lat[0]);
	m_lat[1] = std::max(m_lat[1], other.m_lat[1]);
	
	m_lon[0] = std::min(m_lon[0], other.m_lon[0]);
	m_lon[1] = std::max(m_lon[1], other.m_lon[1]);
}

}}//end namespace

std::ostream & operator<<(std::ostream & out, const sserialize::spatial::GeoRect & rect) {
	return out << "GeoRect[(" << rect.lat()[0] << ", " << rect.lon()[0] << "); (" << rect.lat()[1] << ", " << rect.lon()[1]  << ")]";
}

sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & destination, const sserialize::spatial::GeoRect & rect) {
	sserialize::spatial::GeoPoint bottomLeft(rect.lat()[0], rect.lon()[0]);
	sserialize::spatial::GeoPoint topRight(rect.lat()[1], rect.lon()[1]);
	destination << bottomLeft << topRight;
	return destination;
}

sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & src, sserialize::spatial::GeoRect & rect) {
	sserialize::Static::spatial::GeoPoint bL, tR;
	src >> bL >> tR;
	rect.lat()[0] = bL.lat();
	rect.lat()[1] = tR.lat();
	rect.lon()[0] = bL.lon();
	rect.lon()[1] = tR.lon();
	return src;
}
