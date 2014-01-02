#ifndef SSERIALIZE_SPATIAL_POLYGON_STORE_H
#define SSERIALIZE_SPATIAL_POLYGON_STORE_H
#include <vector>
#include <set>
#include <sserialize/spatial/RWGeoGrid.h>
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/utility/ProgressInfo.h>
#include <iostream>
#include <fstream>

//TODO: look into sweep line algo

namespace sserialize {
namespace spatial {

/** As of now, this only works for lat,lon > 0.0 **/
template<class TValue>
class GeoRegionStore {
public:
	typedef GeoPoint Point;
private:

	typedef std::vector<uint32_t> PolyRasterElement;
	/** This class holds the tile data of the PolyRaster. Before final deletion, deleteStorage() has to be called on the LAST copy! */
	struct RasterElementPolygons {
		RasterElementPolygons() : enclosing(0), colliding(0) {}
		virtual ~RasterElementPolygons() {}
		PolyRasterElement * enclosing; //Polygons that fully enclose this raster-element
		PolyRasterElement * colliding; //Polygons that collide with this raster element, but don't enclose it
		std::ostream & operator<<(std::ostream & out) const {
			PolyRasterElement::const_iterator end;
			out << "BEGIN-ENCLOSING" << std::endl;
			if (enclosing) {
				end = enclosing->end();
				for(PolyRasterElement::const_iterator it = enclosing->begin(); it != end; ++it) {
					out << *it << std::endl;
				}
			}
			out << "END-ENCLOSING" << std::endl;
			out << "BEGIN-COLLIDING" << std::endl;
			if (colliding) {
				end = colliding->end();
				for(PolyRasterElement::const_iterator it = colliding->begin(); it != end; ++it) {
					out << *it << std::endl;
				}
			}
			out << "END-COLLIDING" << std::endl;
			return out;
		}
		void deleteStorage() {
			delete enclosing;
			enclosing = 0;
			delete colliding;
			colliding = 0;
		}
	};

	class PolyRaster: private RWGeoGrid< RasterElementPolygons, std::vector<RasterElementPolygons> > {
		friend class GeoRegionStore;
		typedef RWGeoGrid< RasterElementPolygons, std::vector<RasterElementPolygons> > MyRWGeoGrid;
		typedef RGeoGrid<RasterElementPolygons, std::vector<RasterElementPolygons> > MyRGeoGrid;
	private:
		void createCellPoly(const GeoRect & cellRect, GeoPolygon & cellPoly) const {
			cellPoly.points().push_back( Point(cellRect.lat()[0], cellRect.lon()[0]) );
			cellPoly.points().push_back( Point(cellRect.lat()[1], cellRect.lon()[0]) );
			cellPoly.points().push_back( Point(cellRect.lat()[1], cellRect.lon()[1]) );
			cellPoly.points().push_back( Point(cellRect.lat()[0], cellRect.lon()[1]) );
			cellPoly.updateBoundaryRect();
		}
		void createCellPoly(GeoPolygon & cellPoly, uint32_t i , uint32_t j) {
			GeoRect cellRect(MyRWGeoGrid::cellBoundary(i,j));
			createCellPoly(cellRect, cellPoly);
		}
	public:
		PolyRaster(const GeoRect & bounds, uint32_t latcount, uint32_t loncount) :
		RWGeoGrid<RasterElementPolygons>(bounds, latcount, loncount)
		{
			MyRGeoGrid::storage().resize(latcount*loncount);
		}
		~PolyRaster() { 
			for(size_t i = 0; i < MyRGeoGrid::storage().size(); i++) {
				MyRGeoGrid::storage().at(i).deleteStorage();
			}
		}
		
