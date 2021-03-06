#include <algorithm>

#include "FvScheme.h"

template <class T>
void FvScheme::interpolateInteriorFaces(InterpolationMethod method, Field<T> &field)
{
    const HexaFvmMesh &mesh = field.getMesh();

    switch (method)
    {
    case VOLUME_WEIGHTED:

        for(int k = 0, nK = mesh.nCellsK(); k < nK; ++k)
        {
            for(int j = 0, nJ = mesh.nCellsJ(); j < nJ; ++j)
            {
                for(int i = 0, nI = mesh.nCellsI(); i < nI; ++i)
                {
                    if(i == 0 && mesh.westMeshExists())
                        field.faceW(i, j, k) = mesh.gW(i, j, k)*field(i, j, k) + (1. - mesh.gW(i, j, k))*field(i - 1, j, k);

                    if(j == 0 && mesh.southMeshExists())
                        field.faceS(i, j, k) = mesh.gS(i, j, k)*field(i, j, k) + (1. - mesh.gS(i, j, k))*field(i, j - 1, k);

                    if(k == 0 && mesh.bottomMeshExists())
                        field.faceB(i, j, k) = mesh.gB(i, j, k)*field(i, j, k) + (1. - mesh.gB(i, j, k))*field(i, j, k - 1);

                    if(i < mesh.uCellI() || mesh.eastMeshExists())
                        field.faceE(i, j, k) = mesh.gE(i, j, k)*field(i, j, k) + (1. - mesh.gE(i, j, k))*field(i + 1, j, k);

                    if(j < mesh.uCellJ() || mesh.northMeshExists())
                        field.faceN(i, j, k) = mesh.gN(i, j, k)*field(i, j, k) + (1. - mesh.gN(i, j, k))*field(i, j + 1, k);

                    if(k < mesh.uCellK() || mesh.topMeshExists())
                        field.faceT(i, j, k) = mesh.gT(i, j, k)*field(i, j, k) + (1. - mesh.gT(i, j, k))*field(i, j, k + 1);
                }
            }
        }
        break;

    case NON_WEIGHTED:

        for(int k = 0, nK = mesh.nCellsK(); k < nK; ++k)
        {
            for(int j = 0, nJ = mesh.nCellsJ(); j < nJ; ++j)
            {
                for(int i = 0, nI = mesh.nCellsI(); i < nI; ++i)
                {
                    if(i == 0 && mesh.westMeshExists())
                        field.faceW(i, j, k) = 0.5*(field(i, j, k) + field(i - 1, j, k));

                    if(j == 0 && mesh.southMeshExists())
                        field.faceS(i, j, k) = 0.5*(field(i, j, k) + field(i, j - 1, k));

                    if(k == 0 && mesh.bottomMeshExists())
                        field.faceB(i, j, k) = 0.5*(field(i, j, k) + field(i, j, k - 1));

                    if(i < mesh.uCellI() || mesh.eastMeshExists())
                        field.faceE(i, j, k) = 0.5*(field(i, j, k) + field(i + 1, j, k));

                    if(j < mesh.uCellJ() || mesh.northMeshExists())
                        field.faceN(i, j, k) = 0.5*(field(i, j, k) + field(i, j + 1, k));

                    if(k < mesh.uCellK() || mesh.topMeshExists())
                        field.faceT(i, j, k) = 0.5*(field(i, j, k) + field(i, j, k + 1));
                }
            }
        }

        break;
    };
}
