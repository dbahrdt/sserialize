#include <sserialize/Static/Triangulation.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/printers.h>
#include <sserialize/utility/assert.h>
#include <sserialize/spatial/GeoPolygon.h>


namespace sserialize {
namespace Static {
namespace spatial {

constexpr Triangulation::FaceId Triangulation::NullFace;
constexpr Triangulation::VertexId Triangulation::NullVertex;

//inner classes
Triangulation::Face::Face() :
Face(0, Triangulation::NullFace)
{}

Triangulation::Face::Face(const Triangulation* p, FaceId pos) :
m_p(p),
m_pos(pos)
{}

Triangulation::Face::~Face() {}

bool Triangulation::Face::valid() const {
	return m_p && id() != Triangulation::NullFace;
}

bool Triangulation::Face::isDegenerate() const {
	return m_p->faceInfo().at(m_pos.ut(), FI_IS_DEGENERATE);
}

bool Triangulation::Face::isNeighbor(position_type pos) const {
	return (m_p->faceInfo().at(m_pos.ut(), FI_NEIGHBOR_VALID) & (static_cast<uint8_t>(1) << pos));
}

Triangulation::FaceId Triangulation::Face::neighborId(position_type pos) const {
	if (!isNeighbor(pos)) {
		return Triangulation::NullFace;
	}
	return FaceId(m_p->faceInfo().at(m_pos.ut(), FI_NEIGHBOR_BEGIN+pos));
}

Triangulation::Face Triangulation::Face::neighbor(position_type pos) const {
	auto nId = neighborId(pos);
	if (nId != Triangulation::NullFace) {
		return Face(m_p, nId);
	}
	return Face();
}

Triangulation::VertexId Triangulation::Face::vertexId(position_type pos) const {
	if (pos > 2) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::Face::vertexId");
	}
	return VertexId(m_p->faceInfo().at(m_pos.ut(), FI_VERTEX_BEGIN+pos));
}

Triangulation::VertexId Triangulation::Face::vertexId(const Point & p) const {
	for(position_type i(0); i < 3; ++i) {
		if (p.equal(point(i), 0)) {
			return vertexId(i);
		}
	}
	throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::Face::vertexId");
}

Triangulation::Vertex Triangulation::Face::vertex(position_type pos) const {
	return m_p->vertex(vertexId(pos));
}

Triangulation::Vertex Triangulation::Face::vertex(const Point & p) const {
	return m_p->vertex( vertexId(p) );
}

Triangulation::Point Triangulation::Face::point(position_type pos) const {
	return vertex(pos).point();
}

bool Triangulation::Face::isVertex(const Point & p) const {
	for(position_type i(0); i < 3; ++i) {
		if (p.equal(point(i), 0)) {
			return true;
		}
	}
	return false;
}

bool Triangulation::Face::isOnEdge(const Point & p) const {
	auto w = where(p);
	return CT_ON_EDGE_0 <= w && w <= CT_ON_EDGE_2;
}

int Triangulation::Face::edge(const Point & p) const {
	auto w = where(p);
	if (CT_ON_EDGE_0 <= w && w <= CT_ON_EDGE_2) {
		return w-CT_ON_EDGE_0;
	}
	else {
		return -1;
	}
}

Triangulation::Face::ContainmentType Triangulation::Face::where(const Triangulation::Point & p) const {
	detail::Triangulation::Orientation<Triangulation::Point> ot;
	CGAL::Sign ot0cw = ot(point(0), point(Triangulation::cw(0)), p);
	CGAL::Sign ot0ccw = ot(point(0), point(Triangulation::ccw(0)), p);
	CGAL::Sign otcwccw = ot(point(Triangulation::cw(0)), point(Triangulation::ccw(0)), p);
	
	if (CGAL::RIGHT_TURN == ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::RIGHT_TURN == otcwccw) {
		return CT_INSIDE;
	}
	else if (CGAL::LEFT_TURN != ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::RIGHT_TURN == otcwccw) {
		if (ot0cw == CGAL::COLLINEAR) {
			return Triangulation::Face::ContainmentType(CT_ON_EDGE_0+Triangulation::ccw(0));
		}
		else {
			return CT_INSIDE;
		}
	}
	else if (CGAL::RIGHT_TURN == ot0cw && CGAL::RIGHT_TURN != ot0ccw && CGAL::RIGHT_TURN == otcwccw) {
		if (ot0ccw == CGAL::COLLINEAR) {
			return Triangulation::Face::ContainmentType(CT_ON_EDGE_0+Triangulation::cw(0));
		}
		else {
			return CT_INSIDE;
		}
	}
	else if (CGAL::RIGHT_TURN == ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::LEFT_TURN != otcwccw) {
		if (otcwccw == CGAL::COLLINEAR) {
			return CT_ON_EDGE_0;
		}
		else {
			return CT_INSIDE;
		}
	}
	else if (((CGAL::COLLINEAR == ot0cw)+(CGAL::COLLINEAR == ot0ccw)+(CGAL::COLLINEAR == otcwccw)) == 2) {
		if (CGAL::COLLINEAR == ot0cw && CGAL::COLLINEAR == ot0ccw) {
			return CT_ON_VERTEX_0;
		}
		else if (CGAL::COLLINEAR == ot0cw && CGAL::COLLINEAR == otcwccw) {
			return Triangulation::Face::ContainmentType(CT_ON_VERTEX_0+Triangulation::cw(0));
		}
		else {
			return Triangulation::Face::ContainmentType(CT_ON_VERTEX_0+Triangulation::ccw(0));
		}
	}
	else {
		return CT_OUTSIDE;
	}
};

bool Triangulation::Face::contains(const Triangulation::Point & p) const {
	detail::Triangulation::Orientation<Triangulation::Point> ot;
	CGAL::Sign ot0cw = ot(point(0), point(Triangulation::cw(0)), p);
	CGAL::Sign ot0ccw = ot(point(0), point(Triangulation::ccw(0)), p);
	CGAL::Sign otcwccw = ot(point(Triangulation::cw(0)), point(Triangulation::ccw(0)), p);
	//CGAL::_TURN != x is the same as (CGAL::OPPOSITE_TURN == x || CGAL::COLLINEAR == x)
	//strongly inside is:
// 	(CGAL::RIGHT_TURN == ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::RIGHT == otcwccw) 
	return (CGAL::LEFT_TURN != ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::RIGHT_TURN == otcwccw) ||
			(CGAL::RIGHT_TURN == ot0cw && CGAL::RIGHT_TURN != ot0ccw && CGAL::RIGHT_TURN == otcwccw) ||
			(CGAL::RIGHT_TURN == ot0cw && CGAL::LEFT_TURN == ot0ccw && CGAL::LEFT_TURN != otcwccw) ||
			//bool -> int = (0|1); if 2 are collinear then p is at the corner of these two, the corner is a vertex of the face
			(((CGAL::COLLINEAR == ot0cw)+(CGAL::COLLINEAR == ot0ccw)+(CGAL::COLLINEAR == otcwccw)) == 2);
}

bool Triangulation::Face::intersects(const Triangulation::Point& p, const Triangulation::Point& q) const {
	detail::Triangulation::Do_intersect<Triangulation::Point> ci;
	return ci(p, q, point(0), point(1)) ||
			ci(p, q, point(1), point(2)) ||
			ci(p, q, point(2), point(0));
}

Triangulation::Point Triangulation::Face::centroid() const {
	detail::Triangulation::Compute_centroid<Triangulation::Point> cc;
	return cc(point(0), point(1), point(2) );
}

int Triangulation::Face::index(const Triangulation::Vertex& v) const {
	VertexId vertexId = v.id();
	for(position_type j(0); j < 3; ++j) {
		if (this->vertexId(j) == vertexId) {
			return j;
		}
	}
	return -1;
}

bool Triangulation::Face::operator==(const Triangulation::Face& other) const {
	return m_pos == other.m_pos && m_p == other.m_p;
}

bool Triangulation::Face::operator!=(const Triangulation::Face& other) const {
	return m_pos != other.m_pos || m_p != other.m_p;
}

double
Triangulation::Face::area() const {
	double A;
	struct geod_geodesic g;
	struct geod_polygon p;
	geod_init(&g, 6378137, 1/298.257223563);
	geod_polygon_init(&p, 0);
	
	Point p0(point(0)), p1(point(1)), p2(point(2));

	geod_polygon_addpoint(&g, &p, p0.lat(), p0.lon());
	geod_polygon_addpoint(&g, &p, p1.lat(), p1.lon());
	geod_polygon_addpoint(&g, &p, p2.lat(), p2.lon());
	geod_polygon_compute(&g, &p, 0, 1, &A, 0);
	return std::abs<double>(A);
}

void Triangulation::Face::dump(std::ostream& out) const {
	out << "Triangulation::Face {\n";
	out << "\tid=" << id() << "\n";
	out << "\tneighbor_valid=" << m_p->faceInfo().at(m_pos.ut(), FI_NEIGHBOR_VALID) << "\n";
	for(position_type j(0); j < 3; ++j) {
		out << "\tneighbor[" << j << "]=";
		if (isNeighbor(j)) {
			out << neighborId(j);
		}
		else {
			out << "invalid";
		}
		out << "\n";
	}
	for(position_type j(0); j < 3; ++j) {
		out << "\tvertex[" << j << "]=" << vertexId(j)  << "\n";
	}
	for(position_type j(0); j < 3; ++j) {
		out << "\tpoint[" << j << "]=" << vertex(j).point()  << "\n";
	}
	out << "}";
}

void Triangulation::Face::dump() const {
	dump(std::cout);
}


//Vertex definitions

Triangulation::Vertex::Vertex() :
Vertex(0, Triangulation::NullVertex)
{}

Triangulation::Vertex::Vertex(const Triangulation* p, VertexId pos) :
m_p(p),
m_pos(pos)
{}

Triangulation::Vertex::~Vertex() {}

bool Triangulation::Vertex::valid() const {
	return m_p && id() != Triangulation::NullVertex;
}

Triangulation::Point Triangulation::Vertex::point() const {
	return m_p->points().at(m_pos.ut());
}

Triangulation::FaceId Triangulation::Vertex::beginFaceId() const {
	return FaceId(m_p->vertexInfos().at(m_pos.ut(), VI_FACES_BEGIN));
}

Triangulation::FaceId Triangulation::Vertex::endFaceId() const {
	return FaceId(m_p->vertexInfos().at(m_pos.ut(), VI_FACES_END));
}

Triangulation::Face Triangulation::Vertex::beginFace() const {
	return m_p->face(beginFaceId());
}

Triangulation::Face Triangulation::Vertex::endFace() const {
	return m_p->face(endFaceId());
}

void Triangulation::Vertex::dump(std::ostream& out) const {
	out << "Triangulation::Vertex {\n";
	out << "\tid=" << id() << "\n";
	out << "\tpoint=" << point() << "\n";
	out << "\tbeginFaceId=" << beginFaceId() << "\n";
	out << "\tendFaceId=" << endFaceId() << "\n";
	FaceCirculator fIt(facesBegin()), fE(facesEnd());
	for(FaceCirculator::position_type i(0);true; ++fIt, ++i) {
		out << "\tface[" << i << "]=" << fIt.face().id() << std::endl;
		if (fIt == fE) {
			break;
		}
	}
	out << "}";
}

void Triangulation::Vertex::dump() const {
	dump(std::cout);
}

Triangulation::FaceCirculator Triangulation::Vertex::facesBegin() const {
	return FaceCirculator(*this, beginFace());
}

Triangulation::FaceCirculator Triangulation::Vertex::facesEnd() const {
	return FaceCirculator(*this, endFace());
}

Triangulation::FaceCirculator Triangulation::Vertex::faces() const {
	return FaceCirculator(*this, m_p->face(FaceId(m_p->vertexInfos().at(m_pos.ut(), VI_FACES_BEGIN))));
}

bool Triangulation::Vertex::operator!=(const Triangulation::Vertex& other) const {
	return m_pos != other.m_pos || m_p != other.m_p;
}

bool Triangulation::Vertex::operator==(const Triangulation::Vertex& other) const {
	return m_pos == other.m_pos && m_p == other.m_p;
}

Triangulation::FaceCirculator::FaceCirculator(const Triangulation::Vertex& v, const Triangulation::Face& f) :
m_f(f),
m_v(v)
{
	SSERIALIZE_NORMAL_ASSERT(m_f.index(m_v) != -1);
}

Triangulation::FaceCirculator::~FaceCirculator() {}

Triangulation::Face Triangulation::FaceCirculator::operator*() const {
	return m_f;
}

Triangulation::Face Triangulation::FaceCirculator::operator->() const {
	return m_f;
}

bool Triangulation::FaceCirculator::operator==(const Triangulation::FaceCirculator& other) const {
	return m_f == other.m_f && m_v == other.m_v;
}

bool Triangulation::FaceCirculator::operator!=(const Triangulation::FaceCirculator& other) const {
	return m_f != other.m_f || m_v != other.m_v;
}

Triangulation::FaceCirculator& Triangulation::FaceCirculator::operator++() {
	if (m_f.id() != m_v.endFaceId()) {
		int i = m_f.index(m_v);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(i, 0);
		m_f = m_f.neighbor(Triangulation::ccw(i));
	}
	else {
		m_f = m_v.beginFace();
	}
	return *this;
}

Triangulation::FaceCirculator Triangulation::FaceCirculator::operator++(int) {
	FaceCirculator tmp(*this);
	this->operator++();
	return tmp;
}

Triangulation::FaceCirculator& Triangulation::FaceCirculator::operator--() {
	if (m_f.id() != m_v.beginFaceId()) {
		int i = m_f.index(m_v);
		SSERIALIZE_CHEAP_ASSERT_LARGER_OR_EQUAL(i, 0);
		m_f = m_f.neighbor((position_type)Triangulation::cw(i));
	}
	else {
		m_f = m_v.endFace();
	}
	return *this;
}

Triangulation::FaceCirculator Triangulation::FaceCirculator::operator--(int) {
	FaceCirculator tmp(*this);
	this->operator--();
	return tmp;
}

const Triangulation::Face& Triangulation::FaceCirculator::face() const {
	return m_f;
}

const Triangulation::Vertex& Triangulation::FaceCirculator::vertex() const {
	return m_v;
}

Triangulation::Triangulation() :
m_features(Triangulation::F_CLEAN_GEOMETRY)
{}

Triangulation::Triangulation(const UByteArrayAdapter& d) :
m_features(d.at(1)),
m_p(d+SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE),
m_vi(d+(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE+m_p.getSizeInBytes())),
m_fi(d+(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE+m_p.getSizeInBytes()+m_vi.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION, d.at(0), "sserialize::Static::spatial::Triangulation::Triangulation");
	SSERIALIZE_EQUAL_LENGTH_CHECK(m_p.size(), m_vi.size(), "sserialize::Static::spatial::Triangulation::Triangulation: m_vi != m_p");
	SSERIALIZE_VERY_EXPENSIVE_ASSERT(selfCheck());
}

Triangulation::~Triangulation() {}

UByteArrayAdapter::OffsetType Triangulation::getSizeInBytes() const {
	return SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE+m_vi.getSizeInBytes()+m_p.getSizeInBytes()+m_fi.getSizeInBytes();
}

Triangulation::Face Triangulation::face(FaceId pos) const {
	if (pos >= faceCount()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::face");
	}
	return Face(this, pos); 
}

Triangulation::Vertex Triangulation::vertex(VertexId pos) const {
	if (pos >= vertexCount()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::vertex");
	}
	return Vertex(this, pos); 
}

bool Triangulation::selfCheck() const {
	//check face-neigbor relations
	for(FaceId faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		for(Face::position_type j(0); j < 3; ++j) {
			if (!f.isNeighbor(j)) {
				continue;
			}
			Face fn(f.neighbor(j));
			bool ok = false;
			for(Face::position_type k(0); k < 3; ++k) {
				if (fn.neighborId(k) == faceId) {
					ok = true;
				}
			}
			if (!ok) {
				return false;
			}
		}
	}
	//now check face-edge-face-neighbor relations. see http://doc.cgal.org/latest/Triangulation_2/ for the spec
	//The three vertices of a face are indexed with 0, 1 and 2 in counterclockwise order.
	//The neighbors of a face are also indexed with 0,1,2 in such a way that the neighbor indexed by i is opposite to the vertex with the same index.
	//See Figure 36.2, the functions ccw(i) and cw(i) shown on this figure compute respectively i+1 and i−1 modulo 3
	for(FaceId faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		for(Face::position_type j(0); j < 3; ++j) {
			Vertex vs(f.vertex(j)), ve(f.vertex(cw(j)));
			if (f.isNeighbor(ccw(j))) {
				Face fn(f.neighbor(ccw(j)));
				for(int k(0); k < 3; ++k) {
					if (fn.vertexId(k) == vs.id()) {
						bool ok = false;
						if (fn.vertexId(cw(k)) == ve.id()) {
							ok = fn.neighborId(ccw(k)) == f.id();
						}
						else if (fn.vertexId(ccw(k)) == ve.id()) {
							ok = fn.neighborId(cw(k)) == f.id();
						}
						if (!ok) {
							return false;
						}
					}
				}
			}
		}
	}
	for(FaceId faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		Vertex v0(f.vertex(0)), v1(f.vertex(1)), v2(f.vertex(2));
		using sserialize::spatial::equal;
		if (v0.id() == v1.id() || v0.id() == v2.id() || v1.id() == v2.id() ||
			equal(v0.point(), v1.point(), 0.0) || equal(v0.point(), v2.point(), 0.0) || equal(v1.point(), v2.point(), 0.0))
		{
			return false;
		}
	}
	for(VertexId vertexId(0), s(vertexCount()); vertexId < s; ++vertexId) {
		Vertex v(vertex(vertexId));
		FaceCirculator fc(v.faces());
		FaceCirculator fcIt(fc);
		if (fcIt.face().index(v) == -1) {
			return false;
		}
		for(++fcIt; fcIt != fc; ++fcIt) {
			if (fcIt.face().index(v) == -1) {
				return false;
			}
		}
	}
	return true;
}


void Triangulation::printStats(std::ostream& out) const {
	out << "sserialize::Static::spatial::Triangulation::stats {\n";
	out << "\t#vertices=" << m_p.size() << "\n";
	out << "\t#faces=" << m_fi.size() << "\n";
	out << "\tTotal size=" << sserialize::prettyFormatSize(getSizeInBytes()) << "\n";
	out << "}";
}

Triangulation::FaceId Triangulation::locate(const Point& target, FaceId hint, TraversalType tt) const {
	if (!faceCount()) {
		return NullFace;
	}
	if (hint >= faceCount()) {
		hint = FaceId(0);
	}
	return traverse(target, face(hint).centroid(), [](Face const &) {}, hint, tt);
}

}}}//end namespace
