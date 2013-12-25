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


/** \file src/DensityMatrix.h
** \brief A storage of the matrix elements of the hamiltonian in Fock basis, provides eigenvalues and eigenfunctions
** 
** \author Andrey Antipov(Andrey.E.Antipov@gmail.com)
** \author Igor Krivenko (Igor.S.Krivenko@gmail.com)
*/

#ifndef __INCLUDE_MPISKEL_H
#define __INCLUDE_MPISKEL_H

#include <boost/mpi.hpp>
#include "Misc.h"

namespace Pomerol {
namespace MPI {

template <typename PartType>
struct ComputeWrap {
    PartType *x;
    ComputeWrap(PartType &y):x(&y){};
    ComputeWrap() = default;
    ~ComputeWrap() = default;
    void run(){x->compute();}; 
    template <typename T = decltype(std::declval<PartType>().getSize()), typename std::enable_if<std::is_convertible<T,int>::value,bool>::type = 0> 
        int getSize(){return x->getSize();};
    template <typename T = decltype(std::declval<PartType>().getSize()), typename std::enable_if<!std::is_convertible<T,int>::value,bool>::type = 0> 
        int getSize(){return 0;};
};

template <typename PartType>
struct PrepareWrap {
    PartType *x;
    PrepareWrap(PartType &y):x(&y){};
    PrepareWrap() = default;
    ~PrepareWrap() = default;
    void run(){x->prepare();}; 
    template <typename T = decltype(std::declval<PartType>().getSize()), typename std::enable_if<std::is_convertible<T,int>::value,bool>::type = 0> 
        int getSize(){return x->getSize();};
    template <typename T = decltype(std::declval<PartType>().getSize()), typename std::enable_if<!std::is_convertible<T,int>::value,bool>::type = 0> 
        int getSize(){return 0;};
};


template <typename WrapType>
struct MPISkel {
    std::vector<WrapType> parts;
    std::map<MPI::JobId, MPI::WorkerId> run(const boost::mpi::communicator& comm, bool VerboseOutput = true);
};

template <typename WrapType>
std::map<Pomerol::MPI::JobId, Pomerol::MPI::WorkerId> MPISkel<WrapType>::run(const boost::mpi::communicator& comm, bool VerboseOutput)
{
    int rank = comm.rank();
    int comm_size = comm.size(); 
    comm.barrier();
    if (rank==0) { INFO("Calculating " << parts.size() << " jobs using " << comm_size << " procs."); };

    size_t ROOT = 0;
    std::unique_ptr<MPI::MPIMaster> disp;

    if (comm.rank() == ROOT) { 
        // prepare one Master on a root process for distributing parts.size() jobs
        std::vector<MPI::JobId> job_order(parts.size());
        for (size_t i=0; i<job_order.size(); i++) job_order[i] = i;
    //    for (size_t i=0; i<job_order.size(); i++) std::cout << job_order[i] << " " << std::flush; std::cout << std::endl; // DEBUG
    //    for (size_t i=0; i<job_order.size(); i++) std::cout << parts[job_order[i]].getSize() << " " << std::flush; std::cout << std::endl; // DEBUG
        std::sort(job_order.begin(), job_order.end(), [&](const int &l, const int &r){return (parts[l].getSize() > parts[r].getSize());});
    //    for (size_t i=0; i<job_order.size(); i++) std::cout << job_order[i] << " " << std::flush; std::cout << std::endl; // DEBUG
    //    for (size_t i=0; i<job_order.size(); i++) std::cout << parts[job_order[i]].getSize() << " " << std::flush; std::cout << std::endl; // DEBUG
        disp.reset(new MPI::MPIMaster(comm,job_order,true)); 
        disp->order();
    };
    comm.barrier();

    // Start calculating data
    for (MPI::MPIWorker worker(comm,ROOT);!worker.is_finished();) {
        if (rank == ROOT) disp->order(); 
        worker.receive_order(); 
        //DEBUG((worker.Status == WorkerTag::Pending));
        if (worker.is_working()) { // for a specific worker
            auto p = worker.current_job;
            if (VerboseOutput) std::cout << "["<<p+1<<"/"<<parts.size()<< "] P" << comm.rank() 
                                         << " : part " << p << " [" << parts[p].getSize() << "] run ... " << std::flush;
            parts[p].run(); 
            worker.report_job_done(); 
	        if (VerboseOutput) INFO("done.");
        };
        if (rank == ROOT) disp->check_workers(); // check if there are free workers 
    };
    // at this moment all communication is finished
    // Now spread the information, who did what.
    comm.barrier();
    std::map<MPI::JobId, MPI::WorkerId> job_map;
    if (rank == ROOT) { 
        job_map = disp -> DispatchMap; 
        std::vector<MPI::JobId> jobs(job_map.size());
        std::vector<MPI::WorkerId> workers(job_map.size());
        std::transform(job_map.begin(), job_map.end(), jobs.begin(), [](std::pair<MPI::JobId, MPI::WorkerId> x){return x.first;});
        boost::mpi::broadcast(comm, jobs, ROOT);
        std::transform(job_map.begin(), job_map.end(), workers.begin(), [](std::pair<MPI::JobId, MPI::WorkerId> x){return x.second;});
        boost::mpi::broadcast(comm, workers, ROOT);
        disp.release(); 
    } 
    else {
        std::vector<MPI::JobId> jobs(parts.size());
        boost::mpi::broadcast(comm, jobs, ROOT);
        std::vector<MPI::WorkerId> workers(parts.size());
        boost::mpi::broadcast(comm, workers, ROOT);
        for (size_t i=0; i<jobs.size(); i++) job_map[jobs[i]] = workers[i]; 
    }
    return job_map;
}

}; // end of namespace MPI
}; //

#endif
