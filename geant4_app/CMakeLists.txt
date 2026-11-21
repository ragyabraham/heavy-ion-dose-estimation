cmake_minimum_required(VERSION 3.16)
project(CarbonPETSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

set(SOURCES
  src/main.cc
  src/CarbonPETDetectorConstruction.cc
  src/CarbonPETActionInitialization.cc
  src/CarbonPETPrimaryGeneratorAction.cc
  src/CarbonPETSteppingAction.cc
  src/CarbonPETScoring.cc
)

add_executable(carbon_pet_sim ${SOURCES})
target_include_directories(carbon_pet_sim PRIVATE include)
target_link_libraries(carbon_pet_sim ${Geant4_LIBRARIES})

install(TARGETS carbon_pet_sim DESTINATION bin)
