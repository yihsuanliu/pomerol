#ifndef __OPERATOR_CONTAINERS__
#define __OPERATOR_CONTAINERS__

#include "config.h"
#include "output.h"
#include "StatesClassification.h"
#include "HamiltonianPart.h"
#include "Hamiltonian.h" 
#include "FieldOperatorPart.h"

class OperatorContainer
{
protected:
	StatesClassification &System;
	Hamiltonian &H;
	output_handle OUT;

	int bit;
	FieldOperatorPart **Data;
	//vector<FieldOperatorPart*> MappedData;
	std::map<unsigned int,BlockNumber> mapNontrivialBlocks;
	unsigned int size;

	virtual	BlockNumber where(BlockNumber in)=0;
	virtual	QuantumNumbers where(QuantumNumbers in)=0;

public:
	OperatorContainer(StatesClassification &System_, Hamiltonian &H_, output_handle &OUT_, int bit_):System(System_),H(H_),OUT(OUT_),bit(bit_){size=0;};	


	FieldOperatorPart& part(BlockNumber in);
	FieldOperatorPart& part(QuantumNumbers in);

	void compute();
	void dump();
	void print_to_screen();
};

class CreationOperator : public OperatorContainer
{
public:
	void prepare();
	BlockNumber where(BlockNumber in);
	QuantumNumbers where(QuantumNumbers in);

	CreationOperator(StatesClassification &System_, Hamiltonian &H_, output_handle &OUT_, int bit_):OperatorContainer(System_,H_,OUT_,bit_){};
};

class AnnihilationOperator : public OperatorContainer
{
public:
	AnnihilationOperator(StatesClassification &System_, Hamiltonian &H_, output_handle &OUT_, int bit_):OperatorContainer(System_,H_,OUT_,bit_){};
};

#endif