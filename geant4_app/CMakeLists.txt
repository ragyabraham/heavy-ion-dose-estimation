cmake_minimum_required(VERSION 3.16)
project(CarbonPETSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Geant4 (required)
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

# Optional: pybind11 for Python bindings
find_package(pybind11 QUIET)

# Sources
set(SOURCES
  src/CarbonPETActionInitialization.cc
  src/CarbonPETDetectorConstruction.cc
  src/CarbonPETPrimaryGeneratorAction.cc
  src/CarbonPETRunAction.cc
  src/CarbonPETEventAction.cc
  src/CarbonPETSteppingAction.cc
  src/main.cc
)

add_executable(carbon_pet_sim ${SOURCES})
target_include_directories(carbon_pet_sim PRIVATE include)
target_link_libraries(carbon_pet_sim ${Geant4_LIBRARIES})

# Install
install(TARGETS carbon_pet_sim DESTINATION bin)

# Python module (if pybind11 available)
if(pybind11_FOUND)
  pybind11_add_module(carbon_pet_py python/bindings.cpp)
  target_link_libraries(carbon_pet_py PRIVATE carbon_pet_sim)
endif()
