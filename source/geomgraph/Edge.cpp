/**********************************************************************
 * $Id$
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 * Copyright (C) 2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/geomgraph.h>

//#define DEBUG_INTERSECT 0
#ifndef DEBUG
#define DEBUG 0
#endif

namespace geos {

/**
 * Updates an IM from the label for an edge.
 * Handles edges from both L and A geometrys.
 */
void
Edge::updateIM(Label *lbl, IntersectionMatrix *im) 
{
	im->setAtLeastIfValid(lbl->getLocation(0,Position::ON),lbl->getLocation(1,Position::ON),1);
	if (lbl->isArea()) {
		im->setAtLeastIfValid(lbl->getLocation(0,Position::LEFT),lbl->getLocation(1,Position::LEFT),2);
		im->setAtLeastIfValid(lbl->getLocation(0,Position::RIGHT),lbl->getLocation(1,Position::RIGHT),2);
	}
}

#if 0
Edge::Edge()
{
	//cerr<<"["<<this<<"] Edge()"<<endl;
	eiList=NULL;
	isIsolatedVar=true;
	depth=NULL;
	depthDelta=0;
	mce=NULL;
	pts=NULL;
	env=NULL;
}
#endif

Edge::~Edge()
{
	//cerr<<"["<<this<<"] ~Edge()"<<endl;
	delete mce;
	delete pts;
	delete env;
}

Edge::Edge(CoordinateSequence* newPts, Label *newLabel):
	GraphComponent(newLabel),
	pts(newPts),
	eiList(this),
	mce(NULL),
	env(NULL),
	isIsolatedVar(true),
	depth(),
	depthDelta(0)
{
}

Edge::Edge(CoordinateSequence* newPts):
	GraphComponent(),
	pts(newPts),
	eiList(this),
	mce(NULL),
	env(NULL),
	isIsolatedVar(true),
	//depth(new Depth()),
	depth(),
	depthDelta(0)
{
}

int
Edge::getNumPoints() const
{
	return pts->getSize();
}

void
Edge::setName(const string &newName)
{
	name=newName;
}

const CoordinateSequence*
Edge::getCoordinates() const
{
	return pts;
}

const Coordinate&
Edge::getCoordinate(int i) const
{
	return pts->getAt(i);
}

const Coordinate&
Edge::getCoordinate() const
{
	return pts->getAt(0);
}

Depth &
Edge::getDepth()
{
	return depth;
}

/*
 * The depthDelta is the change in depth as an edge is crossed from R to L
 * @return the change in depth as the edge is crossed from R to L
 */
int
Edge::getDepthDelta() const
{
	return depthDelta;
}

void
Edge::setDepthDelta(int newDepthDelta)
{
	depthDelta=newDepthDelta;
}

int
Edge::getMaximumSegmentIndex() const
{
	return getNumPoints()-1;
}

EdgeIntersectionList&
Edge::getEdgeIntersectionList()
{
	return eiList;
}

MonotoneChainEdge*
Edge::getMonotoneChainEdge()
{
	if (mce==NULL) mce=new MonotoneChainEdge(this);
	return mce;
}

bool
Edge::isClosed() const
{
	return pts->getAt(0)==pts->getAt(getNumPoints()-1);
}

/*
 * An Edge is collapsed if it is an Area edge and it consists of
 * two segments which are equal and opposite (eg a zero-width V).
 */
bool
Edge::isCollapsed() const
{
	if (!label->isArea()) return false;
	if (getNumPoints()!= 3) return false;
	if (pts->getAt(0)==pts->getAt(2) ) return true;
	return false;
}

Edge*
Edge::getCollapsedEdge()
{
	CoordinateSequence *newPts = new CoordinateArraySequence(2);
	newPts->setAt(pts->getAt(0),0);
	newPts->setAt(pts->getAt(1),1);
	return new Edge(newPts, Label::toLineLabel(*label));
}

/*
 * Adds EdgeIntersections for one or both
 * intersections found for a segment of an edge to the edge intersection list.
 */
void
Edge::addIntersections(LineIntersector *li, int segmentIndex, int geomIndex)
{
#if DEBUG
	cerr<<"["<<this<<"] Edge::addIntersections("<<li->toString()<<", "<<segmentIndex<<", "<<geomIndex<<") called"<<endl;
#endif
	for (int i=0; i<li->getIntersectionNum();i++) {
		addIntersection(li,segmentIndex,geomIndex,i);
	}
}

/*
 * Add an EdgeIntersection for intersection intIndex.
 * An intersection that falls exactly on a vertex of the edge is normalized
 * to use the higher of the two possible segmentIndexes
 */