		bool addRegion(GeoRegion & p, uint32_t polyId) {
			//First, get the bbox, then test all rasterelements for inclusion
			GeoRect b( p.boundary() );
			GeoGrid::GridBin blBin = GeoGrid::select(b.minLat(), b.minLon());
			GeoGrid::GridBin trBin = GeoGrid::select(b.maxLat(), b.maxLon());
			if (!blBin.valid() || !trBin.valid())
				return false;
			unsigned int xbinStart = blBin.x;
			unsigned int ybinStart = blBin.y;
			unsigned int xbinEnd = trBin.x;
			unsigned int ybinEnd = trBin.y;
			if (xbinStart == xbinEnd && ybinStart == ybinEnd) { //one cell
				if (!MyRWGeoGrid::binAt(xbinStart, ybinStart).colliding)
					MyRWGeoGrid::binAt(xbinStart, ybinStart).colliding = new PolyRasterElement;
				MyRWGeoGrid::binAt(xbinStart, ybinStart).colliding->push_back(polyId);
			}
			else { //polygon spans multiple cells
				std::vector< std::pair<uint32_t, uint32_t> > cells;
				cells.reserve((xbinEnd-xbinStart+1)*(ybinEnd-ybinStart+1));
				for(unsigned int i = xbinStart; i <= xbinEnd; i++) {
					for(unsigned int j = ybinStart; j <= ybinEnd; j++) {
						cells.push_back(std::pair<uint32_t, uint32_t>(i,j));
					}
				}
				size_t cellsSize = cells.size();
#pragma omp parallel for schedule(dynamic, 5)
				for(size_t k=0; k < cellsSize; k++) {
					unsigned int i = cells[k].first;
					unsigned int j = cells[k].second;
					GeoRect cellRect( MyRGeoGrid::cellBoundary(i, j) );
					//test enclosing
					if (p.contains( GeoPoint(cellRect.lat()[0], cellRect.lon()[0]) ) && p.contains( GeoPoint(cellRect.lat()[1], cellRect.lon()[1]) ) ) {
						if (!MyRWGeoGrid::binAt(i,j).enclosing) {
							MyRWGeoGrid::binAt(i,j).enclosing = new PolyRasterElement;
						}
						MyRWGeoGrid::binAt(i,j).enclosing->push_back(polyId);
					}
					else {
						GeoPolygon cellPoly;
						createCellPoly(cellRect, cellPoly);
						if (p.intersects(cellPoly)) {
							if (!MyRWGeoGrid::binAt(i,j).colliding) {
								MyRWGeoGrid::binAt(i,j).colliding = new PolyRasterElement;
							}
							MyRWGeoGrid::binAt(i,j).colliding->push_back(polyId);
						}
					}
				}
			}
			return true;
		}
		
		RasterElementPolygons checkedAt(const Point & p) const {
			return MyRGeoGrid::at(p.lat, p.lon);
		}

		void getPolygons(const Point & p, std::set<uint32_t> & definiteHits, std::set<uint32_t> & possibleHits) const {
			RasterElementPolygons polys = MyRGeoGrid::at(p.lat, p.lon);
			if (polys.enclosing)
				definiteHits.insert(polys.enclosing->begin(), polys.enclosing->end());

			if (polys.colliding)
				possibleHits.insert(polys.colliding->begin(), polys.colliding->end());

		}
		
		void getPolygons(const std::vector<Point> & p, std::set<uint32_t> & definiteHits, std::set<uint32_t> & possibleHits) const {
			if (!p.size())
				return std::set<uint32_t>();
			if (p.size() == 1)
				return getPolygons(p.front(), definiteHits, possibleHits);
			
			std::vector<Point>::const_iterator end( p.end());
			for(std::vector<Point>::const_iterator it = p.begin(); it != end; ++it)
				getPolygons(*it, definiteHits, possibleHits);
		}
		
		std::ostream & dump(std::ostream & out) const {
			out << "POLYRASTER" << std::endl;
			out << this->rect().lat()[0] << std::endl;
			out << this->rect().lon()[0] << std::endl;
			out << this->rect().lat()[1] << std::endl;
			out << this->rect().lon()[1] << std::endl;
			out << this->latCount() << std::endl;
			out << this->lonCount() << std::endl;
			for(size_t i = 0; i < this->storage().size(); i++) {
				out << "STARTBIN" << std::endl;
				this->storage().at(i).operator<<(out);
				out << "ENDBIN" << std::endl;
			}
			return out;
		}
	};

private:
	std::vector<GeoRegion*> m_regionStore;
	std::vector<TValue> m_values;
	PolyRaster * m_polyRaster;
	
private:
	inline bool pointInRegion(const Point & p, const GeoRegion & pg) { return pg.contains(p);}

	
public:
	GeoRegionStore(): m_polyRaster(0) {}
	~GeoRegionStore() {
		for(std::vector<GeoRegion*>::iterator it(m_regionStore.begin()), end(m_regionStore.end()); it != end; ++it) {
			delete *it;
		}
		delete m_polyRaster;
	}
	std::size_t size() const { return regions().size(); }
	const std::vector<GeoRegion*> & regions() const { return m_regionStore; }
	const std::vector<TValue> & values() const { return m_values; }
	inline void push_back(const GeoRegion & p, const TValue & value) {
		m_regionStore.push_back(static_cast<GeoRegion*>(p.copy()));
		m_values.push_back(value);
	}
	
