#!/usr/bin/python

# prevent mpi4py from calling MPI_Finalize()
import mpi4py.rc
mpi4py.rc.finalize   = False

from mpi4py import MPI
from skylark import cskylark

import kdt
import math

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

# Skylark is automatically initilalized when you import Skylark,
# It will use system time to generate the seed. However, we can 
# reinitialize for so to fix the seed.
cskylark.initialize(123834);

# creating an example matrix
# It seems that pySpParMat can only be created from dense vectors
rows = kdt.Vec(60, sparse=False)
cols = kdt.Vec(60, sparse=False)
vals = kdt.Vec(60, sparse=False)
for i in range(0, 60):
  rows[i] = math.floor(i / 6)
for i in range(0, 60):
  cols[i] = i % 6
for i in range(0, 60):
  vals[i] = i

ACB = kdt.Mat(rows, cols, vals, 6, 10)
print ACB

nullVec = kdt.Vec(0, sparse=False)
SACB    = kdt.Mat(nullVec, nullVec, nullVec, 6, 6)
S       = cskylark.CWT(10, 6, intype="DistSparseMatrix", outtype="DistSparseMatrix")
S.apply(ACB, SACB, 0)

if (rank == 0):
  print("Sketched A (CWT sparse, columnwise)")
  print SACB

S.free()

SACB = kdt.Mat(nullVec, nullVec, nullVec, 3, 10)
S    = cskylark.CWT(6, 3, intype="DistSparseMatrix", outtype="DistSparseMatrix")
S.apply(ACB, SACB, 1)

if (rank == 0):
  print("Sketched A (CWT sparse, rowwise)")
  print SACB

# As with all Python object they will be automatically garbage
# collected, and the associated memory will be freed.
# You can also explicitly free them.
del S     # S = 0 will also free memory.

# Really no need to close skylark -- it will do it automatically.
# However, if you really want to you can do it.
cskylark.finalize()
