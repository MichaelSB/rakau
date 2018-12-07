#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
    wget https://repo.continuum.io/miniconda/Miniconda2-latest-MacOSX-x86_64.sh -O miniconda.sh;
else
    wget https://repo.continuum.io/miniconda/Miniconda2-latest-Linux-x86_64.sh -O miniconda.sh;
fi
export deps_dir=$HOME/local
export PATH="$HOME/miniconda/bin:$PATH"
bash miniconda.sh -b -p $HOME/miniconda
conda config --add channels conda-forge --force

conda_pkgs="cmake>=3.3 boost-cpp xsimd tbb tbb-devel"

conda create -q -p $deps_dir -y $conda_pkgs
source activate $deps_dir

set +e
set +x
