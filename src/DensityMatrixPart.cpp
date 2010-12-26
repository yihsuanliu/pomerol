/** \file src/DensityMatrixPart.cpp
** \brief Part (diagonal block) of a density matrix.
**
** \author Igor Krivenko (igor@shg.ru)
** \author Andrey Antipov (antipov@ct-qmc.org)
*/
#include "DensityMatrixPart.h"

DensityMatrixPart::DensityMatrixPart(HamiltonianPart& hpart, RealType beta, RealType GroundEnergy) :
    hpart(hpart), beta(beta), GroundEnergy(GroundEnergy)
{
    partSize = hpart.size();
    weights.resize(partSize);
}

RealType DensityMatrixPart::compute(void)
{
    Z_part = 0;
    for(QuantumState m = 0; m < partSize; ++m){
        // The non-normalized weight is <=1 for any state.
        weights(m) = exp(-beta*(hpart.reV(m)-GroundEnergy));
        Z_part += weights(m);
    }

    return Z_part;
}

void DensityMatrixPart::normalize(RealType Z)
{
    weights /= Z;
    Z_part /= Z;
}

RealType DensityMatrixPart::getAverageEnergy()
{
    RealType E=0.;
    for(QuantumState m = 0; m < partSize; ++m){
        E += weights(m)*hpart.reV(m);
    }
    return E;
};

RealType DensityMatrixPart::weight(int m)
{
    return weights(m);
}

RealType DensityMatrixPart::getBeta(void)
{
    return beta;
}
