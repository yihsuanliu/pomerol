//
// This file is a part of pomerol - a scientific ED code for obtaining 
// properties of a Hubbard model on a finite-size lattice 
//
// Copyright (C) 2010-2012 Andrey Antipov <Andrey.E.Antipov@gmail.com>
// Copyright (C) 2010-2012 Igor Krivenko <Igor.S.Krivenko@gmail.com>
//
// pomerol is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pomerol is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pomerol.  If not, see <http://www.gnu.org/licenses/>.


#include "FieldOperator.h"

namespace Pomerol{
FieldOperator::FieldOperator(const IndexClassification &IndexInfo, const StatesClassification &S, const Hamiltonian &H, ParticleIndex Index) :
    ComputableObject(Constructed), IndexInfo(IndexInfo), S(S), H(H), Index(Index)
{}

CreationOperator::CreationOperator(const IndexClassification &IndexInfo, const StatesClassification &S, const Hamiltonian &H, ParticleIndex Index) :
    FieldOperator(IndexInfo,S,H,Index)
{
    O = new Pomerol::OperatorPresets::Cdag(Index);
}

AnnihilationOperator::AnnihilationOperator(const IndexClassification &IndexInfo, const StatesClassification &S, const Hamiltonian &H, ParticleIndex Index) :
    FieldOperator(IndexInfo,S,H,Index)
{
    O = new Pomerol::OperatorPresets::C(Index);
}

const std::list<BlockMapping>& FieldOperator::getNonTrivialIndices() const
{
    if (Status < Computed) { ERROR("FieldOperator is not computed yet."); throw (exStatusMismatch()); }
    return LeftRightIndices;
}
 
const FieldOperatorPart& FieldOperator::getPartFromRightIndex(BlockNumber in) const
{
    if (Status < Computed) { ERROR("FieldOperator is not computed yet."); throw (exStatusMismatch()); }
    return *parts[mapPartsFromRight.find(in)->second];
}

const FieldOperatorPart& FieldOperator::getPartFromRightIndex(const QuantumNumbers& in) const
{
    if (Status < Computed) { ERROR("FieldOperator is not computed yet."); throw (exStatusMismatch()); }
    return *parts[mapPartsFromRight.find(S.getBlockNumber(in))->second];
}

const FieldOperatorPart& FieldOperator::getPartFromLeftIndex(BlockNumber in) const
{
    if (Status < Computed) { ERROR("FieldOperator is not computed yet."); throw (exStatusMismatch()); }
    return *parts[mapPartsFromLeft.find(in)->second];
}

const FieldOperatorPart& FieldOperator::getPartFromLeftIndex(const QuantumNumbers& in) const
{
    if (Status < Computed) { ERROR("FieldOperator is not computed yet."); throw (exStatusMismatch()); }
    return *parts[mapPartsFromLeft.find(S.getBlockNumber(in))->second];
}

void FieldOperator::compute(void)
{
    if (Status >= Computed) return;
    size_t Size = parts.size();
    INFO_NONEWLINE("Computing " << *O << " in eigenbasis of the Hamiltonian: ")
    for (size_t BlockIn = 0; BlockIn < Size; BlockIn++){
        INFO_NONEWLINE( (int) ((1.0*BlockIn/Size) * 100 ) << "  " << std::flush);
        parts[BlockIn]->compute();
    };
    INFO("");
    Status = Computed;
}

ParticleIndex FieldOperator::getIndex(void) const
{
    return Index;
}

void CreationOperator::prepare(void)
{
    if (Status >= Prepared) return;
    size_t Size = parts.size();
    for (BlockNumber RightIndex=0; RightIndex<S.NumberOfBlocks(); RightIndex++){
        BlockNumber LeftIndex = mapsTo(RightIndex);
        //DEBUG(RightIndex << "->" << LeftIndex);
        if (LeftIndex.isCorrect()){
            FieldOperatorPart *Part = new CreationOperatorPart(IndexInfo, S,
                                    H.getPart(RightIndex),H.getPart(LeftIndex),Index);
            parts.push_back(Part);
            mapPartsFromRight[RightIndex]=Size;
            mapPartsFromLeft[LeftIndex]=Size;
            LeftRightIndices.push_back(BlockMapping(LeftIndex,RightIndex));
            mapRightToLeftIndex[RightIndex]=LeftIndex;
            mapLeftToRightIndex[LeftIndex]=RightIndex;
            Size++;
        }
    }
    INFO("CreationOperator_" << Index <<": " << Size << " parts will be computed");
    LeftRightIndices.sort(); 
    Status = Prepared;
}

void AnnihilationOperator::prepare()
{
    if (Status >= Prepared) return;
    size_t Size = parts.size();
    for (BlockNumber RightIndex=0;RightIndex<S.NumberOfBlocks();RightIndex++){
        BlockNumber LeftIndex = mapsTo(RightIndex);
        if (LeftIndex.isCorrect()){
            FieldOperatorPart *Part = new AnnihilationOperatorPart(IndexInfo, S,
                                    H.getPart(RightIndex),H.getPart(LeftIndex), Index);
            parts.push_back(Part);
            mapPartsFromRight[RightIndex]=Size;
            mapPartsFromLeft[LeftIndex]=Size;
            mapRightToLeftIndex[RightIndex]=LeftIndex;
            mapLeftToRightIndex[LeftIndex]=RightIndex;
            LeftRightIndices.push_back(BlockMapping(LeftIndex,RightIndex));
            Size++;
        }
    }
    INFO("AnnihilationOperator_" << Index <<": " << Size << " parts will be computed");
    //LeftRightIndices.sort(); 
    Status = Prepared;
}

BlockNumber FieldOperator::getRightIndex(BlockNumber LeftIndex) const
{
    if (Status < Prepared) { ERROR("FieldOperator is not prepared yet."); throw (exStatusMismatch()); }
    return mapLeftToRightIndex.count(LeftIndex) ?
        mapLeftToRightIndex.find(LeftIndex)->second : ERROR_BLOCK_NUMBER;
}

BlockNumber FieldOperator::getLeftIndex(BlockNumber RightIndex) const
{
    if (Status < Prepared) { ERROR("FieldOperator is not prepared yet."); throw (exStatusMismatch()); }
    return mapRightToLeftIndex.count(RightIndex) ? 
        mapRightToLeftIndex.find(RightIndex)->second : ERROR_BLOCK_NUMBER;
}

BlockNumber FieldOperator::mapsTo(BlockNumber RightIndex) const
{
    bool found=false;
    std::map<FockState, MelemType> result;
    const std::vector<FockState> &states=S.getFockStates(RightIndex);
    for (std::vector<FockState>::const_iterator state_it=states.begin(); state_it!=states.end() && !found; state_it++) {
        result = O->actRight(*state_it);
        found = (result.size()>0);
        }
    return (found)?S.getBlockNumber(result.begin()->first):ERROR_BLOCK_NUMBER;
}

QuantumNumbers FieldOperator::mapsTo(const QuantumNumbers& in) const 
{
    BlockNumber out = this->mapsTo(S.getBlockNumber(in));
    if ( out == ERROR_BLOCK_NUMBER) throw (QuantumNumbers::exWrongNumbers());
    return S.getQuantumNumbers(out);
}

} // end of namespace Pomerol
