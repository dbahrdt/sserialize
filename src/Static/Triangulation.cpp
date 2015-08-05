#include <sserialize/Static/Triangulation.h>
#include <sserialize/utility/exceptions.h>
#include <sserialize/utility/printers.h>


namespace sserialize {
namespace Static {
namespace spatial {

constexpr uint32_t Triangulation::NullFace;
constexpr uint32_t Triangulation::NullVertex;

//inner classes
Triangulation::Face::Face() :
Face(0, Triangulation::NullFace)
{}

Triangulation::Face::Face(const Triangulation* p, uint32_t pos) :
m_p(p),
m_pos(pos)
{}

Triangulation::Face::~Face() {}

bool Triangulation::Face::valid() const {
	return m_p;
}

bool Triangulation::Face::isNeighbor(uint8_t pos) const {
	return (m_p->faceInfo().at(m_pos, FI_NEIGHBOR_VALID) & (static_cast<uint8_t>(1) << pos));
}

uint32_t Triangulation::Face::neighborId(uint8_t pos) const {
	if (!isNeighbor(pos)) {
		return Triangulation::NullFace;
	}
	return m_p->faceInfo().at(m_pos, FI_NEIGHBOR_BEGIN+pos);
}

Triangulation::Face Triangulation::Face::neighbor(uint32_t pos) const {
	uint32_t nId = neighborId(pos);
	if (nId != Triangulation::NullFace) {
		return Face(m_p, nId);
	}
	return Face();
}

uint32_t Triangulation::Face::vertexId(uint32_t pos) const {
	if (pos > 2) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::Face::vertexId");
	}
	return m_p->faceInfo().at(m_pos, FI_VERTEX_BEGIN+pos);
}

Triangulation::Vertex Triangulation::Face::vertex(uint32_t pos) const {
	return Vertex(m_p, vertexId(pos));
}

Triangulation::Point Triangulation::Face::point(uint32_t pos) const {
	return m_p->points().at(vertexId(pos));
}

Triangulation::Point Triangulation::Face::centroid() const {
	Triangulation::Point p[] = { point(0), point(1), point(2) };
	double lat = (p[0].lat() + p[1].lat() + p[2].lat()) / 3;
	double lon = (p[0].lon() + p[1].lon() + p[2].lon()) / 3;
	return Triangulation::Point(lat, lon);
}

int Triangulation::Face::index(const Triangulation::Vertex& v) const {
	uint32_t vertexId = v.id();
	for(int j(0); j < 3; ++j) {
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

void Triangulation::Face::dump(std::ostream& out) const {
	out << "Triangulation::Face {\n";
	out << "\tid=" << id() << "\n";
	out << "\tneighbor_valid=" << m_p->faceInfo().at(m_pos, FI_NEIGHBOR_VALID) << "\n";
	for(int j(0); j < 3; ++j) {
		out << "\tneighbor[" << j << "]=" << neighborId(j)  << "\n";
	}
	for(int j(0); j < 3; ++j) {
		out << "\tvertex[" << j << "]=" << vertexId(j)  << "\n";
	}
	for(int j(0); j < 3; ++j) {
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

Triangulation::Vertex::Vertex(const Triangulation* p, uint32_t pos) :
m_p(p),
m_pos(pos)
{}

Triangulation::Vertex::~Vertex() {}

bool Triangulation::Vertex::valid() const {
	return id() != Triangulation::NullVertex;
}

Triangulation::Point Triangulation::Vertex::point() const {
	return m_p->points().at(m_pos);
}

uint32_t Triangulation::Vertex::beginFaceId() const {
	return m_p->vertexInfos().at(m_pos, VI_FACES_BEGIN);
}

uint32_t Triangulation::Vertex::endFaceId() const {
	return m_p->vertexInfos().at(m_pos, VI_FACES_END);
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
	return FaceCirculator(*this, m_p->face(m_p->vertexInfos().at(m_pos, VI_FACES_BEGIN)));
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
	assert(m_f.index(m_v) != -1);
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
		m_f = m_f.neighbor(Triangulation::cw(i));
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

Triangulation::Triangulation() {}

Triangulation::Triangulation(const UByteArrayAdapter& d) :
m_p(d+1),
m_vi(d+(1+m_p.getSizeInBytes())),
m_fi(d+(1+m_p.getSizeInBytes()+m_vi.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION, d.at(0), "sserialize::Static::spatial::Triangulation::Triangulation");
	SSERIALIZE_EQUAL_LENGTH_CHECK(m_p.size(), m_vi.size(), "sserialize::Static::spatial::Triangulation::Triangulation: m_vi != m_p");
	assert(selfCheck());
}

Triangulation::~Triangulation() {}

UByteArrayAdapter::OffsetType Triangulation::getSizeInBytes() const {
	return 1+m_vi.getSizeInBytes()+m_p.getSizeInBytes()+m_fi.getSizeInBytes();
}

Triangulation::Face Triangulation::face(uint32_t pos) const {
	if (pos >= faceCount()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::face");
	}
	return Face(this, pos); 
}

Triangulation::Vertex Triangulation::vertex(uint32_t pos) const {
	if (pos >= vertexCount()) {
		throw sserialize::OutOfBoundsException("sserialize::Static::spatial::Triangulation::vertex");
	}
	return Vertex(this, pos); 
}

bool Triangulation::selfCheck() const {
	//check face-neigbor relations
	for(uint32_t faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		for(int j(0); j < 3; ++j) {
			if (!f.isNeighbor(j)) {
				continue;
			}
			Face fn(f.neighbor(j));
			bool ok = false;
			for(int k(0); k < 3; ++k) {
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
	for(uint32_t faceId(0), s(faceCount()); faceId < s; ++faceId) {
		Face f(face(faceId));
		for(int j(0); j < 3; ++j) {
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
	for(uint32_t vertexId(0), s(vertexCount()); vertexId < s; ++vertexId) {
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



}}}//end namespace