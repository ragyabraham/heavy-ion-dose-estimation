#include "CarbonPETActionInitialization.hh"
#include "CarbonPETPrimaryGeneratorAction.hh"

void CarbonPETActionInitialization::BuildForMaster() const {}

void CarbonPETActionInitialization::Build() const {
  SetUserAction(new CarbonPETPrimaryGeneratorAction);
}
