#ifdef OCCGEOMETRY

#include <mystdlib.h>
#include <occgeom.hpp>
#include <meshing.hpp>
#include "occtransfinite.hpp"

#include <BRepAdaptor_Curve.hxx>

#include <algorithm>
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <vector>


namespace netgen
{
	// A grid with a triangle sharper than this is left to the advancing front.
	static const double transfinite_min_angle = 5. * M_PI / 180.;

	// A grid with more than this many times the triangles the mesh size asks
	// for is left to the advancing front.
	static const double transfinite_max_excess = 2;


	struct TransfiniteSide
	{
		std::vector<int> pnums;
		std::vector<Point<2> > uv;
		// arc length from the first node, normalised to 1 at the last
		std::vector<double> s;
	};


	// Orders the boundary segments of the face head to tail into one loop.
	// Where several segments leave the same point - the two sides of a seam -
	// the one continuing in the parameter space is taken.
	static bool ChainFaceBoundary(const Mesh & mesh, int facenr, std::vector<Segment> & loop)
	{
		std::vector<Segment> segments;
		for (int i = 1; i <= mesh.GetNSeg(); i++)
			if (mesh.LineSegment(i).si == facenr)
				segments.push_back(mesh.LineSegment(i));

		loop.clear();
		if (segments.size() < 4)
			return false;

		std::vector<std::pair<int, int> > by_start(segments.size());
		for (size_t i = 0; i < segments.size(); i++)
			by_start[i] = std::make_pair(int(segments[i][0]), int(i));
		std::sort(by_start.begin(), by_start.end());

		std::vector<bool> used(segments.size(), false);
		loop.push_back(segments[0]);
		used[0] = true;

		while (loop.size() < segments.size())
		{
			const Segment & last = loop.back();
			std::vector<std::pair<int, int> >::const_iterator it = std::lower_bound(
				by_start.begin(), by_start.end(), std::make_pair(int(last[1]), -1));

			int next = -1;
			double best = 0;
			for (; it != by_start.end() && it->first == int(last[1]); ++it)
			{
				if (used[it->second])
					continue;
				const Segment & candidate = segments[it->second];
				double d = sqr(candidate.epgeominfo[0].u - last.epgeominfo[1].u) +
					sqr(candidate.epgeominfo[0].v - last.epgeominfo[1].v);
				if (next < 0 || d < best)
				{
					next = it->second;
					best = d;
				}
			}

			if (next < 0)
				return false;
			used[next] = true;
			loop.push_back(segments[next]);
		}

		return int(loop.back()[1]) == int(loop.front()[0]);
	}


	// Unit tangent of the edge at one end of the segment, pointing along it.
	static Vec<3> SegmentTangent(const OCCGeometry & geom, const Mesh & mesh,
		const Segment & seg, int end)
	{
		Vec<3> chord = mesh.Point(seg[1]) - mesh.Point(seg[0]);

		const int edgenr = seg.epgeominfo[end].edgenr;
		if (edgenr >= 1 && edgenr <= geom.emap.Extent())
		{
			BRepAdaptor_Curve curve(TopoDS::Edge(geom.emap(edgenr)));
			gp_Pnt p;
			gp_Vec d;
			curve.D1(seg.epgeominfo[end].dist, p, d);

			Vec<3> t(d.X(), d.Y(), d.Z());
			if (t.Length2() > 0)
			{
				if (t * chord < 0)
					t *= -1;
				return (1. / t.Length()) * t;
			}
		}

		if (chord.Length2() > 0)
			chord *= 1. / chord.Length();
		return chord;
	}


	static bool ComputeArcLength(const Mesh & mesh, TransfiniteSide & side)
	{
		side.s.assign(side.pnums.size(), 0.);
		for (size_t i = 1; i < side.pnums.size(); i++)
			side.s[i] = side.s[i - 1] + Dist(mesh.Point(side.pnums[i]), mesh.Point(side.pnums[i - 1]));

		const double length = side.s.back();
		if (length <= 0)
			return false;
		for (size_t i = 0; i < side.s.size(); i++)
			side.s[i] /= length;
		return true;
	}


