#ifndef SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_H
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_VERSION 3
#define SSERIALIZE_STATIC_SPATIAL_TRIANGULATION_FIXED_HEADER_SIZE 2
#include <sserialize/containers/MultiVarBitArray.h>
#include <sserialize/Static/Array.h>
#include <sserialize/containers/OOMArray.h>
#include <sserialize/containers/ArraySet.h>
#include <sserialize/spatial/DistanceCalculator.h>
#include <sserialize/Static/detail/Triangulation.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/utility/strongtypedefs.h>

#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
	#include <sserialize/algorithm/hashspecializations.h>
#endif

#include <queue>

namespace sserialize::Static::spatial::detail::Triangulation {

	class VertexId:
		public sserialize::st::strong_type_ca<
			VertexId,
			sserialize::Size,
			sserialize::st::Arithmetic,
			sserialize::st::CompareAll,
			sserialize::st::Serialize,
			sserialize::st::Deserialize,
			sserialize::st::OStream<std::ostream>
		>
	{
	public:
		using underlying_type = sserialize::Size;
		static constexpr underlying_type npos = std::numeric_limits<underlying_type>::max();
		using Self = VertexId;
	public:
		constexpr VertexId() : m_v(npos) {}
		explicit constexpr VertexId(underlying_type v) : m_v(v) {}
		constexpr VertexId(Self const &) = default;
		Self & operator=(Self const &) = default;
	public:
		inline friend bool operator >(Self const & a, underlying_type const & b) { return a.m_v > b; }
		inline friend bool operator >=(Self const & a, underlying_type const & b) { return a.m_v >= b; }
		inline friend bool operator <(Self const & a, underlying_type const & b) { return a.m_v < b; }
		inline friend bool operator <=(Self const & a, underlying_type const & b) { return a.m_v <= b; } 
	public:
		inline explicit operator underlying_type const & () const { return m_v; }
		inline explicit operator underlying_type& () { return m_v; }
	public:
		inline underlying_type const & ut() const { return m_v; }
	private:
		underlying_type m_v{npos};
	};
	
	class FaceId:
		public sserialize::st::strong_type_ca<
			FaceId,
			sserialize::Size,
			sserialize::st::Arithmetic,
			sserialize::st::CompareAll,
			sserialize::st::Serialize,
			sserialize::st::Deserialize,
			sserialize::st::OStream<std::ostream>
		>
	{
	public:
		using underlying_type = sserialize::Size;
		static constexpr underlying_type npos = std::numeric_limits<underlying_type>::max();
		using Self = FaceId;
	public:
		constexpr FaceId() : m_v(npos) {}
		explicit constexpr FaceId(underlying_type v) : m_v(v) {}
		constexpr FaceId(Self const &) = default;
		Self & operator=(Self const &) = default;
	public:
		inline friend bool operator >(Self const & a, underlying_type const & b) { return a.m_v > b; }
		inline friend bool operator >=(Self const & a, underlying_type const & b) { return a.m_v >= b; }
		inline friend bool operator <(Self const & a, underlying_type const & b) { return a.m_v < b; }
		inline friend bool operator <=(Self const & a, underlying_type const & b) { return a.m_v <= b; } 
	public:
		inline explicit operator underlying_type const & () const { return m_v; }
		inline explicit operator underlying_type& () { return m_v; }
	public:
		inline underlying_type const & ut() const { return m_v; }
	private:
		underlying_type m_v{npos};
	};
	
} //end namespace sserialize::Static::spatial::detail::Triangulation

namespace std {
	
template<>
struct hash<sserialize::Static::spatial::detail::Triangulation::VertexId> {
	using type = sserialize::Static::spatial::detail::Triangulation::VertexId;
	std::hash<type::underlying_type> m_h;
	std::size_t operator()(type const & v) const {
		return m_h(v.ut());
	}
};

template<>
struct hash<sserialize::Static::spatial::detail::Triangulation::FaceId> {
	using type = sserialize::Static::spatial::detail::Triangulation::FaceId;
	std::hash<type::underlying_type> m_h;
	std::size_t operator()(type const & v) const {
		return m_h(v.ut());
	}
};
	
}

namespace sserialize::Static::spatial {

/**
  * Layout (v3)
  * {
  *   VERSION u8
  *   u8 feature flag
  *   POINTS sserialize::Array<Point> gp (id identical with vertex ids)
  *   VERTICES sserialize::MultiVarBitArray
  *   {
  *      FacesBegin u?
  *      FacesEnd u?
  *   }
  *   FACES    sserialize::MultiVarBitArray
  *   {
  *      ValidNeighbor u3
  *      Neighbors 3*u?
  *      Vertices 3*u?
  *   }
  * }
  * Changelog:
  * add feature flags
  */

///Triangulation of the convex-hull of a set of points
class Triangulation final {
private:
	typedef sserialize::MultiVarBitArray FaceInfos;
	typedef sserialize::MultiVarBitArray VertexInfos;
public:
	typedef enum {F_CLEAN_GEOMETRY=0x1, F_DEGENERATE_FACES=0x2, F_BROKEN_GEOMETRY=0x4} FeatureFlags;
	typedef enum {GCT_NONE=0, GCT_REMOVE_DEGENERATE_FACES, GCT_SNAP_VERTICES, GCT_SNAP_ROUND} GeometryCleanType;
	typedef enum {TT_ZIG_ZAG=0, TT_STRAIGHT=1} TraversalType;
	using Point = sserialize::spatial::GeoPoint;
	using PointsContainer = sserialize::Static::Array<Point>;
	
