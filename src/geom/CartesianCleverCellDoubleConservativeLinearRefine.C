//////////////////////////////////////////////////////////////////////////////
// Crown Copyright 2014 AWE, Copyright 2014 David Beckingsale.
//
// This file is part of CleverLeaf.
//
// CleverLeaf is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// CleverLeaf is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// CleverLeaf. If not, see http://www.gnu.org/licenses/.
//////////////////////////////////////////////////////////////////////////////
#include "CartesianCleverCellDoubleConservativeLinearRefine.h"

#include "pdat/CleverCellData.h"

#include "SAMRAI/pdat/CellOverlap.h"
#include "SAMRAI/geom/CartesianPatchGeometry.h"

#include "macros.h"

#define F90_FUNC(name,NAME) name ## _

extern "C" {
  void F90_FUNC(cartclinrefclevercelldoub2d, CARTCLINREFCLEVERCELLDOUB2D)
    (const int&,const int&,const int&,const int&,
     const int&,const int&,const int&,const int&,
     const int&,const int&,const int&,const int&,
     const int&,const int&,const int&,const int&,
     const int*,const double*,const double*,
     const double*,double*, double*,double*,double*,double*);
}

namespace clever {
namespace geom {

CartesianCleverCellDoubleConservativeLinearRefine::
CartesianCleverCellDoubleConservativeLinearRefine():
  SAMRAI::hier::RefineOperator("CONSERVATIVE_LINEAR_REFINE")
{
}

CartesianCleverCellDoubleConservativeLinearRefine::
~CartesianCleverCellDoubleConservativeLinearRefine()
{
}

int CartesianCleverCellDoubleConservativeLinearRefine::getOperatorPriority() const
{
  return 0;
}

SAMRAI::hier::IntVector 
CartesianCleverCellDoubleConservativeLinearRefine::getStencilWidth(
    const SAMRAI::tbox::Dimension &dim) const
{
  return SAMRAI::hier::IntVector::getOne(dim);
}

void CartesianCleverCellDoubleConservativeLinearRefine::refine(
    SAMRAI::hier::Patch& fine,
    const SAMRAI::hier::Patch& coarse,
    const int dst_component,
    const int src_component,
    const SAMRAI::hier::BoxOverlap& fine_overlap,
    const SAMRAI::hier::IntVector& ratio) const
{
  const SAMRAI::pdat::CellOverlap* cell_overlap =
    PTR_CAST(const SAMRAI::pdat::CellOverlap *,&fine_overlap);

  const SAMRAI::hier::BoxContainer& boxes = 
    cell_overlap->getDestinationBoxContainer();

  for (SAMRAI::hier::BoxContainer::const_iterator box = boxes.begin();
      box != boxes.end();
      ++box) {
    refine(fine,
        coarse,
        dst_component,
        src_component,
        *box,
        ratio);
  }
}

void CartesianCleverCellDoubleConservativeLinearRefine::refine(
    SAMRAI::hier::Patch& fine,
    const SAMRAI::hier::Patch& coarse,
    const int dst_component,
    const int src_component,
    const SAMRAI::hier::Box& fine_box,
    const SAMRAI::hier::IntVector& ratio) const
{
  const SAMRAI::tbox::Dimension& dim(fine.getDim());

  boost::shared_ptr<pdat::CleverCellData<double> > coarse_data(
      SHARED_PTR_CAST(pdat::CleverCellData<double>,
        coarse.getPatchData(src_component)));

  boost::shared_ptr<pdat::CleverCellData<double> > fine_data(
      SHARED_PTR_CAST(pdat::CleverCellData<double>,
        fine.getPatchData(dst_component)));

  const boost::shared_ptr<SAMRAI::geom::CartesianPatchGeometry> coarse_geometry(
      SHARED_PTR_CAST(SAMRAI::geom::CartesianPatchGeometry,
        coarse.getPatchGeometry()));

  const boost::shared_ptr<SAMRAI::geom::CartesianPatchGeometry> fine_geometry(
      SHARED_PTR_CAST(SAMRAI::geom::CartesianPatchGeometry,
        fine.getPatchGeometry()));


   const SAMRAI::hier::Box coarse_box = SAMRAI::hier::Box::coarsen(fine_box, ratio);
   const SAMRAI::hier::Index ifirstc = coarse_box.lower();
   const SAMRAI::hier::Index ilastc = coarse_box.upper();
   const SAMRAI::hier::Index ifirstf = fine_box.lower();
   const SAMRAI::hier::Index ilastf = fine_box.upper();

  const SAMRAI::hier::Box coarse_ghost_box = coarse_data->getGhostBox();

   const SAMRAI::hier::Index cilo = coarse_ghost_box.lower();
   const SAMRAI::hier::Index cihi = coarse_ghost_box.upper();
   const SAMRAI::hier::Index filo = fine_data->getGhostBox().lower();
   const SAMRAI::hier::Index fihi = fine_data->getGhostBox().upper();

  SAMRAI::hier::Box diff_box = coarse_data->getGhostBox();
  diff_box.growUpper(SAMRAI::hier::IntVector::getOne(dim));

  const SAMRAI::hier::IntVector tmp_ghosts(
      SAMRAI::hier::IntVector::getZero(dim));

  clever::pdat::CleverCellData<double> diff0(diff_box, 1, tmp_ghosts);
  clever::pdat::CleverCellData<double> diff1(diff_box, 1, tmp_ghosts);

  clever::pdat::CleverCellData<double> slope0(coarse_ghost_box, 1, tmp_ghosts);
  clever::pdat::CleverCellData<double> slope1(coarse_ghost_box, 1, tmp_ghosts);

  for(int depth = 0; depth < fine_data->getDepth(); depth++) {
    if (dim == SAMRAI::tbox::Dimension(2)) {
      F90_FUNC(cartclinrefclevercelldoub2d, CARTCLINREFCLEVERCELLDOUB2D)
        (ifirstc(0), ifirstc(1), ilastc(0), ilastc(1), ifirstf(0),
         ifirstf(1), ilastf(0), ilastf(1), cilo(0), cilo(1), cihi(0),
         cihi(1), filo(0), filo(1), fihi(0), fihi(1), &ratio[0],
         coarse_geometry->getDx(), fine_geometry->getDx(), coarse_data->getPointer(depth),
         fine_data->getPointer(depth), diff0.getPointer(), slope0.getPointer(),
         diff1.getPointer(), slope1.getPointer());
    } else {
      TBOX_ERROR("CartesianCleverCellDoubleConservativeLinearRefine error...\n"
        << "dim != 2 not supported." << std::endl);
    }
  }
}

}
}