	static double MinAngle(const Point<3> & a, const Point<3> & b, const Point<3> & c)
	{
		const Point<3> * p[3] = { &a, &b, &c };
		double min_angle = M_PI;
		for (int k = 0; k < 3; k++)
		{
			Vec<3> u = *p[(k + 1) % 3] - *p[k];
			Vec<3> w = *p[(k + 2) % 3] - *p[k];
			double lu = u.Length(), lw = w.Length();
			if (lu <= 0 || lw <= 0)
				return 0;
			double cosine = max(-1., min(1., (u * w) / (lu * lw)));
			min_angle = min(min_angle, acos(cosine));
		}
		return min_angle;
	}


	// Unit tangent of the edge where the wire enters it (at_end false) or
	// leaves it, pointing along the wire.
	static bool WireEdgeTangent(const TopoDS_Edge & edge, bool at_end, gp_Vec & t)
	{
		BRepAdaptor_Curve curve(edge);
		const bool reversed = edge.Orientation() == TopAbs_REVERSED;
		gp_Pnt p;
		curve.D1(at_end != reversed ? curve.LastParameter() : curve.FirstParameter(), p, t);
		if (reversed)
			t.Reverse();
		if (t.Magnitude() <= gp::Resolution())
			return false;
		t.Normalize();
		return true;
	}


	// An internal edge is a line inside the face that a grid would run across,
	// and an external one is not part of its boundary at all.
	static bool HasOnlyBoundaryEdges(const TopoDS_Face & face)
	{
		for (TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next())
		{
			const TopAbs_Orientation orientation = exp.Current().Orientation();
			if (orientation != TopAbs_FORWARD && orientation != TopAbs_REVERSED)
				return false;
		}
		return true;
	}


	bool CollectTransfiniteSides(const OCCGeometry & geom, int facenr,
		double corner_angle, std::vector<int> sides[4])
	{
		for (int q = 0; q < 4; q++)
			sides[q].clear();

		const TopoDS_Face face = TopoDS::Face(geom.fmap(facenr));
		if (!HasOnlyBoundaryEdges(face))
			return false;

		TopoDS_Wire wire;
		int wires = 0;
		for (TopExp_Explorer exp(face, TopAbs_WIRE); exp.More(); exp.Next())
		{
			wire = TopoDS::Wire(exp.Current());
			wires++;
		}
		if (wires != 1)
			return false;

		int wire_edges = 0;
		for (TopExp_Explorer exp(wire, TopAbs_EDGE); exp.More(); exp.Next())
			wire_edges++;
		if (wire_edges < 4)
			return false;

		// the wire explorer gives the edges head to tail, a seam twice
		std::vector<TopoDS_Edge> edges;
		for (BRepTools_WireExplorer exp(wire, face); exp.More(); exp.Next())
		{
			const TopoDS_Edge & edge = exp.Current();
			if (BRep_Tool::Degenerated(edge))
				return false;
			edges.push_back(edge);
		}
		const int nedges = int(edges.size());
		if (nedges != wire_edges)
			return false;

		// corners[k]: the wire turns sharply where edge k starts
		const double corner_turn = M_PI - corner_angle * M_PI / 180.;
		std::vector<int> corners;
		for (int k = 0; k < nedges; k++)
		{
			gp_Vec t_in, t_out;
			if (!WireEdgeTangent(edges[(k + nedges - 1) % nedges], true, t_in) ||
				!WireEdgeTangent(edges[k], false, t_out))
				return false;
			if (t_in.Angle(t_out) > corner_turn)
				corners.push_back(k);
		}
		if (corners.size() != 4)
			return false;

		for (int q = 0; q < 4; q++)
			for (int k = corners[q]; k != corners[(q + 1) % 4]; k = (k + 1) % nedges)
			{
				const int edgenr = geom.emap.FindIndex(edges[k]);
				if (edgenr < 1)
					return false;
				sides[q].push_back(edgenr);
			}
		return true;
	}


