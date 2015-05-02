/**
 * @file    Simple.cc
 * @author  Adam O'Brien <obrienadam89@gmail.com>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This file contains the implementations for the methods of class
 * Simple.
 */

#include <algorithm>

#include "Simple.h"

// ************* Constructors and Destructors *************

Simple::Simple()
    :
      aP_("aP", AUXILLARY),
      aE_("aE", AUXILLARY),
      aW_("aW", AUXILLARY),
      aN_("aN", AUXILLARY),
      aS_("aS", AUXILLARY),
      aT_("aT", AUXILLARY),
      aB_("aB", AUXILLARY),
      bP_("bP", AUXILLARY),
      massFlow_("massFlow", PRIMITIVE),
      pCorr_("pCorr", PRIMITIVE),
      gradPCorr_("gradPCorr", PRIMITIVE),
      dField_("dField", PRIMITIVE),
      gradUField_("gradUField", PRIMITIVE),
      gradPField_("gradPField", PRIMITIVE),
      relaxationFactor_(0.8),
      rho_(998.),
      mu_(0.1),
      maxGsIters_(20)
{
    nu_ = mu_/rho_;
}

// ************* Private Methods *************

void Simple::computeMassFlow(Field<Vector3D> &uField)
{
    int i, j, k, uI, uJ, uK;
    HexaFvmMesh& mesh = *meshPtr_;

    uI = nFacesI_ - 1;
    uJ = nFacesJ_ - 1;
    uK = nFacesK_ - 1;

    for(k = 0; k < nFacesK_; ++k)
    {
        for(j = 0; j < nFacesJ_; ++j)
        {
            for(i = 0; i < nFacesI_; ++i)
            {
                if(j < uJ && k < uK)
                    massFlow_.faceI(i, j, k) = rho_*dot(uField.faceI(i, j, k), mesh.fAreaNormI(i, j, k));

                if(i < uI && k < uK)
                    massFlow_.faceJ(i, j, k) = rho_*dot(uField.faceJ(i, j, k), mesh.fAreaNormJ(i, j, k));

                if(i < uI && j < uJ)
                    massFlow_.faceK(i, j, k) = rho_*dot(uField.faceK(i, j, k), mesh.fAreaNormK(i, j, k));
            }
        }
    }
}

