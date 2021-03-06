#include <sserialize/Static/TriangulationGridLocator.h>
#include <sserialize/utility/exceptions.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

namespace sserialize {
namespace Static {
namespace spatial {

TriangulationGridLocator::TriangulationGridLocator() {}

TriangulationGridLocator::TriangulationGridLocator(const UByteArrayAdapter& d) :
m_trs(d+1),
m_grid(d+(1+m_trs.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_GRID_LOCATOR_VERSION, d.at(0), "sserialize::Static::spatial::TriangulationGridLocator::TriangulationGridLocator");
}

TriangulationGridLocator::~TriangulationGridLocator() {}

UByteArrayAdapter::OffsetType TriangulationGridLocator::getSizeInBytes() const {
	return 1+m_grid.getSizeInBytes()+m_trs.getSizeInBytes();
}

bool TriangulationGridLocator::gridContains(const TriangulationGridLocator::Point& p) const {
	return m_grid.contains(p);
}

bool TriangulationGridLocator::gridContains(double lat, double lon) const {
	return m_grid.contains(lat, lon);
}

bool TriangulationGridLocator::contains(const TriangulationGridLocator::Point& p) const {
	return contains(p.lat(), p.lon());
}

bool TriangulationGridLocator::contains(double lat, double lon) const {
	return faceId(lat, lon) != NullFace;
}

TriangulationGridLocator::FaceId
TriangulationGridLocator::faceHint(double lat, double lon) const {
	try {
		return m_grid.at(lat, lon);
	}
	catch (sserialize::OutOfBoundsException const &) { //no face hint available, this means that the point is outside of the triangulation
		return NullFace;
	}
}

TriangulationGridLocator::FaceId
TriangulationGridLocator::faceHint(const TriangulationGridLocator::Point& p) const {
	return faceHint(p.lat(), p.lon());
}

TriangulationGridLocator::FaceId
TriangulationGridLocator::faceId(double lat, double lon) const {
	if (gridContains(lat, lon)) {//BUG: hint is sometimes wrong
		auto hint = faceHint(lat, lon);
		return m_trs.locate(Point(lat, lon), hint, Triangulation::TT_STRAIGHT);
	}
	return NullFace;
}

TriangulationGridLocator::FaceId
TriangulationGridLocator::faceId(const TriangulationGridLocator::Point& p) const {
	return faceId(p.lat(), p.lon());
}

TriangulationGridLocator::Face TriangulationGridLocator::face(const TriangulationGridLocator::Point& p) const {
	return face(p.lat(), p.lon());
}

TriangulationGridLocator::Face TriangulationGridLocator::face(double lat, double lon) const {
	auto fId = faceId(lat, lon);
	if (fId == NullFace) {
		return Face();
	}
	return m_trs.face(fId);
}

}}}//end namespace
