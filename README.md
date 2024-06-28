# QuickPool

This directory contains the implementation of the intersection based and end-point based matching protocols, proposed in QuickPool.
The protocols are implemented in C++17 and [CMake](https://cmake.org/) is used as the build system.

## External Dependencies
The following libraries need to be installed separately and should be available to the build system and compiler.

- [GMP](https://gmplib.org/)
- [NTL](https://www.shoup.net/ntl/) (11.0.0 or later)
- [Boost](https://www.boost.org/) (1.72.0 or later)
- [Nlohmann JSON](https://github.com/nlohmann/json)
- [EMP Tool](https://github.com/emp-toolkit/emp-tool)

### Docker
All required dependencies to compile and run the project are available through the docker image.
To build and run the docker image, execute the following commands from the root directory of the repository:

```sh
# Build the QuickPool Docker image.
#
# Building the Docker image requires at least 4GB RAM. This needs to be set 
# explicitly in case of Windows and MacOS.
docker build -t quickpool .

# Create and run a container.
#
# This should start the shell from within the container.
docker run -it -v $PWD:/code quickpool

# The following command changes the working directory to the one containing the 
# source code and should be run on the shell started using the previous command.
cd /code
```

## Compilation
The project uses [CMake](https://cmake.org/) for building the source code. 
To compile, run the following commands from the root directory of the repository:

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# The two main targets are 'benchmarks' and 'tests' corresponding to
# binaries used to run benchmarks and unit tests respectively.
make <target>
```

## Usage
A short description of the compiled programs is given below.
All of them provide detailed usage description on using the `--help` option.

- `benchmarks/quickpool_intersection`: Benchmark the performance of the intersection based matching protocol of QuickPool for a given number of riders and drivers.
- `benchmarks/quickpool_ED`: Benchmark the performance of the end-point based matching protocol of QuickPool for a given number of riders and drivers.
- `tests/*`: These programs contain unit tests for various parts of the codebase. 

Execute the following commands from the `build` directory created during compilation to run the programs:
```sh
# Benchmark QuickPool.
#
# The command below should be run on n+1 different terminals, where 
# n = #riders + #drivers with $PID set to 0, 1, 2, and upto n 
# i.e., one instance corresponding to each party.
#
# '-r' and '-d' denotes the numbers of riders and drivers respectively.
#
# The program can be run on different machines by replacing the `--localhost`
# option with '--net-config <net_config.json>' where 'net_config.json' is a
# JSON file containing the IPs of the parties. A template is given in the
# repository root.

./benchmarks/quickpool_intersection -p $PID --localhost -r 40 -d 40
# or
./benchmarks/quickpool_ED -p $PID --localhost -r 40 -d 40

# The `run_quickpool.sh` script in the repository root can be used to run the programs 
# for all parties from the same terminal.
# For example, the previous benchmarks can be run using the script as shown below.

./../run_quickpool.sh 40 40 1 # for `quickpool_intersection`
#or
./../run_quickpool.sh 40 40 0 # for `quickpool_ED`
```