	void addPolygonsToRaster(unsigned int gridLatCount, unsigned int gridLonCount) {
		if (!m_regionStore.size())
			return;
		if (m_polyRaster) {
			delete m_polyRaster;
			m_polyRaster = 0;
		}
		

		GeoRect bbox( m_regionStore.front()->boundary() );
		for(size_t i=0; i < m_regionStore.size(); i++) {
			bbox.enlarge( m_regionStore[i]->boundary() );
		}
		
		std::cout << "Creating PolyRaster with " << gridLatCount << "x" << gridLonCount << "bins. BBox: " << bbox << std::endl;
		m_polyRaster = new PolyRaster(bbox, gridLatCount, gridLonCount);
		sserialize::ProgressInfo info;
		info.begin(m_regionStore.size(), "Polyraster::addPolygon");
		for(size_t i=0; i < m_regionStore.size(); i++) {
			m_polyRaster->addRegion(*m_regionStore[i], i);
			info(i);
		}
		info.end("Polyraster::addPolygon");
		std::cout << std::endl;
	}
	
	inline std::set<uint32_t> test(double lat, double lon) {
		return test(Point(lat, lon));
	}
	
	std::set<uint32_t> test(const Point & p) {
		std::set<uint32_t> polys;
		if (!m_polyRaster) {
			for(std::size_t it(0), s(m_regionStore.size()); it < s; ++it) {
				if ( pointInRegion(p, *m_regionStore[it]) ) {
					polys.insert(it);
				}
			}
		}
		else {
			RasterElementPolygons rep( m_polyRaster->checkedAt(p) );
			if (rep.enclosing)
				polys.insert(rep.enclosing->begin(), rep.enclosing->end());
			if (rep.colliding) {
				for(PolyRasterElement::const_iterator it(rep.colliding->begin()), end( rep.colliding->end()); it != end; ++it) {
					if (pointInRegion(p, *m_regionStore[*it]) ) {
						polys.insert(*it);
					}
				}
			}
		}
		return polys;
	}
	
	template<typename TPOINT_ITERATOR>
	std::set<uint32_t> test(TPOINT_ITERATOR begin, TPOINT_ITERATOR end) {
		std::set<unsigned int> polyIds;
		if (!m_polyRaster) {
			for(std::size_t it = 0, itEnd(m_regionStore.size()); it < itEnd; ++it) {
				for(TPOINT_ITERATOR pit(begin); pit != end; ++pit) {
					if ( pointInRegion(*pit, *m_regionStore[it]) ) {
						polyIds.insert(it);
						break; // at least one point is within the currently tested polygon
					}
				}
			}
		}
		else {
			for(TPOINT_ITERATOR it(begin); it != end; ++it) {
				RasterElementPolygons rep( m_polyRaster->checkedAt(*it) );
				if (rep.enclosing)
					polyIds.insert(rep.enclosing->begin(), rep.enclosing->end());
				if (rep.colliding) {
					for(PolyRasterElement::const_iterator pit(rep.colliding->begin()), pend( rep.colliding->end()); pit != pend; ++pit) {
						if (!polyIds.count(*pit) && pointInRegion(*it, *m_regionStore[*pit]) ) {
							polyIds.insert(*pit);
						}
					}
				}
			}
		}
		return polyIds;
	}
	
	std::set<uint32_t> test(const std::vector<Point> & p) {
		return test(p.begin(), p.end());
	}
};

}}//end namespace

#endif