	using SizeType = PointsContainer::SizeType;
	
	class Face;
	class Vertex;
	class FaceCirculator;
	
	using VertexId = detail::Triangulation::VertexId;
	using FaceId = detail::Triangulation::FaceId;
	
	class Vertex final {
	private:
		friend class Triangulation;
		friend class Face;
		friend class FaceCirculator;
	private:
		typedef enum {
			VI_FACES_BEGIN=0, VI_FACES_END=1, VI__NUMBER_OF_ENTRIES=2
		} VertexInfo;
	private:
		const Triangulation * m_p;
		VertexId m_pos;
	private:
		Vertex(const Triangulation * p, VertexId pos);
		FaceId beginFaceId() const;
		FaceId endFaceId() const;
		Face beginFace() const;
		Face endFace() const;
	public:
		Vertex();
		~Vertex();
		bool valid() const;
		inline VertexId id() const { return m_pos; }
		Point point() const;
		FaceCirculator faces() const;
		FaceCirculator facesBegin() const;
		///This iterator points to the LAST face and NOT! one-passed the end
		FaceCirculator facesEnd() const;
		bool operator==(const Vertex & other) const;
		bool operator!=(const Vertex & other) const;
		void dump(std::ostream & out) const;
		void dump() const;
	};

	///A Face has up to 3 neighbors
	class Face final {
	private:
		friend class Triangulation;
		friend class Vertex;
		friend class FaceCirculator;
	private:
		typedef enum : uint32_t {
			FI_NEIGHBOR_VALID=0,
			FI_NEIGHBOR0=1, FI_NEIGHBOR1=2, FI_NEIGHBOR2=3, FI_NEIGHBOR_BEGIN=FI_NEIGHBOR0, FI_NEIGHBOR_END=FI_NEIGHBOR2+1,
			FI_VERTEX0=4, FI_VERTEX1=5, FI_VERTEX2=6, FI_VERTEX_BEGIN=FI_VERTEX0, FI_VERTEX_END=FI_VERTEX2+1,
			FI_IS_DEGENERATE=7, FI_ADDITIONAL_INFO_BEGIN=FI_IS_DEGENERATE, FI_ADDITIONAL_INFO_END=FI_IS_DEGENERATE+1,
			FI__NUMBER_OF_ENTRIES=FI_ADDITIONAL_INFO_END
		} FaceInfo;
	private:
		const Triangulation * m_p;
		FaceId m_pos;
	private:
		Face(const Triangulation * p, FaceId pos);
	public:
		enum ContainmentType {
			CT_OUTSIDE=0x0,
			CT_INSIDE=0x1,
			CT_ON_EDGE_0=0x2, CT_ON_EDGE_1=CT_ON_EDGE_0+1, CT_ON_EDGE_2=CT_ON_EDGE_0+2,
			CT_ON_VERTEX_0=0x8, CT_ON_VERTEX_1=CT_ON_VERTEX_0+1, CT_ON_VERTEX_2=CT_ON_VERTEX_0+2
		};
		using position_type = uint32_t;
	public:
		Face();
		~Face();
		inline FaceId id() const { return m_pos; }
		bool valid() const;
		///a face is degenerate if any two vertices have the same coordinates
		///This may happen since the static version uses fixed precision numbers
		///The topology is still correct
		bool isDegenerate() const;
		bool isNeighbor(position_type pos) const;
		FaceId neighborId(position_type pos) const;
		Face neighbor(position_type pos) const;
		VertexId vertexId(position_type pos) const;
		VertexId vertexId(const Point & p) const;
		Vertex vertex(position_type pos) const;
		Vertex vertex(const Point & p) const;
		Point point(position_type pos) const;
		bool isVertex(const Point & p) const;
		bool isOnEdge(const Point & p) const;
		///return the edge on which the point p lies
		int edge(const Point & p) const;
		ContainmentType where(const Point & p) const;
		bool contains(const Point & p) const;
		bool intersects(const Point & p, const Point & q) const;
		///inexact computed centroid
		Point centroid() const;
		///index of the vertex, -1 if vertex is not part of this face
		int index(const Vertex & v) const;
		void dump(std::ostream & out) const;
		void dump() const;
		bool operator==(const Face & other) const;
		bool operator!=(const Face & other) const;
		//inexact area computation on sphere s^2 in m^2
		double area() const;
		
	};
	
	class FaceCirculator final {
	public:
		using position_type = uint32_t;
	private:
		Face m_f;
		Vertex m_v;
	public:
		FaceCirculator(const Vertex & v, const Face & f);
		~FaceCirculator();
		Face operator*() const;
		Face operator->() const;
		FaceCirculator & operator++();
		FaceCirculator operator++(int);
		FaceCirculator & operator--();
		FaceCirculator operator--(int);
		bool operator==(const FaceCirculator & other) const;
		bool operator!=(const FaceCirculator & other) const;
		const Vertex & vertex() const;
		const Face & face() const;
	};
public:
	static constexpr FaceId NullFace{FaceId::npos};
	static constexpr VertexId NullVertex{VertexId::npos};
private:
	uint8_t m_features;
	PointsContainer m_p;
	VertexInfos m_vi;
	FaceInfos m_fi;
protected:
	inline const FaceInfos & faceInfo() const { return m_fi; }
	inline const VertexInfos & vertexInfos() const { return m_vi; }
	inline const PointsContainer & points() const { return m_p; }
public:
	Triangulation();
	Triangulation(const sserialize::UByteArrayAdapter & d);
	~Triangulation();
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	SizeType vertexCount() const { return m_vi.size(); }
	SizeType faceCount() const { return m_fi.size(); }
	Face face(FaceId pos) const;
	Vertex vertex(VertexId pos) const;
	