void Simple::computeMomentum(Field<Vector3D>& uField, Field<double>& pField)
{
    using namespace std;

    int i, j, k, l, itrNo;
    HexaFvmMesh& mesh = *meshPtr_;
    double alpha;
    Vector3D sf, ds;
    Tensor3D gradUBar;

    pField.setBoundaryFields();
    uField.setBoundaryFields();

    FvScheme::interpolateInteriorFaces(uField);

    computeMassFlow(uField);

    computeCellCenteredJacobians(uField, gradUField_);
    computeCellCenteredGradients(pField, gradPField_);

    for(k = 0; k < nCellsK_; ++k)
    {
        for(j = 0; j < nCellsJ_; ++j)
        {
            for(i = 0; i < nCellsI_; ++i)
            {
                //- Some of these coefficient computations are redundant. This should be fixed in the future

                // Face coefficients
                aE_(i, j, k) =  min(massFlow_.faceE(i, j, k), 0.) - mu_*dot(mesh.fAreaNormE(i, j, k), mesh.fAreaNormE(i, j, k))/dot(mesh.fAreaNormE(i, j, k), mesh.rCellE(i, j, k));
                aW_(i, j, k) =  - max(massFlow_.faceW(i, j, k), 0.) - mu_*dot(mesh.fAreaNormW(i, j, k), mesh.fAreaNormW(i, j, k))/dot(mesh.fAreaNormW(i, j, k), mesh.rCellW(i, j, k));
                aN_(i, j, k) =  + min(massFlow_.faceN(i, j, k), 0.) - mu_*dot(mesh.fAreaNormN(i, j, k), mesh.fAreaNormN(i, j, k))/dot(mesh.fAreaNormN(i, j, k), mesh.rCellN(i, j, k));
                aS_(i, j, k) =  - max(massFlow_.faceS(i, j, k), 0.) - mu_*dot(mesh.fAreaNormS(i, j, k), mesh.fAreaNormS(i, j, k))/dot(mesh.fAreaNormS(i, j, k), mesh.rCellS(i, j, k));
                aT_(i, j, k) =  + min(massFlow_.faceT(i, j, k), 0.) - mu_*dot(mesh.fAreaNormT(i, j, k), mesh.fAreaNormT(i, j, k))/dot(mesh.fAreaNormT(i, j, k), mesh.rCellT(i, j, k));
                aB_(i, j, k) =  - max(massFlow_.faceB(i, j, k), 0.) - mu_*dot(mesh.fAreaNormB(i, j, k), mesh.fAreaNormB(i, j, k))/dot(mesh.fAreaNormB(i, j, k), mesh.rCellB(i, j, k));

                // Central coefficient

                aP_(i, j, k) = max(massFlow_.faceE(i, j, k), 0.) + mu_*dot(mesh.fAreaNormE(i, j, k), mesh.fAreaNormE(i, j, k))/dot(mesh.fAreaNormE(i, j, k), mesh.rCellE(i, j, k))
                        - min(massFlow_.faceW(i, j, k), 0.) + mu_*dot(mesh.fAreaNormW(i, j, k), mesh.fAreaNormW(i, j, k))/dot(mesh.fAreaNormW(i, j, k), mesh.rCellW(i, j, k))
                        + max(massFlow_.faceN(i, j, k), 0.) + mu_*dot(mesh.fAreaNormN(i, j, k), mesh.fAreaNormN(i, j, k))/dot(mesh.fAreaNormN(i, j, k), mesh.rCellN(i, j, k))
                        - min(massFlow_.faceS(i, j, k), 0.) + mu_*dot(mesh.fAreaNormS(i, j, k), mesh.fAreaNormS(i, j, k))/dot(mesh.fAreaNormS(i, j, k), mesh.rCellS(i, j, k))
                        + max(massFlow_.faceT(i, j, k), 0.) + mu_*dot(mesh.fAreaNormT(i, j, k), mesh.fAreaNormT(i, j, k))/dot(mesh.fAreaNormT(i, j, k), mesh.rCellT(i, j, k))
                        - min(massFlow_.faceB(i, j, k), 0.) + mu_*dot(mesh.fAreaNormB(i, j, k), mesh.fAreaNormB(i, j, k))/dot(mesh.fAreaNormB(i, j, k), mesh.rCellB(i, j, k));

                // Compute the cross-diffusion terms (due to mesh non-orthogonality)

                FvScheme::getMeshStencil(i, j, k, EAST, sf, ds, alpha);
                gradUBar = alpha*gradUField_(i, j, k) + (1. - alpha)*gradUField_(i + 1, j, k);

                bP_(i, j, k) = dot(mu_*gradUBar, sf - ds*dot(sf, sf)/dot(sf, ds));

                FvScheme::getMeshStencil(i, j, k, WEST, sf, ds, alpha);
                gradUBar = alpha*gradUField_(i, j, k) + (1. - alpha)*gradUField_(i - 1, j, k);

                bP_(i, j, k) += dot(mu_*gradUBar, sf - ds*dot(sf, sf)/dot(sf, ds));

                FvScheme::getMeshStencil(i, j, k, NORTH, sf, ds, alpha);
                gradUBar = alpha*gradUField_(i, j, k) + (1. - alpha)*gradUField_(i, j + 1, k);

                bP_(i, j, k) += dot(mu_*gradUBar, sf - ds*dot(sf, sf)/dot(sf, ds));

                FvScheme::getMeshStencil(i, j, k, SOUTH, sf, ds, alpha);
                gradUBar = alpha*gradUField_(i, j, k) + (1. - alpha)*gradUField_(i, j - 1, k);

                bP_(i, j, k) += dot(mu_*gradUBar, sf - ds*dot(sf, sf)/dot(sf, ds));

                FvScheme::getMeshStencil(i, j, k, TOP, sf, ds, alpha);
                gradUBar = alpha*gradUField_(i, j, k) + (1. - alpha)*gradUField_(i, j, k + 1);

                bP_(i, j, k) += dot(mu_*gradUBar, sf - ds*dot(sf, sf)/dot(sf, ds));

                FvScheme::getMeshStencil(i, j, k, BOTTOM, sf, ds, alpha);
                gradUBar = alpha*gradUField_(i, j, k) + (1. - alpha)*gradUField_(i, j, k - 1);

                bP_(i, j, k) += dot(mu_*gradUBar, sf - ds*dot(sf, sf)/dot(sf, ds));

                // Compute the higher-order convection terms

                if(i < nCellsI_ - 1)
                    bP_(i, j, k) += -min(massFlow_.faceE(i, j, k), 0.)*dot(gradUField_(i + 1, j, k), mesh.rFaceW(i + 1, j, k)) - max(massFlow_.faceE(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceE(i, j, k));
                else
                    bP_(i, j, k) += -max(massFlow_.faceE(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceE(i, j, k));

                if(i > 0)
                    bP_(i, j, k) += max(massFlow_.faceW(i, j, k), 0.)*dot(gradUField_(i - 1, j, k), mesh.rFaceE(i - 1, j, k)) + min(massFlow_.faceW(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceW(i, j, k));
                else
                    bP_(i, j, k) += min(massFlow_.faceW(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceW(i, j, k));

                if(j < nCellsJ_ - 1)
                    bP_(i, j, k) += -min(massFlow_.faceN(i, j, k), 0.)*dot(gradUField_(i, j + 1, k), mesh.rFaceS(i, j + 1, k)) - max(massFlow_.faceN(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceN(i, j, k));
                else
                    bP_(i, j, k) += -max(massFlow_.faceN(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceN(i, j, k));

                if(j > 0)
                    bP_(i, j, k) += max(massFlow_.faceS(i, j, k), 0.)*dot(gradUField_(i, j - 1, k), mesh.rFaceN(i, j - 1, k)) + min(massFlow_.faceS(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceS(i, j, k));
                else
                    bP_(i, j, k) += min(massFlow_.faceS(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceS(i, j, k));

                if(k < nCellsK_ - 1)
                    bP_(i, j, k) += -min(massFlow_.faceT(i, j, k), 0.)*dot(gradUField_(i, j, k + 1), mesh.rFaceB(i, j, k + 1)) - max(massFlow_.faceT(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceT(i, j, k));
                else
                    bP_(i, j, k) += -max(massFlow_.faceT(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceT(i, j, k));

                if(k > 0)
                    bP_(i, j, k) += max(massFlow_.faceB(i, j, k), 0.)*dot(gradUField_(i, j, k - 1), mesh.rFaceT(i, j, k - 1)) + min(massFlow_.faceB(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceB(i, j, k));
                else
                    bP_(i, j, k) += min(massFlow_.faceB(i, j, k), 0.)*dot(gradUField_(i, j, k), mesh.rFaceB(i, j, k));

                bP_(i, j, k) += -gradPField_(i, j, k)*mesh.cellVol(i, j, k);

                // Update D-field

                dField_(i, j, k) = mesh.cellVol(i, j, k)/aP_(i, j, k);
            }
        }
    }

    for(itrNo = 0; itrNo < maxGsIters_; ++itrNo)
    {
        uField.setBoundaryFields();

        for(k = 0; k < nCellsK_; ++k)
        {
            for(j = 0; j < nCellsJ_; ++j)
            {
                for(i = 0; i < nCellsI_; ++i)
                {
                    for(l = 0; l < 3; ++l)
                    {
                        uField(i, j, k)(l) = -(aE_(i, j, k)*uField(i + 1, j, k)(l) + aW_(i, j, k)*uField(i - 1, j, k)(l)
                                               + aN_(i, j, k)*uField(i, j + 1, k)(l) + aS_(i, j, k)*uField(i, j - 1, k)(l)
                                               + aT_(i, j, k)*uField(i, j, k + 1)(l) + aB_(i, j, k)*uField(i, j, k - 1)(l)
                                               - bP_(i, j, k)(l))/aP_(i, j, k);
                    }
                }
            }
        }
    }
}

void Simple::computePCorr(Field<Vector3D>& uField, Field<double>& dField)
{
    int i, j, k, itrNo;
    HexaFvmMesh& mesh = *meshPtr_;
    Vector3D sf, ds;

    uField.setBoundaryFields();
    FvScheme::interpolateInteriorFaces(uField);
    computeMassFlow(uField);

    FvScheme::interpolateInteriorFaces(dField);

    Output::print("Simple", "Computing pressure corrections.");

    for(k = 0; k < nCellsK_; ++k)
    {
        for(j = 0; j < nCellsJ_; ++j)
        {
            for(i = 0; i < nCellsI_; ++i)
            {
                aE_(i, j, k) = -dot(rho_*dField_.faceE(i, j, k)*mesh.fAreaNormE(i, j, k), mesh.fAreaNormE(i, j, k))/dot(mesh.fAreaNormE(i, j, k), mesh.rCellE(i, j, k));
                aW_(i, j, k) = -dot(rho_*dField_.faceW(i, j, k)*mesh.fAreaNormW(i, j, k), mesh.fAreaNormW(i, j, k))/dot(mesh.fAreaNormW(i, j, k), mesh.rCellW(i, j, k));
                aN_(i, j, k) = -dot(rho_*dField_.faceN(i, j, k)*mesh.fAreaNormN(i, j, k), mesh.fAreaNormN(i, j, k))/dot(mesh.fAreaNormN(i, j, k), mesh.rCellN(i, j, k));
                aS_(i, j, k) = -dot(rho_*dField_.faceS(i, j, k)*mesh.fAreaNormS(i, j, k), mesh.fAreaNormS(i, j, k))/dot(mesh.fAreaNormS(i, j, k), mesh.rCellS(i, j, k));
                aT_(i, j, k) = -dot(rho_*dField_.faceT(i, j, k)*mesh.fAreaNormT(i, j, k), mesh.fAreaNormT(i, j, k))/dot(mesh.fAreaNormT(i, j, k), mesh.rCellT(i, j, k));
                aB_(i, j, k) = -dot(rho_*dField_.faceB(i, j, k)*mesh.fAreaNormB(i, j, k), mesh.fAreaNormB(i, j, k))/dot(mesh.fAreaNormB(i, j, k), mesh.rCellB(i, j, k));
                aP_(i, j, k) = -(aE_(i, j, k) + aW_(i, j, k) + aN_(i, j, k) + aS_(i, j, k) + aT_(i, j, k) + aB_(i, j, k));

                massFlow_(i, j, k) = massFlow_.faceW(i, j, k) - massFlow_.faceE(i, j, k)
                        + massFlow_.faceS(i, j, k) - massFlow_.faceN(i, j, k)
                        + massFlow_.faceB(i, j, k) - massFlow_.faceT(i, j, k);
            }
        }
    }

    //- These are for the cross-diffusion terms, and hence are only computed once

    computeCellCenteredGradients(pCorr_, gradPCorr_);
    computeFaceCenteredGradients(pCorr_, gradPCorr_);

    for(k = 0; k < nCellsK_; ++k)
    {
        for(j = 0; j < nCellsJ_; ++j)
        {
            for(i = 0; i < nCellsI_; ++i)
            {
                bP_(i, j, k).x = -massFlow_(i, j, k);

                sf = mesh.fAreaNormE(i, j, k);
                ds = mesh.rCellE(i, j, k);

                bP_(i, j, k).x += dot(rho_*dField_.faceE(i, j, k)*gradPCorr_.faceE(i, j, k), sf - ds*dot(sf, sf)/dot(sf, ds));

                sf = mesh.fAreaNormW(i, j, k);
                ds = mesh.rCellW(i, j, k);

                bP_(i, j, k).x +=  dot(rho_*dField_.faceW(i, j, k)*gradPCorr_.faceW(i, j, k), sf - ds*dot(sf, sf)/dot(sf, ds));

                sf = mesh.fAreaNormN(i, j, k);
                ds = mesh.rCellN(i, j, k);

                bP_(i, j, k).x +=  dot(rho_*dField_.faceN(i, j, k)*gradPCorr_.faceN(i, j, k), sf - ds*dot(sf, sf)/dot(sf, ds));

                sf = mesh.fAreaNormS(i, j, k);
                ds = mesh.rCellS(i, j, k);

                bP_(i, j, k).x +=  dot(rho_*dField_.faceS(i, j, k)*gradPCorr_.faceS(i, j, k), sf - ds*dot(sf, sf)/dot(sf, ds));

                sf = mesh.fAreaNormT(i, j, k);
                ds = mesh.rCellT(i, j, k);

                bP_(i, j, k).x +=  dot(rho_*dField_.faceT(i, j, k)*gradPCorr_.faceT(i, j, k), sf - ds*dot(sf, sf)/dot(sf, ds));

                sf = mesh.fAreaNormB(i, j, k);
                ds = mesh.rCellB(i, j, k);

                bP_(i, j, k).x +=  dot(rho_*dField_.faceB(i, j, k)*gradPCorr_.faceB(i, j, k), sf - ds*dot(sf, sf)/dot(sf, ds));
            }
        }
    }

    //- iterate using gauss-seidel

    for(itrNo = 0; itrNo < maxGsIters_; ++itrNo)
    {
        for(k = 0; k < nCellsK_; ++k)
        {
            for(j = 0; j < nCellsJ_; ++j)
            {
                for(i = 0; i < nCellsI_; ++i)
                {
                    pCorr_(i, j, k) = -(aE_(i, j, k)*pCorr_(i + 1, j, k) + aW_(i, j, k)*pCorr_(i - 1, j, k)
                                        + aN_(i, j, k)*pCorr_(i, j + 1, k) + aS_(i, j, k)*pCorr_(i, j - 1, k)
                                        + aT_(i, j, k)*pCorr_(i, j, k + 1) + aB_(i, j, k)*pCorr_(i, j, k - 1)
                                        - bP_(i, j, k).x)/aP_(i, j, k);
                }
            }
        }
    }
}

void Simple::correctPressure(Field<double> &pCorrField, Field<double> &pField)
{
    int i, j, k;

    for(k = 0; k < nCellsK_; ++k)
    {
        for(j = 0; j < nCellsJ_; ++j)
        {
            for(i = 0; i < nCellsI_; ++i)
            {
                pField(i, j, k) += relaxationFactor_*pCorrField(i, j, k);
            }
        }
    }
}

void Simple::correctVelocity(Field<Vector3D> &uField, Field<double> &dField, Field<double> &pCorrField)
{
    int i, j, k;

    for(k = 0; k < nCellsK_; ++k)
    {
        for(j = 0; j < nCellsJ_; ++j)
        {
            for(i = 0; i < nCellsI_; ++i)
            {

            }
        }
    }
}

// ************* Public Methods *************

void Simple::initialize(Input &input, HexaFvmMesh &mesh)
{
    FvScheme::initialize(input, mesh, "NA");
    uFieldPtr_ = &mesh.findVectorField("u");
    pFieldPtr_ = &mesh.findScalarField("p");

    relaxationFactor_ = input.inputDoubles["relaxationFactor"];
    rho_ = input.inputDoubles["rho"];
    mu_ = input.inputDoubles["mu"];
    maxGsIters_ = input.inputInts["maxGsIters"];
    nu_ = mu_/rho_;

    aP_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    aE_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    aW_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    aN_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    aS_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    aT_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    aB_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    bP_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    massFlow_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    pCorr_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    gradPCorr_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    dField_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    gradUField_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    gradPField_.allocate(nCellsI_, nCellsJ_, nCellsK_);

    pCorr_.setAllBoundaries(FIXED, 0.,
                            FIXED, 0.,
                            FIXED, 0.,
                            FIXED, 0.,
                            FIXED, 0.,
                            FIXED, 0.);

    A_.initialize(input);
    A_.allocate(nConservedVariables(), nConservedVariables());
    b_.allocate(nConservedVariables());
    x_.allocate(nConservedVariables());
}

int Simple::nConservedVariables()
{
    return 4*meshPtr_->size();
}

void Simple::discretize(std::vector<double> &timeDerivatives_)
{
    Field<Vector3D>& uField = *uFieldPtr_;
    Field<double>& pField = *pFieldPtr_;

    //- Compute the predicted momentum

    computeMomentum(uField, pField);

    //- Compute the pressure corrections

    computePCorr(uField, dField_);

    //- Apply the pressure and velocity corrections

    correctPressure(pField, pCorr_);
    correctVelocity(uField, dField_, pCorr_);
}

void Simple::copySolution(std::vector<double> &original)
{

}

void Simple::updateSolution(std::vector<double> &update, int method)
{

}
