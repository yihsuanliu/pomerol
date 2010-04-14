#ifndef ____DEFINE_HAMILTONIAN____
#define ____DEFINE_HAMILTONIAN____
#include "config.h"
#include "BitClassification.h"
#include "StatesClassification.h"
#include "HamiltonianPart.h"
#include "output.h"
#include <vector>

class Hamiltonian
{
  HamiltonianPart **Hpart;


  BitClassification &Formula;
  StatesClassification& S;
  output_handle &OUT;
  string config_path;

public :

  Hamiltonian(BitClassification &F_, StatesClassification &S_,output_handle &OUT_, string &config_path_);
  void enter(bool diag=false,bool dump=false);

  HamiltonianPart& block(const QuantumNumbers &in);
  HamiltonianPart& block(BlockNumber in);
  RealType eigenval( QuantumState &state );

  void diagonalize();
  void dump();
};

#endif // endif :: #ifndef ____DEFINE_HAMILTONIAN____