	// The structured grid of a face between the corners found on its boundary loop: corners[q] is the loop
	// segment side q starts with.  Strict leaves out the checks that hand a face back to the advancing front -
	// the triangle angle and the size - and keeps only the fold, giving the reason in failure.  With quads every
	// grid cell is one quad, else two triangles.
	static bool BuildGridMesh(const OCCGeometry & geom, const Mesh & mesh, int facenr, const std::vector<Segment> & loop,
		const std::vector<int> & corners, bool strict, bool check_size, bool quads, OCCTransfiniteFaceMesh & result,
		std::string & failure)
	{
		const TopoDS_Face face = TopoDS::Face(geom.fmap(facenr));
		const int nseg = int(loop.size());

		// The sides run along the loop from corner to corner.
		TransfiniteSide sides[4];
		for (int q = 0; q < 4; q++)
		{
			const int first = corners[q];
			const int count = (corners[(q + 1) % 4] - first + nseg) % nseg;
			TransfiniteSide & side = sides[q];

			for (int c = 0; c < count; c++)
			{
				const Segment & seg = loop[(first + c) % nseg];
				side.pnums.push_back(seg[0]);
				side.uv.push_back(Point<2>(seg.epgeominfo[0].u, seg.epgeominfo[0].v));
			}
			const Segment & last = loop[(first + count - 1) % nseg];
			side.pnums.push_back(last[1]);
			side.uv.push_back(Point<2>(last.epgeominfo[1].u, last.epgeominfo[1].v));
		}

		const int n = int(sides[0].pnums.size()) - 1;
		const int m = int(sides[1].pnums.size()) - 1;
		if (int(sides[2].pnums.size()) != n + 1 || int(sides[3].pnums.size()) != m + 1)
		{
			failure = "has opposite sides with different numbers of segments";
			return false;
		}

		// Bottom and right follow the loop, top and left run against it, so all
		// four go from low to high grid index.
		TransfiniteSide & bottom = sides[0];
		TransfiniteSide & right = sides[1];
		TransfiniteSide & top = sides[2];
		TransfiniteSide & left = sides[3];
		std::reverse(top.pnums.begin(), top.pnums.end());
		std::reverse(top.uv.begin(), top.uv.end());
		std::reverse(left.pnums.begin(), left.pnums.end());
		std::reverse(left.uv.begin(), left.uv.end());

		for (int q = 0; q < 4; q++)
			if (!ComputeArcLength(mesh, sides[q]))
			{
				failure = "has a side of no length";
				return false;
			}

		Box<2> uvbox(bottom.uv[0], bottom.uv[0]);
		for (int q = 0; q < 4; q++)
			for (size_t i = 0; i < sides[q].uv.size(); i++)
				uvbox.Add(sides[q].uv[i]);
		const double uvscale = max(uvbox.PMax()(0) - uvbox.PMin()(0), uvbox.PMax()(1) - uvbox.PMin()(1));
		if (uvscale <= 0)
		{
			failure = "has no extent in its parameter space";
			return false;
		}

		const Point<2> c0 = bottom.uv[0], c1 = bottom.uv[n], c2 = top.uv[n], c3 = top.uv[0];
		const double corner_tolerance = 1e-3 * uvscale;
		if (Dist(c0, left.uv[0]) > corner_tolerance || Dist(c1, right.uv[0]) > corner_tolerance ||
			Dist(c2, right.uv[m]) > corner_tolerance || Dist(c3, left.uv[m]) > corner_tolerance)
		{
			failure = "does not close at its corners in its parameter space";
			return false;
		}

		const int nx = n + 1;
		const int ny = m + 1;
		std::vector<int> pnums(nx * ny);
		std::vector<Point<2> > uv(nx * ny);
		std::vector<Point<3> > xyz(nx * ny);

		for (int j = 0; j <= m; j++)
		{
			pnums[j * nx] = left.pnums[j];
			uv[j * nx] = left.uv[j];
			pnums[j * nx + n] = right.pnums[j];
			uv[j * nx + n] = right.uv[j];
		}
		for (int i = 0; i <= n; i++)
		{
			pnums[i] = bottom.pnums[i];
			uv[i] = bottom.uv[i];
			pnums[m * nx + i] = top.pnums[i];
			uv[m * nx + i] = top.uv[i];
		}
		for (int j = 0; j <= m; j++)
			for (int i = 0; i <= n; i++)
				if (i == 0 || j == 0 || i == n || j == m)
					xyz[j * nx + i] = mesh.Point(pnums[j * nx + i]);

		// The logical position of a node is where the line between its bottom
		// and top nodes crosses the one between its left and right nodes, which
		// carries the spacing of the sides into the interior.
		Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
		for (int j = 1; j < m; j++)
			for (int i = 1; i < n; i++)
			{
				const double sb = bottom.s[i], st = top.s[i];
				const double sl = left.s[j], sr = right.s[j];
				const double xi = (sb + sl * (st - sb)) / (1 - (st - sb) * (sr - sl));
				const double eta = sl + xi * (sr - sl);

				Point<2> p;
				for (int d = 0; d < 2; d++)
					p(d) = (1 - xi) * left.uv[j](d) + xi * right.uv[j](d)
						+ (1 - eta) * bottom.uv[i](d) + eta * top.uv[i](d)
						- ((1 - xi) * (1 - eta) * c0(d) + xi * (1 - eta) * c1(d)
							+ xi * eta * c2(d) + (1 - xi) * eta * c3(d));

				const gp_Pnt sp = surface->Value(p(0), p(1));
				const int k = j * nx + i;
				uv[k] = p;
				xyz[k] = Point<3>(sp.X(), sp.Y(), sp.Z());
				result.points.push_back(MeshPoint(xyz[k]));
				pnums[k] = -int(result.points.size());
			}

		// The loop runs along the boundary segments, so a triangle whose nodes
		// go round the grid in increasing (i, j) order passes each boundary
		// segment in its own direction, as the advancing front does.
		const double min_uv_area = 1e-12 * uvscale * uvscale;
		double orientation = 0;
		for (int j = 0; j < m; j++)
			for (int i = 0; i < n; i++)
			{
				const int k00 = j * nx + i, k10 = k00 + 1, k01 = k00 + nx, k11 = k01 + 1;
				// the diagonals alternate from cell to cell, as Gmsh's AlternateLeft
				const bool diagonal_00 = (i + j) % 2 == 0;
				const int trigs[2][3] = {
					{ k00, k10, diagonal_00 ? k11 : k01 },
					{ diagonal_00 ? k00 : k10, k11, k01 } };

				for (int t = 0; t < 2; t++)
				{
					const int a = trigs[t][0], b = trigs[t][1], c = trigs[t][2];

					// a grid folded over in the parameter space flips triangles
					const Vec<2> ab = uv[b] - uv[a], ac = uv[c] - uv[a];
					const double area = ab(0) * ac(1) - ab(1) * ac(0);
					if (orientation == 0)
						orientation = area > 0 ? 1 : -1;
					if (area * orientation <= min_uv_area ||
						(!strict && MinAngle(xyz[a], xyz[b], xyz[c]) < transfinite_min_angle))
					{
						result.points.clear();
						result.elements.clear();
						failure = "folds: its grid turns over in its parameter space";
						return false;
					}
					if (quads)
						continue;

					Element2d element(pnums[a], pnums[b], pnums[c]);
					element.SetIndex(facenr);
					const int nodes[3] = { a, b, c };
					for (int v = 0; v < 3; v++)
					{
						PointGeomInfo & gi = element.GeomInfoPi(v + 1);
						gi.trignum = facenr;
						gi.u = uv[nodes[v]](0);
						gi.v = uv[nodes[v]](1);
					}
					result.elements.push_back(element);
				}
				if (!quads)
					continue;
				// The cell as one quad, turned as the triangles it was checked as
				Element2d element(pnums[k00], pnums[k10], pnums[k11], pnums[k01]);
				element.SetIndex(facenr);
				const int nodes[4] = { k00, k10, k11, k01 };
				for (int v = 0; v < 4; v++)
				{
					PointGeomInfo & gi = element.GeomInfoPi(v + 1);
					gi.trignum = facenr;
					gi.u = uv[nodes[v]](0);
					gi.v = uv[nodes[v]](1);
				}
				result.elements.push_back(element);
			}

		if (check_size && !strict)
		{
			// The advancing front coarsens from the boundary towards the mesh size
			// inside the face, which a grid cannot: a finely divided stretch of a side
			// runs across the whole face as a row of small cells.  The mesh size asks
			// for about an equilateral triangle of the local size per unit of area,
			// and never fewer triangles than it takes to fill the boundary.
			const double face_maxh = geom.face_maxh[facenr - 1];
			double wanted = 0;
			for (size_t k = 0; k < result.elements.size(); k++)
			{
				Point<3> p[3];
				for (int v = 0; v < 3; v++)
				{
					const int pnum = result.elements[k].PNum(v + 1);
					p[v] = pnum < 0 ? Point<3>(result.points[-pnum - 1]) : Point<3>(mesh.Point(pnum));
				}
				const Point<3> centre = Center(p[0], p[1], p[2]);
				const double h = min(face_maxh, mesh.GetH(Point3d(centre(0), centre(1), centre(2))));
				const double area = 0.5 * Cross(p[1] - p[0], p[2] - p[0]).Length();
				wanted += area / (sqrt(3.) / 4 * h * h);
			}
			wanted = max(wanted, double(nseg - 2));
			if (result.elements.size() > transfinite_max_excess * wanted)
			{
				result.points.clear();
				result.elements.clear();
				return false;
			}
		}

		return true;
	}