	///traverse the triangulation in a straight line starting from source to target
	///@return faceid where the destination point is inside or NullFace
	///@param visitor operator()(const Face & face)
	template<typename TVisitor>
	FaceId traverse(const Point & target, const Point & source, TVisitor visitor, FaceId sourceHint = NullFace, TraversalType tt = TT_ZIG_ZAG) const;
	
	///Locate the face the point lies in, need exact predicates, hint: id of start face
	///Traversal types may return different faces iff target is equal to a vertex point
	FaceId locate(const Point & target, FaceId hint = NullFace, TraversalType tt = TT_ZIG_ZAG) const;
	
	///Explores the triangulation starting at startFace
	///@param explorer operator()(const Face & face) -> bool, return false if the exploration should stop at this face (neighbors of this face are not explored)
	template<typename T_EXPLORER>
	void explore(FaceId startFace, T_EXPLORER explorer) const;
	bool selfCheck() const;
	void printStats(std::ostream & out) const;
	//counter-clock-wise next vertex/neighbor as defined in cgal
	inline static int ccw(const int i) { return (i+1)%3; }
	//clock-wise next vertex/neighbor as defined in cgal
	inline static int cw(const int i) { return (i+2)%3; }
	
	//counter-clock-wise next vertex/neighbor as defined in cgal
	inline static Face::position_type ccw(const Face::position_type i) { return (i+1)%3; }
	//clock-wise next vertex/neighbor as defined in cgal
	inline static Face::position_type cw(const Face::position_type i) { return (i+2)%3; }

	///prepare triangulation for serialization (currently contracts faces that are not representable)
	///@param minEdgeLength the minimal length an edge has to have in order to be processed
	template<typename T_CTD, typename T_REMOVED_EDGES = detail::Triangulation::PrintRemovedEdges>
	static std::size_t prepare(T_CTD& ctd, T_REMOVED_EDGES re, GeometryCleanType gct, double minEdgeLength);
	
