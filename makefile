SHELL = /bin/sh

## Set-up the compiler flags

CC_FLAGS = -O3 -Wall -std=c++11

CXX_FLAGS = $(CC_FLAGS)

## Modules

ADVEC_DIFF = caffeAdvectionDiffusion
MODULES += $(ADVEC_DIFF)

## Directories

BUILD_DIR = Build
MODULES_DIR = Modules
MATH_DIR = src/Math
DATA_STRUCTURES_DIR = src/DataStructures
RUN_CONTROL_DIR = src/RunControl
DOMAIN_DIR = src/Domains
FIELD_DIR = src/Fields
MESH_DIR = src/Domains/Meshes
SOLVER_DIR = src/Solvers
SCHEME_DIR = src/Schemes

ALL_DIRS = $(MODULES_DIR) $(MATH_DIR) $(DATA_STRUCTURES_DIR) $(RUN_CONTROL_DIR) $(DOMAIN_DIR) \
$(MESH_DIR) $(FIELD_DIR) $(SOLVER_DIR) $(SCHEME_DIR)

## Includes

INCLUDE = $(addprefix -I./, $(ALL_DIRS))

## External libraries

## Source files

# Math

MATH_SRC_FILES += Vector3D.cc
MATH_SRC_FILES += Tensor3D.cc

MATH_SRC = $(addprefix $(MATH_DIR)/, $(MATH_SRC_FILES))

# Run control

RUN_CONTROL_SRC_FILES += RunControl.cc
RUN_CONTROL_SRC_FILES += ArgsList.cc
RUN_CONTROL_SRC_FILES += Input.cc

RUN_CONTROL_SRC = $(addprefix $(RUN_CONTROL_DIR)/, $(RUN_CONTROL_SRC_FILES))

# Fields

FIELD_SRC = $(addprefix $(FIELD_DIR)/, $(FIELD_SRC_FILES))

# Domains

DOMAIN_SRC_FILES += DomainInterface.cc

DOMAIN_SRC = $(addprefix $(DOMAIN_DIR)/, $(DOMAIN_SRC_FILES))

# Meshes

MESH_SRC_FILES += PrimitiveMesh.cc
MESH_SRC_FILES += HexaFdmMesh.cc

MESH_SRC = $(addprefix $(MESH_DIR)/, $(MESH_SRC_FILES))

# Solvers

SOLVER_SRC_FILES += Euler.cc

SOLVER_SRC = $(addprefix $(SOLVER_DIR)/, $(SOLVER_SRC_FILES))

# Schemes

SCHEME_SRC_FILES += FiniteDifference.cc

SCHEME_SRC = $(addprefix $(SCHEME_DIR)/, $(SCHEME_SRC_FILES))

## Module Dependencies

# Advection Diffusion

ADVEC_DIFF_SRC += Modules/$(ADVEC_DIFF).cc
ADVEC_DIFF_SRC += $(MATH_SRC)
ADVEC_DIFF_SRC += $(RUN_CONTROL_SRC)
ADVEC_DIFF_SRC += $(DOMAIN_SRC)
ADVEC_DIFF_SRC += $(MESH_SRC)
ADVEC_DIFF_SRC += $(SOLVER_SRC)
ADVEC_DIFF_SRC += $(SCHEME_SRC)
ADVEC_DIFF_OBJS = $(ADVEC_DIFF_SRC:.cc=.o)

install: all

all: $(MODULES)

$(ADVEC_DIFF): $(ADVEC_DIFF_OBJS)
	$(CXX) $(INCLUDE) $(CXX_FLAGS) -o $(ADVEC_DIFF) $(ADVEC_DIFF_OBJS) -lboost_program_options -lboost_system -lboost_date_time -lboost_chrono
	mv $(ADVEC_DIFF) bin/

$(ADVEC_DIFF_OBJS):%.o: %.cc
	$(CXX) $(INCLUDE) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f $(addsuffix /*.o, $(ALL_DIRS))
	rm -f $(addsuffix /*~, $(ALL_DIRS))