	bool BuildTransfiniteFaceMesh(const OCCGeometry & geom, const Mesh & mesh,
		int facenr, double corner_angle, bool check_size, OCCTransfiniteFaceMesh & result)
	{
		result.points.clear();
		result.elements.clear();

		const TopoDS_Face face = TopoDS::Face(geom.fmap(facenr));

		// A hole makes a second wire, and a degenerated edge (the tip of a cone)
		// leaves a gap in the parameter space that the sides cannot bridge.
		int wires = 0;
		for (TopExp_Explorer exp(face, TopAbs_WIRE); exp.More(); exp.Next())
			wires++;
		if (wires != 1 || !HasOnlyBoundaryEdges(face))
			return false;
		for (TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next())
			if (BRep_Tool::Degenerated(TopoDS::Edge(exp.Current())))
				return false;

		std::vector<Segment> loop;
		if (!ChainFaceBoundary(mesh, facenr, loop))
			return false;
		const int nseg = int(loop.size());

		// Corners can only be at vertices: where the next segment belongs to
		// another edge, or restarts a closed one.  Edges meeting at a flatter
		// angle are joined into one side.
		const double corner_turn = M_PI - corner_angle * M_PI / 180.;
		std::vector<int> corners;
		for (int k = 0; k < nseg; k++)
		{
			const Segment & in = loop[(k + nseg - 1) % nseg];
			const Segment & out = loop[k];
			if (in.epgeominfo[1].edgenr == out.epgeominfo[0].edgenr &&
				in.epgeominfo[1].dist == out.epgeominfo[0].dist)
				continue;

			double cosine = SegmentTangent(geom, mesh, in, 1) * SegmentTangent(geom, mesh, out, 0);
			if (acos(max(-1., min(1., cosine))) > corner_turn)
				corners.push_back(k);
		}
		if (corners.size() != 4)
			return false;

		std::string failure;
		return BuildGridMesh(geom, mesh, facenr, loop, corners, false, check_size, false, result, failure);
	}
	OCCGivenTransfiniteFaces & GivenTransfiniteFaces()
	{
		static OCCGivenTransfiniteFaces given;
		return given;
	}