	template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
	static sserialize::UByteArrayAdapter & append(T_CGAL_TRIANGULATION_DATA_STRUCTURE& src, T_FACE_TO_FACE_ID_MAP& faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP& vertexToVertexId, sserialize::UByteArrayAdapter& dest, GeometryCleanType gct);
	
protected:
	template<typename TVisitor, bool T_BROKEN_GEOMETRY>
	FaceId traverse_zz_imp(const Point & target, FaceId hint, TVisitor visitor) const;
	template<typename TVisitor>
	FaceId traverse_straight_imp(const Point & target, const Point & source, const Face & startFace, TVisitor visitor) const;

	
};

template<typename TVisitor>
Triangulation::FaceId Triangulation::traverse(const Point& target, const Point& source, TVisitor visitor, FaceId sourceHint, TraversalType tt) const {

	if (sourceHint >= faceCount() || !face(sourceHint).contains(source)) {
		sourceHint = locate(source);
	}
	
	if (sourceHint == NullFace) {
		return NullFace;
	}

	if (tt == TT_ZIG_ZAG) {
		if (m_features & (F_BROKEN_GEOMETRY | F_DEGENERATE_FACES)) {
			return traverse_zz_imp<TVisitor, true>(target, sourceHint, visitor);
		}
		else {
			return traverse_zz_imp<TVisitor, false>(target, sourceHint, visitor);
		}
	}
	else if (TT_STRAIGHT) {
		assert((m_features & (F_BROKEN_GEOMETRY | F_DEGENERATE_FACES)) == 0);
		return traverse_straight_imp(target, source, face(sourceHint), visitor);
	}
	
	throw std::invalid_argument("sserialize::Static::Triangulation::traverse: invalid traversal type");
	return NullFace;
}

template<typename TVisitor, bool T_BROKEN_GEOMETRY>
Triangulation::FaceId Triangulation::traverse_zz_imp(const Point & target, FaceId hint, TVisitor visitor) const {
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef typename K::FT FT;
	typedef typename K::Point_2 Point_2;
	typedef typename K::Orientation_2 Orientation_2;
	typedef typename K::Collinear_are_ordered_along_line_2 Collinear_are_ordered_along_line_2;
	typedef typename K::Compute_squared_distance_2 Compute_squared_distance_2;
	if (!faceCount()) {
		return NullFace;
	}
	K traits;
	
	if (hint >= faceCount()) {
		hint = FaceId(0);
	}
	//(p,q,r) ->  CGAL::Orientation
	Orientation_2 ot(traits.orientation_2_object());
	//(p,q,r) = 1 iff q is between p and r
	Collinear_are_ordered_along_line_2 oal(traits.collinear_are_ordered_along_line_2_object());
	Compute_squared_distance_2 sqd(traits.compute_squared_distance_2_object());

	detail::Triangulation::Convert<Triangulation::Point, Point_2> mp2kp;

	Vertex circleVertex = face(hint).vertex(0);

	Point_2 p( mp2kp(circleVertex.point()) ); //start point
	Point_2 q( mp2kp(target) ); //target

	//TODO: there's currently no way to tell that the point is identical with a vertex
	auto returnFaceFromVertex = [](const Vertex & v) -> FaceId {
		return v.facesBegin().face().id();
	};
	
	//does a bfs from startFace in the direction of target until we are at least 10*InPoint::precision away from our startFace
	//This needs that p is reset to circleVertex
	FT bfsMinDistFromStart(
		sqd(
			Point_2(sserialize::spatial::GeoPoint::toDoubleLat(0), sserialize::spatial::GeoPoint::toDoubleLon(0)),
			Point_2(sserialize::spatial::GeoPoint::toDoubleLat(10), sserialize::spatial::GeoPoint::toDoubleLon(10))
		)
	);
	auto bfs = [this, &target, &q, &sqd, &mp2kp, &bfsMinDistFromStart](const Face & startFace) -> Vertex {
		Point_2 startPoint = mp2kp(this->face(startFace.id()).centroid());
		sserialize::ArraySet<FaceId> visited;
		std::vector<FaceId> queue;

		visited.insert(startFace.id());
		queue.emplace_back(startFace.id());
		for(std::size_t i(0); i < queue.size(); ++i) {
			Face f(this->face(queue[i]));
			//check if current face contains target 
			//check if we're far enough away from our start point
			//use the closest of the 3 vertices to do the circle step afterwards
			if (f.contains(target) || sqd(mp2kp(f.centroid()), startPoint) > bfsMinDistFromStart) {
				uint32_t closest = 0;
				FT minDist(std::numeric_limits<double>::max());
				for(uint32_t j(0); j < 3; ++j) {
					FT myDist = sqd(mp2kp(f.point(j)), q);
					if (myDist < minDist) {
						closest = j;
						minDist = myDist;
					}
				}
				return f.vertex(closest);
			}
			//add neighborhood
			for(uint32_t j(0); j < 3; ++j) {
				if (f.isNeighbor(j)) {
					auto fnId = f.neighborId(j);
					if (!visited.count(fnId)) {
						visited.insert(fnId);
						queue.push_back(fnId);
					}
				}
			}
		}
		throw std::runtime_error("sserialize::Static::spatial::Triangulation::traverse: could not reach target");
		return Vertex();
	};
	
	Point_2 cv;
	Point_2 rp,lp;
	Vertex rv, lv;
	Face curFace;
	
	//BEGIN T_BROKEN_GEOMETRY
	FT distToTgt = sqd(p, q);
	std::size_t stepCount = 0;
	detail::Triangulation::CircularArraySet<16, FaceId> lastTriangs; //we set this to 16*4 Bytes = one cache line on most processors
	//END T_BROKEN_GEOMETRY
	
	//if T_BROKEN_GEOMETRY then we check every 1000? steps or so if we're closer.
	//We also check the 10? last seen triangles if we've already visited one
	//If either checks fails, then something is wrong
	
	while (true) {
		++stepCount;
		//this check is expensive and should not be called very often
		//This essentially checks if we made progress and if not in uses bfs to guarantee it
		//distToTgt get updates every 0xF steps AFTER this
		if ((stepCount & 0x3FF) == 0x3FF) {
			FT myDist;
			if (circleVertex.valid()) {
				myDist = sqd(mp2kp(circleVertex.point()), q);
			}
			else {
				myDist = sqd(mp2kp(curFace.centroid()), q);
			}
			if (myDist >= distToTgt) { //we're not closer than before
				if (circleVertex.valid()) {
					circleVertex = bfs(circleVertex.beginFace());
				}
				else {
					circleVertex = bfs(curFace);
					curFace = Face();
				}
				distToTgt = sqd(mp2kp(circleVertex.point()), q);
			}
		}
		
		if (circleVertex.valid()) {
			cv = mp2kp(circleVertex.point());
			
			if (T_BROKEN_GEOMETRY && (stepCount & 0xF) == 0xF) {
				using std::min;
				distToTgt = min(distToTgt, sqd(cv, q));
			}
			
			//reset p to cv, since then we don't have to explicity check if q is inside the active triangle in case s is collinear with p->q
			//this has the overhead of going back to the former triangle but simplifies the logic
			//This is also needed for bfs
			p = cv;
			
			if (cv == q) {
				auto resultFaceId = returnFaceFromVertex(circleVertex);
				SSERIALIZE_NORMAL_ASSERT(face(resultFaceId).contains(target));
				return resultFaceId;
			}
			//p->q goes through circleVertex, we have to find the right triangle
			//That triangle has circleVertex as a vertex
			FaceCirculator fcBegin(circleVertex.facesBegin()), fcEnd(circleVertex.facesEnd());
			while (true) {
				const Face & cf = fcBegin.face();
				SSERIALIZE_CHEAP_ASSERT(cf.valid());
				int cvIdx = cf.index(circleVertex);
				SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(cvIdx, -1);
				
				Vertex myLv(cf.vertex((uint32_t)Triangulation::cw(cvIdx))), myRv(cf.vertex((uint32_t)Triangulation::ccw(cvIdx)));
				Point_2 myLP(mp2kp(myLv.point())), myRP(mp2kp(myRv.point()));
				
				CGAL::Orientation lvOt = ot(p, q, myLP);
				CGAL::Orientation rvOt = ot(p, q, myRP);
				
				//we've found the right triangle
				if (lvOt == CGAL::Orientation::LEFT_TURN && rvOt == CGAL::Orientation::RIGHT_TURN) {
					//check if the point is within this triangle
					//we do this by checking if it is to the right of the line l->r or on it
					CGAL::Orientation lrqO = ot(myLP, myRP, q);
					if (lrqO == CGAL::Orientation::RIGHT_TURN || lrqO == CGAL::COLLINEAR) {
						SSERIALIZE_NORMAL_ASSERT(cf.contains(target));
						return cf.id();
					}
					else {
						rv = myRv;
						lv = myLv;
						rp = myRP;
						lp = myLP;
						circleVertex = Vertex();
						//the next face is the face that shares the edge myLv<->myRv with cf
						//check if it's inside our triangulation 
						if (!cf.isNeighbor((uint32_t)cvIdx)) { //next face is outside (this is only valid for convex triangulations)
							return NullFace;
						}
						curFace = cf.neighbor((uint32_t)cvIdx);
						visitor(curFace);
						break;
					}
				}
				else if (lvOt == CGAL::Orientation::COLLINEAR) {
					if (oal(p, q, myLP)) { //q lies on the edge, this has to come first in case both p and q lie on vertices
						SSERIALIZE_NORMAL_ASSERT(cf.contains(target));
						return cf.id();
					}
					else if (oal(p, myLP, q)) {
						circleVertex = myLv;
						break;
					}
				}
				else if (rvOt == CGAL::Orientation::COLLINEAR) {
					if (oal(p, q, myRP)) { //q lies on the edge, this has to come first in case both p and q lie on vertices
						SSERIALIZE_NORMAL_ASSERT(cf.contains(target));
						return cf.id();
					}
					else if (oal(p, myRP, q)) {
						circleVertex = myRv;
						break;
					}
				}
				//we've tested all faces and none matched, thus p must be outside of our triangulation
				//This is only correct, if the triangulation is convex
				if (fcBegin == fcEnd) {
					return NullFace;
				}
				else {
					++fcBegin;
				}
			}
			//next iteration
			continue;
		}
		else {
			SSERIALIZE_CHEAP_ASSERT(curFace.valid());
			
			if (T_BROKEN_GEOMETRY) {
				//we should never visit the same triangle twice, something must be wrong
				if (((m_features & F_DEGENERATE_FACES) && curFace.isDegenerate()) ||
						lastTriangs.count(curFace.id()))
				{
					circleVertex = bfs(curFace);
					curFace = Face();
					continue;
				}
				else if ((stepCount & 0xF) == 0xF) {
					using std::min;
					distToTgt = min(distToTgt, sqd(mp2kp(curFace.centroid()), q));
				}
			}
		
			//we have a face, r, l, rv and lv are set, find s
			//p->q does not pass through r or l but may pass through s
			//p->q intersects l->r with l beeing on the left and r beeing on the right
			Vertex sv;
			int lvIndex = curFace.index(lv);
			int rvIndex;
			if (curFace.vertexId((uint32_t)Triangulation::cw(lvIndex)) != rv.id()) {
				sv = curFace.vertex((uint32_t)cw(lvIndex));
				rvIndex = ccw(lvIndex);
			}
			else {
				sv = curFace.vertex((uint32_t)ccw(lvIndex));
				rvIndex = cw(lvIndex);
			}
			SSERIALIZE_CHEAP_ASSERT_EQUAL(curFace.vertexId((uint32_t)rvIndex), rv.id());
			SSERIALIZE_CHEAP_ASSERT_EQUAL(curFace.vertexId((uint32_t)lvIndex), lv.id());
			
			Point_2 sp(mp2kp(sv.point()));
			CGAL::Orientation sot = ot(p, q, sp);
			if (CGAL::Orientation::COLLINEAR == sot) {
				//top takes care of q beeing in the current triangle
				circleVertex = sv;
			}
			else if (CGAL::Orientation::LEFT_TURN == sot) {
				//check if q is within our face
				//this is the case if q is to the right of l->s 
				CGAL::Orientation rsqO = ot(rp, sp, q);
				if (rsqO == CGAL::Orientation::LEFT_TURN || rsqO == CGAL::Orientation::COLLINEAR) {
					SSERIALIZE_NORMAL_ASSERT(curFace.contains(target));
					return curFace.id();
				}
				if (!curFace.isNeighbor((uint32_t)lvIndex)) {
					return NullFace;
				}
				lv = sv;
				lp = sp;
				curFace = curFace.neighbor((uint32_t)lvIndex);
				visitor(curFace);
			}
			else if (CGAL::Orientation::RIGHT_TURN == sot) {
				//check if q is within our face
				//this is the case if q is to the left of r->s
				CGAL::Orientation lsqO = ot(lp, sp, q);
				if (lsqO == CGAL::Orientation::RIGHT_TURN || lsqO == CGAL::Orientation::COLLINEAR) {
					SSERIALIZE_NORMAL_ASSERT(curFace.contains(target));
					return curFace.id();
				}
				if (!curFace.isNeighbor((uint32_t)rvIndex)) {
					return NullFace;
				}
				rv = sv;
				rp = sp;
				curFace = curFace.neighbor((uint32_t)rvIndex);
				visitor(curFace);
			}
			else {
				throw std::runtime_error("sserialize::Static::Triangulation::locate: unexpected error");
			}
		}
	}
	
	return NullFace;
}

template<typename TVisitor>
Triangulation::FaceId Triangulation::traverse_straight_imp(const Point & target, const Point & source, const Face & startFace, TVisitor visitor) const {
	if (!startFace.contains(source)) {
		throw std::invalid_argument("sserialize::Triangulation::traverse_straight: startFace needs to contain source");
	}
	if (m_features & (F_BROKEN_GEOMETRY | F_DEGENERATE_FACES)) {
		throw sserialize::UnimplementedFunctionException("sserialize::Triangulation::traverse_straight: no support for broken geometry yet");
	}
	
	struct State {
		//VERTEX means that the vertex with id vertexId is collinear to source->target
		enum class Type {INVALID, VERTEX, FACE};
		VertexId vertexId;
		FaceId faceId;
		Type type;
		State() : vertexId(Triangulation::NullVertex), faceId(Triangulation::NullFace), type(Type::INVALID) {}
	};
	
	using OrientationTest = detail::Triangulation::Orientation<Point>;
	OrientationTest ot;
	//we now walk from face to face
	//We can exit a face either through an edge or through a Vertex
	

	//This function either returns the face through which the LINE source->target leaves v
	//OR it returns another vertex that is also on the LINE source->target
	auto vertexStep = [&ot, &source, &target](Vertex const & v) -> State {
		assert(v.valid());
		State result;
		Triangulation::FaceCirculator fcBegin(v.facesBegin()), fcEnd(v.facesEnd());
		while (true) {
			Face const & f = fcBegin.face();
			assert(!f.isDegenerate());
			int vp = f.index(v);
			assert(vp != -1);
			auto otcw = ot(source, target, f.point(Triangulation::cw(vp)));
			auto otccw = ot(source, target, f.point(Triangulation::ccw(vp)));
			
			if (otcw == CGAL::LEFT_TURN && otccw == CGAL::RIGHT_TURN) {
				result.faceId = f.id();
				result.type = State::Type::FACE;
				return result;
			}
			else if(otcw == CGAL::COLLINEAR && otccw == CGAL::RIGHT_TURN) {
				result.vertexId = f.vertexId(Triangulation::cw(vp));
				result.faceId = f.id();
				result.type = State::Type::VERTEX;
				return result;
			}
			else if (otcw == CGAL::LEFT_TURN && otccw == CGAL::COLLINEAR) {
				result.vertexId = f.vertexId(Triangulation::ccw(vp));
				result.faceId = f.id();
				result.type = State::Type::VERTEX;
				return result;
			}
			if (fcBegin == fcEnd) {
				return State();
			}
			else {
				++fcBegin;
			}
		}
	};
	
	//This function either returns the edge through which the line source->target passes
	//or a vertex of f that is collinear to source->target in the direction of target
	auto faceStep = [&ot, &source, &target](Face const & f) -> State {
		State state;
		assert(!f.contains(target));
		std::array<CGAL::Sign, 3> ots = {{
			ot(source, target, f.point(0)),
			ot(source, target, f.point(1)),
			ot(source, target, f.point(2))
		}};
		//now check for each edge if source->target leaves through this edge (or is collinear to a supporting vertex)
		//edge i is defined by the vertex i opposite to it, hence edge i is defined by the vertices cw(i) and ccw(i)
		//if source->target passes through edge i, then cw(i) is to the left and ccw(i) to the right
		for(int i(0); i < 3; ++i) {
			if (ots.at(cw(i)) == CGAL::LEFT_TURN && ots.at(ccw(i)) == CGAL::RIGHT_TURN) { //this is the one
				state.faceId = f.neighborId(i);
				state.type = State::Type::FACE;
				return state;
			}
			else if (ots.at(cw(i)) == CGAL::COLLINEAR && ots.at(ccw(i)) == CGAL::RIGHT_TURN) { //we pass through the left edge vertex
				state.vertexId = f.vertexId(cw(i));
				state.faceId = f.id();
				state.type = State::Type::VERTEX;
				return state;
			}
			else if (ots.at(cw(i)) == CGAL::LEFT_TURN && ots.at(ccw(i)) == CGAL::COLLINEAR) { //we pass through the right edge vertex
				state.vertexId = f.vertexId(ccw(i));
				state.faceId = f.id();
				state.type = State::Type::VERTEX;
				return state;
			}
		}
		//this should never happen, it essentially means that source->target does not intersect f
		throw sserialize::InvalidAlgorithmStateException("sserialize::Static::Triangulation::traverse_straight");
		return state;
	};
	
	State state;
	state.faceId = startFace.id();
	state.type = State::Type::FACE;
	
	Face currentFace = face(state.faceId);
	
	visitor(currentFace);
	while (!currentFace.contains(target)) {
		
		if (state.type == State::Type::FACE) {
			state = faceStep(currentFace);
		}
		else if (state.type == State::Type::VERTEX) {
			state = vertexStep(vertex(state.vertexId));
		}
		else {
			assert(state.type == State::Type::INVALID);
			return NullFace;
		}
		
		if (state.faceId == NullFace) {
			return NullFace;
		}
		currentFace = face(state.faceId);
		visitor(currentFace);
	}
	SSERIALIZE_NORMAL_ASSERT(currentFace.contains(target));
	return currentFace.id();
}

template<typename T_EXPLORER>
void Triangulation::explore(FaceId startFace, T_EXPLORER explorer) const {
	std::unordered_set<FaceId> visitedFaces;
	std::unordered_set<FaceId> pendingFaces;
	pendingFaces.insert(startFace);
	
	while (pendingFaces.size()) {
		FaceId cfId;
		{
			auto tmp(pendingFaces.begin());
			cfId = *tmp;
			pendingFaces.erase(tmp);
		}
		Face cf(face(cfId));
		if (explorer(cf)) {
			for(uint32_t j(0); j < 3; ++j) {
				if (cf.isNeighbor(j)) {
					auto nId = cf.neighborId(j);
					if (!visitedFaces.count(nId)) {
						pendingFaces.insert(nId);
						//nId is going to be visited in the future
						visitedFaces.insert(nId);
					}
				}
			}
		}
	}
}

template<typename T_CTD, typename T_REMOVED_EDGES>
std::size_t
Triangulation::prepare(T_CTD& ctd, T_REMOVED_EDGES re, GeometryCleanType gct, double minEdgeLength) {
	if (gct == GCT_REMOVE_DEGENERATE_FACES) {
		return detail::Triangulation::remove_degenerate_faces(ctd);
	}
	else if (gct == GCT_SNAP_VERTICES) {
		return detail::Triangulation::snap_vertices(ctd, re, minEdgeLength);
	}
	else {
		return 0;
	}
}

template<typename T_CGAL_TRIANGULATION_DATA_STRUCTURE, typename T_VERTEX_TO_VERTEX_ID_MAP, typename T_FACE_TO_FACE_ID_MAP>
sserialize::UByteArrayAdapter &
Triangulation::append(T_CGAL_TRIANGULATION_DATA_STRUCTURE& src, T_FACE_TO_FACE_ID_MAP& faceToFaceId, T_VERTEX_TO_VERTEX_ID_MAP& vertexToVertexId, sserialize::UByteArrayAdapter& dest, sserialize::Static::spatial::Triangulation::GeometryCleanType gct) {
	typedef T_CGAL_TRIANGULATION_DATA_STRUCTURE TDS;
	typedef typename TDS::Face_handle Face_handle;
	typedef typename TDS::Vertex_handle Vertex_handle;
	typedef typename TDS::Finite_faces_iterator Finite_faces_iterator;
	typedef typename TDS::Finite_vertices_iterator Finite_vertices_iterator;
	typedef typename TDS::Face_circulator Face_circulator;
	typedef typename TDS::Point Point;
	
	if (src.number_of_vertices() >= VertexId::npos || src.number_of_faces() >= FaceId::npos) {
		throw sserialize::CreationException("sserialize::Static::spatial::Triangulation::append: source has too many vertices or faces");
	}
	
	detail::Triangulation::Convert<Point, Triangulation::Point> kp2mp;
	
	faceToFaceId.clear();
	vertexToVertexId.clear();
	
	FaceId faceId{0};
	VertexId vertexId{0};
	
	SizeType faceCount = 0;
	sserialize::Size vertexCount = 0;
	
	SizeType degenerateFaceCount = 0;
	//the reduced precision of the serialization may produce degenerate triangultions
	//or even triangulations that are incorrect (like self intersections due to rounding errors etc.)
	//we would need to snap the vertices accordingly, unfortunately this not easy
	//as a remedy, mark faces that are degenerate
	//Alternative?: remove vertices that change their precision in such a way that they snap on to another vertex
	
	for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
		faceToFaceId[fh] = faceId;
		++faceId;
	}
	faceCount = faceId.ut();
	
