#include <sserialize/spatial/LatLonCalculations.h>
#include <cmath>

namespace sserialize {
namespace spatial {

inline double sqr(double a) { return a*a;}
inline double toRadian(double deg) { return (deg*M_PI)/180;}
inline double toDegree(double radian) { return (radian*180)/M_PI; }

double bearingTo(double lat0, double lon0, double lat1, double lon1) {
	double ph1(toRadian(lat0)), ph2(toRadian(lat1));
	double deltaLambda = toRadian(lon1-lon0);
	
	double y = ::sin(deltaLambda) * ::cos(ph2);
	double x = ::cos(ph1)* ::sin(ph2) - ::sin(ph1) * ::cos(ph2)* ::cos(deltaLambda);
	double theta = ::atan2(y, x);

	return ::fmod(toDegree(theta)+360.0, 360.0);
}

double distanceTo(double lat0, double lon0, double lat1, double lon1, double earthRadius) {
	double ph1 = toRadian(lat0);
	double ph2 = toRadian(lat1);
	double deltaPh = toRadian(lat1-lat0);
	double deltaLambda = toRadian(lon1-lon0);

	double a = sqr(::sin(deltaPh/2)) + ::cos(ph1) * ::cos(ph2) * sqr(sin(deltaLambda/2));
	double c = 2 * ::atan2(::sqrt(a), ::sqrt(1-a));

	return earthRadius * c;
}

//pathStart=(lat0, lon0), pathEnd=(lat1, lon1), this=(latq, lonq)
double crossTrackDistance(double lat0, double lon0, double lat1, double lon1, double latq, double lonq) {
	double radius(6371e3);

	double delta13 = distanceTo(lat0, lon0, latq, lonq, radius)/radius;
	double theta13 = toRadian( bearingTo(lat0, lon0, latq, lonq) );
	double theta12 = toRadian( bearingTo(lat0, lon0, lat1, lon1) );

	double dxt = ::asin( ::sin(delta13) * ::sin(theta13-theta12) ) * radius;
	return dxt;
}

CrossTrackDistanceCalculator::CrossTrackDistanceCalculator(double lat0, double lon0, double lat1, double lon1) :
m_lat0(lat0),
m_lon0(lon0),
m_lat1(lat1),
m_lon1(lon1),
m_theta12(toRadian( bearingTo(lat0, lon0, lat1, lon1) ))
{}

double CrossTrackDistanceCalculator::operator()(double latq, double lonq) const {
	double radius(6371e3);

	double delta13 = distanceTo(m_lat0, m_lon0, latq, lonq, radius)/radius;
	double theta13 = toRadian( bearingTo(m_lat0, m_lon0, latq, lonq) );

	double dxt = ::asin( ::sin(delta13) * ::sin(theta13-m_theta12) ) * radius;
	return dxt;
}


}}