	// The file PrePoMax writes:
	//   faces <count>
	//   face <face map number> <number of sides> <name>
	//   side <number of edges> <edge map numbers in loop order>      once for every side
	//   edges <count>
	//   edge <edge map number> <segments the user set, 0 for none> <name>
	void LoadGivenTransfiniteFaces(const std::string & filename)
	{
		OCCGivenTransfiniteFaces & given = GivenTransfiniteFaces();
		given = OCCGivenTransfiniteFaces();
		if (filename.empty())
			return;

		std::ifstream file(filename.c_str());
		if (!file)
			throw NgException("Transfinite: the file of the structured faces " + filename + " cannot be read.");

		std::string line, word;
		int count = 0;
		auto rest = [](std::istringstream & stream)
		{
			std::string text;
			std::getline(stream, text);
			size_t first = text.find_first_not_of(' ');
			return first == std::string::npos ? std::string() : text.substr(first);
		};
		while (std::getline(file, line))
		{
			std::istringstream stream(line);
			if (!(stream >> word))
				continue;
			if (word == "faces")
				stream >> count;
			else if (word == "face")
			{
				int facenr = 0, numsides = 0;
				stream >> facenr >> numsides;
				given.face_names[facenr] = rest(stream);
				std::vector<std::vector<int> > & sides = given.sides[facenr];
				for (int q = 0; q < numsides && std::getline(file, line); q++)
				{
					std::istringstream sidestream(line);
					int numedges = 0;
					sidestream >> word >> numedges;
					if (word != "side")
						throw NgException("Transfinite: the sides of face " + std::to_string(facenr) +
							" are not given in " + filename + ".");
					std::vector<int> side(numedges);
					for (int k = 0; k < numedges; k++)
						sidestream >> side[k];
					sides.push_back(side);
				}
			}
			else if (word == "edges")
				stream >> count;
			else if (word == "edge")
			{
				int edgenr = 0, pinned = 0;
				stream >> edgenr >> pinned;
				given.edge_names[edgenr] = rest(stream);
				if (pinned > 0)
					given.pinned_counts[edgenr] = pinned;
			}
		}
		given.loaded = true;
		PrintMessage(3, "Transfinite: ", int(given.sides.size()), " structured faces given");
	}