	uint8_t features = 0;
	if ((gct & (GCT_REMOVE_DEGENERATE_FACES | GCT_SNAP_VERTICES)) == 0) {
		features |= F_DEGENERATE_FACES;
	}
	for(Finite_vertices_iterator vIt(src.finite_vertices_begin()), vEnd(src.finite_vertices_end()); vIt != vEnd; ++vIt) {
		if (detail::Triangulation::IntPoint<Point>::changes(vIt->point())) {
			features |= F_BROKEN_GEOMETRY;
			break;
		}
	}
	if ((gct & GCT_SNAP_VERTICES) && (features & F_BROKEN_GEOMETRY)) {
		throw sserialize::CreationException("sserialize::Static::spatial::Triangulation: triangulation has broken geometry");
	}
	
	dest.putUint8(3);//VERSION
	dest.putUint8(features);
	{ //put the points
		Triangulation::Point gp;
		sserialize::Static::ArrayCreator<sserialize::spatial::GeoPoint> va(dest);
		va.reserveOffsets(src.number_of_vertices());
		for(Finite_vertices_iterator vt(src.finite_vertices_begin()), vtEnd(src.finite_vertices_end()); vt != vtEnd; ++vt) {
			gp = kp2mp(vt->point());
			va.put(gp);
			vertexToVertexId[vt] = vertexId;
			++vertexId;
		}
		va.flush();
		vertexCount = vertexId.ut();
	}
	{//put the vertex info
		std::vector<uint8_t> bitConfig(Triangulation::Vertex::VI__NUMBER_OF_ENTRIES);
		bitConfig[Triangulation::Vertex::VI_FACES_BEGIN] = (uint8_t) sserialize::CompactUintArray::minStorageBits(faceCount);
		bitConfig[Triangulation::Vertex::VI_FACES_END] = bitConfig[Triangulation::Vertex::VI_FACES_BEGIN];
		sserialize::MultiVarBitArrayCreator va(bitConfig, dest);
		for(Finite_vertices_iterator vt(src.finite_vertices_begin()), vtEnd(src.finite_vertices_end()); vt != vtEnd; ++vt) {
			SSERIALIZE_NORMAL_ASSERT(vertexToVertexId.is_defined(vt));
			VertexId vertexId = vertexToVertexId[vt];
			FaceId beginFace, endFace;
			Face_circulator fc(src.incident_faces(vt));
			{
				SizeType numFinite(0), numInfinite(0);
				Face_circulator fcIt(fc);
				++fcIt;
				for(;fcIt != fc; ++fcIt) {
					if (src.is_infinite(fcIt)) {
						++numInfinite;
					}
					else {
						++numFinite;
					}
				}
				if (src.is_infinite(fc)) {
					++numInfinite;
				}
				else {
					++numFinite;
				}
			}
			for(;src.is_infinite(fc); ++fc) {}
			SSERIALIZE_NORMAL_ASSERT(!src.is_infinite(fc));
			//now move forward/backward until we either reach the fc or reach an infite face
			Face_circulator fcBegin(fc), fcEnd(fc);
			while(true) {
				--fcBegin;
				if (fcBegin == fc) { //we came around once
					break;
				}
				if (src.is_infinite(fcBegin)) {
					++fcBegin;
					break;
				}
				
			}
			while(true) {
				++fcEnd;
				if (fcEnd == fc) { //we came around once, prev is the last valid face
					--fcEnd;
					break;
				}
				if (src.is_infinite(fcEnd)) {
					--fcEnd;
					break;
				}
			}
			
			SSERIALIZE_NORMAL_ASSERT(!src.is_infinite(fcBegin));
			SSERIALIZE_NORMAL_ASSERT(!src.is_infinite(fcEnd));
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fcBegin));
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fcEnd));
			
			beginFace = faceToFaceId[fcBegin];
			endFace = faceToFaceId[fcEnd];
			
			va.set(vertexId.ut(), Triangulation::Vertex::VI_FACES_BEGIN, beginFace.ut());
			va.set(vertexId.ut(), Triangulation::Vertex::VI_FACES_END, endFace.ut());
		}
		va.flush();
	}
	{
		std::vector<uint8_t> bitConfig(Triangulation::Face::FI__NUMBER_OF_ENTRIES);
		bitConfig[Triangulation::Face::FI_NEIGHBOR_VALID] = 3;
		bitConfig[Triangulation::Face::FI_NEIGHBOR0] = (uint8_t)sserialize::CompactUintArray::minStorageBits(faceCount);
		bitConfig[Triangulation::Face::FI_NEIGHBOR1] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_NEIGHBOR2] = bitConfig[Triangulation::Face::FI_NEIGHBOR0];
		bitConfig[Triangulation::Face::FI_VERTEX0] = (uint8_t)sserialize::CompactUintArray::minStorageBits(vertexCount);
		bitConfig[Triangulation::Face::FI_VERTEX1] = bitConfig[Triangulation::Face::FI_VERTEX0];
		bitConfig[Triangulation::Face::FI_VERTEX2] = bitConfig[Triangulation::Face::FI_VERTEX0];
		bitConfig[Triangulation::Face::FI_IS_DEGENERATE] = 1;
		sserialize::MultiVarBitArrayCreator fa(bitConfig, dest);
	
		faceId = FaceId(0);
		for(Finite_faces_iterator fh(src.finite_faces_begin()), fhEnd(src.finite_faces_end()); fh != fhEnd; ++fh) {
			SSERIALIZE_NORMAL_ASSERT(faceToFaceId.is_defined(fh) && faceToFaceId[fh] == faceId);
			{//check degeneracy
				auto p0 = fh->vertex(0)->point();
				auto p1 = fh->vertex(1)->point();
				auto p2 = fh->vertex(2)->point();
				if (p0 == p1 || p0 == p2 || p1 == p2) { //this should not happen
					throw sserialize::CreationException("Triangulation has degenerate face in source triangulation");
				}
				detail::Triangulation::IntPoint<Point> ip0(p0), ip1(p1), ip2(p2);
				if (ip0 == ip1 || ip0 == ip2 || ip1 == ip2) {
					if ( !(features & F_DEGENERATE_FACES) ) {
						throw sserialize::CreationException("Triangulation has degenerate face after serialization");
					}
					fa.set(faceId.ut(), Triangulation::Face::FI_IS_DEGENERATE, 1);
					++degenerateFaceCount;
				}
			}
			
			uint8_t validNeighbors = 0;
			for(int j(0); j < 3; ++j) {
				Face_handle nfh = fh->neighbor(j);
				auto nfhId = NullFace;
				if (faceToFaceId.is_defined(nfh)) {
					nfhId = faceToFaceId[nfh];
					validNeighbors |= static_cast<uint8_t>(1) << j;
					fa.set(faceId.ut(), Triangulation::Face::FI_NEIGHBOR_BEGIN+(uint32_t)j, nfhId.ut());
				}
			}
			fa.set(faceId.ut(), Triangulation::Face::FI_NEIGHBOR_VALID, validNeighbors);
			
			for(int j(0); j < 3; ++j) {
				Vertex_handle vh = fh->vertex(j);
				SSERIALIZE_NORMAL_ASSERT(vertexToVertexId.is_defined(vh));
				auto vertexId = vertexToVertexId[vh];
				fa.set(faceId.ut(), Triangulation::Face::FI_VERTEX_BEGIN+(uint32_t)j, vertexId.ut());
			}
			++faceId;
		}
		fa.flush();
		SSERIALIZE_NORMAL_ASSERT_EQUAL(faceId.ut(),faceCount);
	}
	if (degenerateFaceCount) {
		std::cout << "Triangulation has " << degenerateFaceCount << " degenerate faces!" << std::endl;
	}
	return dest;
}

} //end namespace sserialize::Static::spatial

#endif