void
Edge::addIntersection(LineIntersector *li,
	int segmentIndex, int geomIndex, int intIndex)
{
#if DEBUG
	cerr<<"["<<this<<"] Edge::addIntersection("<<li->toString()<<", "<<segmentIndex<<", "<<geomIndex<<", "<<intIndex<<") called"<<endl;
#endif
	const Coordinate& intPt=li->getIntersection(intIndex);
	unsigned int normalizedSegmentIndex=segmentIndex;
	double dist=li->getEdgeDistance(geomIndex,intIndex);

	// normalize the intersection point location
	unsigned int nextSegIndex=normalizedSegmentIndex+1;
	unsigned int npts=getNumPoints();
	if (nextSegIndex<npts)
	{
		const Coordinate& nextPt=pts->getAt(nextSegIndex);
        // Normalize segment index if intPt falls on vertex
        // The check for point equality is 2D only - Z values are ignored
		if (intPt.equals2D(nextPt)) {
			normalizedSegmentIndex=nextSegIndex;
			dist=0.0;
		}
	}

	/*
	 * Add the intersection point to edge intersection list.
	 */
#if DEBUG
	cerr<<"Edge::addIntersection adding to edge intersection list point "<<intPt.toString()<<endl;
#endif
	eiList.add(intPt,normalizedSegmentIndex,dist);
}

/*
 * Update the IM with the contribution for this component.
 * A component only contributes if it has a labelling for both parent geometries
 */
void
Edge::computeIM(IntersectionMatrix *im) 
{
	updateIM(label, im);
}

/*
 * equals is defined to be:
 * 
 * e1 equals e2
 * <b>iff</b>
 * the coordinates of e1 are the same or the reverse of the coordinates in e2
 */
bool
operator==(const Edge &e1, const Edge &e2)
{
	return e1.equals(&e2);
}

/*
 * equals is defined to be:
 * <p>
 * e1 equals e2
 * <b>iff</b>
 * the coordinates of e1 are the same or the reverse of the coordinates in e2
 */
bool
Edge::equals(const Edge *e) const
{
	unsigned int npts1=getNumPoints(); 
	unsigned int npts2=e->getNumPoints(); 

	if (npts1 != npts2 ) return false;

	bool isEqualForward=true;
	bool isEqualReverse=true;

	for (unsigned int i=0, iRev=npts1-1; i<npts1; ++i, --iRev)
	{
		const Coordinate &e1pi=pts->getAt(i);
		const Coordinate &e2pi=e->pts->getAt(i);
		const Coordinate &e2piRev=e->pts->getAt(iRev);

		if ( !e1pi.equals2D(e2pi) ) isEqualForward=false;
		if ( !e1pi.equals2D(e2piRev) ) isEqualReverse=false;
		if ( !isEqualForward && !isEqualReverse ) return false;
	}
	return true;
}

/*
 * @return true if the coordinate sequences of the Edges are identical
 */
bool
Edge::isPointwiseEqual(const Edge *e) const
{
#if DEBUG > 2
	cerr<<"Edge::isPointwiseEqual call"<<endl;
#endif
	unsigned int npts=getNumPoints();
	unsigned int enpts=e->getNumPoints();
	if (npts!=enpts) return false;
#if DEBUG
	cerr<<"Edge::isPointwiseEqual scanning "<<e->npts()<<"x"<<npts<<" points"<<endl;
#endif
	for (unsigned int i=0; i<npts; ++i)
	{
		if (!pts->getAt(i).equals2D(e->pts->getAt(i))) {
			return false;
		}
	}
	return true;
}

string
Edge::print() const
{
	string out="edge " + name + ": ";
	out+="LINESTRING (";
	unsigned int npts=getNumPoints();
	for (unsigned int i=0; i<npts; ++i)
	{
		if (i>0) out+=",";
		out+=pts->getAt(i).toString();
	}
	out+=")  ";
	out+=label->toString();
	out+=" ";
	out+=depthDelta;
	return out;
}
  
string
Edge::printReverse() const
{
	string out="edge " + name + ": ";
	unsigned int npts=getNumPoints();
	for (unsigned int i=npts; i>0; --i)
	{
		out+=pts->getAt(i-1).toString() + " ";
	}
	out+="\n";
	return out;
}

Envelope*
Edge::getEnvelope()
{
	// compute envelope lazily
	if (env==NULL)
	{
		env=new Envelope();
		unsigned int npts=getNumPoints();
		for (unsigned int i = 0; i<npts; ++i)
		{
			env->expandToInclude(pts->getAt(i));
		}
	}
	return env;
}

} // namespace geos