	static std::string GivenFaceName(int facenr)
	{
		const OCCGivenTransfiniteFaces & given = GivenTransfiniteFaces();
		std::map<int, std::string>::const_iterator it = given.face_names.find(facenr);
		return it != given.face_names.end() && !it->second.empty() ? it->second : "face " + std::to_string(facenr);
	}


	static std::string GivenEdgeName(int edgenr)
	{
		const OCCGivenTransfiniteFaces & given = GivenTransfiniteFaces();
		std::map<int, std::string>::const_iterator it = given.edge_names.find(edgenr);
		return it != given.edge_names.end() && !it->second.empty() ? it->second : "edge " + std::to_string(edgenr);
	}


	void BalanceGivenTransfiniteEdgeCounts(const std::vector<int> & own, std::vector<int> & counts)
	{
		const OCCGivenTransfiniteFaces & given = GivenTransfiniteFaces();
		const int nedges = int(counts.size());
		auto valid = [&](int edgenr) { return edgenr >= 1 && edgenr <= nedges; };

		std::set<int> pinned;
		for (std::map<int, int>::const_iterator it = given.pinned_counts.begin(); it != given.pinned_counts.end(); ++it)
			if (valid(it->first))
			{
				counts[it->first - 1] = std::max(1, it->second);
				pinned.insert(it->first - 1);
			}

		struct SidePair
		{
			int facenr;
			std::vector<int> a, b;
		};
		std::vector<SidePair> pairs;
		for (std::map<int, std::vector<std::vector<int> > >::const_iterator it = given.sides.begin();
			it != given.sides.end(); ++it)
		{
			if (it->second.size() != 4)
				throw NgException("Transfinite: " + GivenFaceName(it->first) + " has " +
					std::to_string(it->second.size()) + " sides; Netgen meshes structured faces of 4 sides.");
			for (int q = 0; q < 2; q++)
			{
				SidePair pair;
				pair.facenr = it->first;
				for (int e : it->second[q])
				{
					if (!valid(e)) throw NgException("Transfinite: " + GivenFaceName(it->first) + " names an edge it does not have.");
					pair.a.push_back(e - 1);
				}
				for (int e : it->second[q + 2])
				{
					if (!valid(e)) throw NgException("Transfinite: " + GivenFaceName(it->first) + " names an edge it does not have.");
					pair.b.push_back(e - 1);
				}
				pairs.push_back(pair);
			}
		}

		// Classes of the edges that are whole opposite sides
		std::vector<int> parent(nedges);
		std::iota(parent.begin(), parent.end(), 0);
		auto find = [&](int i)
		{
			int root = i;
			while (parent[root] != root) root = parent[root];
			while (parent[i] != root) { int next = parent[i]; parent[i] = root; i = next; }
			return root;
		};
		for (const SidePair & pair : pairs)
			if (pair.a.size() == 1 && pair.b.size() == 1)
				parent[find(pair.a[0])] = find(pair.b[0]);

		std::map<int, int> value, floor_value;
		std::map<int, double> natural;
		std::map<int, int> natural_count;
		std::set<int> fixed_classes, conflicting_classes;
		std::map<int, std::set<int> > pinned_values;
		for (int i = 0; i < nedges; i++)
		{
			const int root = find(i);
			const int n = std::max(1, own[i]);
			if (value.find(root) == value.end())
			{
				value[root] = n;
				natural[root] = 0;
				natural_count[root] = 0;
				floor_value[root] = 1;
			}
			value[root] = std::min(value[root], n);
			natural[root] += n;
			natural_count[root]++;
			if (pinned.count(i))
				pinned_values[root].insert(counts[i]);
		}
		for (std::map<int, double>::iterator it = natural.begin(); it != natural.end(); ++it)
			it->second /= natural_count[it->first];
		for (std::map<int, std::set<int> >::const_iterator it = pinned_values.begin(); it != pinned_values.end(); ++it)
		{
			value[it->first] = *it->second.rbegin();
			fixed_classes.insert(it->first);
			if (it->second.size() > 1)
				conflicting_classes.insert(it->first);
		}

		// The sides that are chains of edges, balanced over the classes
		std::vector<const SidePair *> chains;
		for (const SidePair & pair : pairs)
			if (pair.a.size() > 1 || pair.b.size() > 1)
				chains.push_back(&pair);
		auto side_sum = [&](const std::vector<int> & side)
		{
			int sum = 0;
			for (int e : side) sum += value[find(e)];
			return sum;
		};
		long long total = 0;
		for (std::map<int, int>::const_iterator it = value.begin(); it != value.end(); ++it) total += it->second;
		const long long max_iterations = 1000LL * std::max<size_t>(1, chains.size()) + 10LL * total;
		bool changed = true;
		for (long long iteration = 0; changed && iteration < max_iterations; iteration++)
		{
			changed = false;
			for (const SidePair * pair : chains)
			{
				const int sum_a = side_sum(pair->a), sum_b = side_sum(pair->b);
				if (sum_a == sum_b)
					continue;
				const std::vector<int> & larger = sum_a > sum_b ? pair->a : pair->b;
				const std::vector<int> & smaller = sum_a > sum_b ? pair->b : pair->a;
				// A class on both sides changes them both, so only what it does to the difference counts
				auto net = [&](int root)
				{
					int n = 0;
					for (int e : larger) if (find(e) == root) n++;
					for (int e : smaller) if (find(e) == root) n--;
					return n;
				};
				int best = -1;
				for (int e : larger)
				{
					const int root = find(e);
					if (fixed_classes.count(root) || value[root] <= floor_value[root] || net(root) <= 0)
						continue;
					if (best < 0 || value[root] / natural[root] > value[best] / natural[best])
						best = root;
				}
				if (best >= 0)
				{
					value[best]--;
					changed = true;
					continue;
				}
				for (int e : smaller)
				{
					const int root = find(e);
					if (fixed_classes.count(root) || net(root) >= 0)
						continue;
					if (best < 0 || value[root] / natural[root] < value[best] / natural[best])
						best = root;
				}
				if (best >= 0)
				{
					value[best]++;
					floor_value[best] = value[best];
					changed = true;
				}
			}
		}

		for (int i = 0; i < nedges; i++)
			if (!pinned.count(i))
				counts[i] = value[find(i)];

		std::string message;
		auto describe = [&](const std::vector<int> & side)
		{
			std::string text;
			for (size_t k = 0; k < side.size(); k++)
			{
				if (k) text += ", ";
				text += GivenEdgeName(side[k] + 1);
				if (pinned.count(side[k])) text += " (set to " + std::to_string(counts[side[k]]) + ")";
			}
			return text;
		};
		for (const SidePair & pair : pairs)
		{
			int sum_a = 0, sum_b = 0;
			for (int e : pair.a) sum_a += counts[e];
			for (int e : pair.b) sum_b += counts[e];
			if (sum_a == sum_b && !conflicting_classes.count(find(pair.a[0])))
				continue;
			message += "Transfinite: the opposite sides of " + GivenFaceName(pair.facenr) + " cannot be given the same "
				"number of elements: " + describe(pair.a) + " against " + describe(pair.b) + ".\n";
		}
		if (!message.empty())
			throw NgException(message + "Change the number of elements set on these edges, or switch off the "
				"transfinite meshing of the 4-sided surfaces.");
	}


