#ifndef FILE_OCCTRANSFINITE
#define FILE_OCCTRANSFINITE

#include <vector>

namespace netgen
{
  /**
     Structured triangulation of a four sided face.

     Points follow the convention of the parallel face results: an element
     vertex that is already in the mesh keeps its number, a new point is
     referenced as -(1 based index into points).
  */
  struct OCCTransfiniteFaceMesh
  {
    std::vector<MeshPoint> points;
    std::vector<Element2d> elements;
  };

  /**
     Meshes face facenr as a transfinite region if it is one.

     The face qualifies when it has a single wire with exactly four corners -
     vertices where the edges meet at less than corner_angle degrees, 180 being
     a straight continuation - and the edge meshing already put the same number
     of segments on opposite sides.  The interior nodes are placed by
     transfinite interpolation in the parameter space of the face, and each grid
     cell is split into two triangles, the diagonals alternating from cell to
     cell.  Returns false, leaving result empty, when the face does not qualify
     or the grid comes out folded, so the caller can mesh it the usual way.  With
     check_size also when the grid has much more triangles than the mesh size
     asks for, since a grid cannot coarsen away from a finely divided side.

     Reads mesh and geom only, so faces can be built on worker threads.
  */
  bool BuildTransfiniteFaceMesh(const OCCGeometry & geom, const Mesh & mesh,
    int facenr, double corner_angle, bool check_size, OCCTransfiniteFaceMesh & result);

  /**
     Splits the wire of face facenr at its corners, in the sense of
     BuildTransfiniteFaceMesh, into four sides and gives the edges of each -
     as edge map numbers, in wire order.  Sides 0 and 2 are opposite, as are
     1 and 3.  The edge meshing makes opposite sides add up to the same
     segment count so that the face qualifies as a structured grid.  Returns
     false for a face that does not have exactly four corners.
  */
  bool CollectTransfiniteSides(const OCCGeometry & geom, int facenr,
    double corner_angle, std::vector<int> sides[4]);
}

#endif
