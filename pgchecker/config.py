
import config_user as cfg
import os

from config_user import PG_BUILD_DIR, PG_INSTALL_DIR

DEPENDENCIES = [ "pgmpitunelib" ]

REPOS = {}

# pgtunelib
REPOS["pgmpitunelib"] = {}
REPOS["pgmpitunelib"]["REPO_GIT"]     = "https://github.com/hunsa/pgmpitunelib.git"
REPOS["pgmpitunelib"]["REPO_CHECKOUT_NAME"] = "pgmpitunelib"
REPOS["pgmpitunelib"]["REPO_SHA1"]    = "b7416e03612e5c1826646965628cf46398a32845"
REPOS["pgmpitunelib"]["CHECKOUT_DIR"] = os.path.join(PG_BUILD_DIR, REPOS['pgmpitunelib']['REPO_CHECKOUT_NAME'])
REPOS["pgmpitunelib"]["BUILD_DIR"]    = os.path.join(REPOS['pgmpitunelib']['CHECKOUT_DIR'], "build")
REPOS["pgmpitunelib"]["INSTALL_DIR"]  = os.path.join(REPOS['pgmpitunelib']['CHECKOUT_DIR'], "install")
REPOS["pgmpitunelib"]["BUILD_CMD"] = f"cmake -DCMAKE_BUILD_TYPE=Release -B {REPOS['pgmpitunelib']['BUILD_DIR']}/ -DCMAKE_INSTALL_PREFIX={REPOS['pgmpitunelib']['INSTALL_DIR']}; cmake --build {REPOS['pgmpitunelib']['BUILD_DIR']}/ --parallel; cmake --install {REPOS['pgmpitunelib']['BUILD_DIR']}/"


# pgchecker
PG_SOURCE_DIR  = os.path.join(os.path.dirname(__file__), "..")
PG_BUILD_CMD  = f"cmake -DCMAKE_BUILD_TYPE=Release -S {PG_SOURCE_DIR} -B {PG_BUILD_DIR} -DENABLE_PGCHECKER=ON -DPGTUNELIB_PATH={REPOS['pgmpitunelib']['INSTALL_DIR']} -DCMAKE_INSTALL_PREFIX={PG_INSTALL_DIR}; cmake --build {PG_BUILD_DIR} --parallel; cmake --install {PG_BUILD_DIR}"

PG_EXP_CONFIG_DIR = "config"
PG_EXP_DATA_DIR   = "data"


PG_SUPPORTED_COLLECTIVES = [
    ["MPI_Allgather", 0],
    ["MPI_Allreduce", 1],
    ["MPI_Alltoall", 0],
    ["MPI_Bcast", 1],
    ["MPI_Gather", 0], 
    ["MPI_Reduce", 1],
    ["MPI_Reduce_scatter_block", 1],
    ["MPI_Scan", 1],
    ["MPI_Scatter", 1]
]

PG_COLLECTIVES_EXP = []

for col in PG_SUPPORTED_COLLECTIVES:
    if col[1] == 0:
        msizes = "8,80,800,8000,80000"
    else:
        msizes = "8,80,800,8000,80000,800000,8000000"
    PG_COLLECTIVES_EXP.append(
        col[0] 
        + " --msizes-list=" + msizes 
        + " --nrep=5000 --proc-sync=roundtime --rt-bench-time-ms=3000 --rt-barrier-count=0"
    )




