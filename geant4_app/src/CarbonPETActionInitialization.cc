#include "CarbonPETActionInitialization.hh"
#include "CarbonPETPrimaryGeneratorAction.hh"
#include "CarbonPETSteppingAction.hh"

void CarbonPETActionInitialization::BuildForMaster() const {}

void CarbonPETActionInitialization::Build() const {
  SetUserAction(new CarbonPETPrimaryGeneratorAction);
  SetUserAction(new CarbonPETSteppingAction);
}
