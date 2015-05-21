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

#include "MultiphaseSimple.h"

MultiphaseSimple::MultiphaseSimple()
    :
      interfaceNormals_("interfaceNormals", AUXILLARY),
      kField_("k", AUXILLARY)
{

}

void MultiphaseSimple::initialize(Input &input, HexaFvmMesh& mesh)
{
    Simple::initialize(input, mesh);

    alphaFieldPtr_ = &mesh.findScalarField("alpha");

    interfaceNormals_.allocate(nCellsI_, nCellsJ_, nCellsK_);
    kField_.allocate(nCellsI_, nCellsJ_, nCellsK_);
}