	void BuildGivenTransfiniteFaceMesh(const OCCGeometry & geom, const Mesh & mesh, int facenr, bool quads,
		OCCTransfiniteFaceMesh & result)
	{
		result.points.clear();
		result.elements.clear();
		const OCCGivenTransfiniteFaces & given = GivenTransfiniteFaces();
		const std::vector<std::vector<int> > & sides = given.sides.at(facenr);

		std::map<int, int> edge_side;
		for (size_t q = 0; q < sides.size(); q++)
			for (int e : sides[q])
				edge_side[e] = int(q);

		std::vector<Segment> loop;
		if (!ChainFaceBoundary(mesh, facenr, loop))
			throw NgException("Transfinite: the boundary of " + GivenFaceName(facenr) + " is not one loop of segments.");

		// A corner is where the loop passes from one side to the next
		const int nseg = int(loop.size());
		std::vector<int> corners;
		for (int k = 0; k < nseg; k++)
		{
			std::map<int, int>::const_iterator in = edge_side.find(loop[(k + nseg - 1) % nseg].epgeominfo[1].edgenr);
			std::map<int, int>::const_iterator out = edge_side.find(loop[k].epgeominfo[0].edgenr);
			if (in == edge_side.end() || out == edge_side.end())
				throw NgException("Transfinite: the boundary of " + GivenFaceName(facenr) +
					" runs along an edge none of its sides has.");
			if (in->second != out->second)
				corners.push_back(k);
		}
		if (corners.size() != sides.size())
			throw NgException("Transfinite: the boundary of " + GivenFaceName(facenr) + " does not pass its " +
				std::to_string(sides.size()) + " sides one after another.");

		std::string failure;
		if (!BuildGridMesh(geom, mesh, facenr, loop, corners, true, false, quads, result, failure))
			throw NgException("Transfinite: " + GivenFaceName(facenr) + " " + failure + ".");
	}
}

#endif
