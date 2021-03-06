#ifndef SSERIALIZE_STATIC_SPATIAL_POINT_ON_S2_H
#define SSERIALIZE_STATIC_SPATIAL_POINT_ON_S2_H
#include <sserialize/spatial/PointOnS2.h>
#include <sserialize/storage/UByteArrayAdapter.h>

/** This is a special class to encode Point in 3D with rational coordinates using libratss
  * In libratss all coordinates of a point have the same denominator 
  *
  * File format of fixed format:
  * u64|s64|s64|s64
  */

namespace sserialize {
namespace Static {
namespace spatial {
namespace ratss {

class PointOnS2: public sserialize::spatial::ratss::PointOnS2 {
public:
	typedef sserialize::spatial::ratss::PointOnS2 MyBaseClass;
public:
	PointOnS2();
	explicit PointOnS2(const sserialize::spatial::GeoPoint & gp);
	explicit PointOnS2(int64_t xnum, int64_t ynum, int64_t znum, uint64_t denom);
	explicit PointOnS2(sserialize::UByteArrayAdapter src);
	virtual ~PointOnS2();
};

}}}}//end namespace

#endif