/**********************************************************************
 * $Log$
 * Revision 1.23  2006/01/31 19:07:34  strk
 * - Renamed DefaultCoordinateSequence to CoordinateArraySequence.
 * - Moved GetNumGeometries() and GetGeometryN() interfaces
 *   from GeometryCollection to Geometry class.
 * - Added getAt(int pos, Coordinate &to) funtion to CoordinateSequence class.
 * - Reworked automake scripts to produce a static lib for each subdir and
 *   then link all subsystem's libs togheter
 * - Moved C-API in it's own top-level dir capi/
 * - Moved source/bigtest and source/test to tests/bigtest and test/xmltester
 * - Fixed PointLocator handling of LinearRings
 * - Changed CoordinateArrayFilter to reduce memory copies
 * - Changed UniqueCoordinateArrayFilter to reduce memory copies
 * - Added CGAlgorithms::isPointInRing() version working with
 *   Coordinate::ConstVect type (faster!)
 * - Ported JTS-1.7 version of ConvexHull with big attention to
 *   memory usage optimizations.
 * - Improved XMLTester output and user interface
 * - geos::geom::util namespace used for geom/util stuff
 * - Improved memory use in geos::geom::util::PolygonExtractor
 * - New ShortCircuitedGeometryVisitor class
 * - New operation/predicate package
 *
 * Revision 1.22  2005/12/07 20:49:23  strk
 * Oops, removed Coordinate copies introduced by recent code cleanups
 *
 * Revision 1.21  2005/11/29 14:39:46  strk
 * Removed number of points cache in Edge, replaced with local caches.
 *
 * Revision 1.20  2005/11/25 11:30:38  strk
 * Fix in ::equals() - this finally passes testLeaksBig.xml tests
 *
 * Revision 1.19  2005/11/24 23:43:13  strk
 * Yes another fix, sorry. Missing const-correctness.
 *
 * Revision 1.18  2005/11/24 23:24:38  strk
 * Fixed equals() function [ optimized in previous commit, but unchecked ]
 *
 * Revision 1.17  2005/11/24 23:09:15  strk
 * CoordinateSequence indexes switched from int to the more
 * the correct unsigned int. Optimizations here and there
 * to avoid calling getSize() in loops.
 * Update of all callers is not complete yet.
 *
 * Revision 1.16  2005/11/16 15:49:54  strk
 * Reduced gratuitous heap allocations.
 *
 * Revision 1.15  2005/11/14 18:14:04  strk
 * Reduced heap allocations made by TopologyLocation and Label objects.
 * Enforced const-correctness on GraphComponent.
 * Cleanups.
 *
 * Revision 1.14  2005/11/07 12:31:24  strk
 * Changed EdgeIntersectionList to use a set<> rathern then a vector<>, and
 * to avoid dynamic allocation of initial header.
 * Inlined short SweepLineEvent methods.
 *
 * Revision 1.13  2005/07/11 10:27:47  strk
 * Cleaned up signed/unsigned mismatches
 *
 * Revision 1.12  2005/02/22 16:24:17  strk
 * cached number of points in Edge
 *
 * Revision 1.11  2005/02/22 10:55:41  strk
 * Optimized Edge::equals(Edge *e)
 *
 * Revision 1.10  2004/12/08 13:54:43  strk
 * gcc warnings checked and fixed, general cleanups.
 *
 * Revision 1.9  2004/11/23 19:53:06  strk
 * Had LineIntersector compute Z by interpolation.
 *
 * Revision 1.8  2004/11/22 12:59:25  strk
 * Added debugging lines
 *
 * Revision 1.7  2004/11/01 16:43:04  strk
 * Added Profiler code.
 * Temporarly patched a bug in DoubleBits (must check drawbacks).
 * Various cleanups and speedups.
 *
 * Revision 1.6  2004/10/20 17:32:14  strk
 * Initial approach to 2.5d intersection()
 *
 * Revision 1.5  2004/07/21 09:55:24  strk
 * CoordinateSequence::atLeastNCoordinatesOrNothing definition fix.
 * Documentation fixes.
 *
 * Revision 1.4  2004/07/08 19:34:49  strk
 * Mirrored JTS interface of CoordinateSequence, factory and
 * default implementations.
 * Added CoordinateArraySequenceFactory::instance() function.
 *
 * Revision 1.3  2004/07/02 13:28:26  strk
 * Fixed all #include lines to reflect headers layout change.
 * Added client application build tips in README.
 *
 * Revision 1.2  2004/06/16 13:13:25  strk
 * Changed interface of SegmentString, now copying CoordinateSequence argument.
 * Fixed memory leaks associated with this and MultiGeometry constructors.
 * Other associated fixes.
 *
 * Revision 1.1  2004/03/19 09:48:45  ybychkov
 * "geomgraph" and "geomgraph/indexl" upgraded to JTS 1.4
 *
 * Revision 1.14  2003/11/07 01:23:42  pramsey
 * Add standard CVS headers licence notices and copyrights to all cpp and h
 * files.
 *
 * Revision 1.13  2003/10/15 16:39:03  strk
 * Made Edge::getCoordinates() return a 'const' value. Adapted code set.
 *
 **********************************************************************/
