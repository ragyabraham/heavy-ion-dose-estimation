#ifndef CarbonPETSteppingAction_h
#define CarbonPETSteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"
#include <map>

class CarbonPETSteppingAction : public G4UserSteppingAction {
public:
  CarbonPETSteppingAction();
  virtual ~CarbonPETSteppingAction();
  virtual void UserSteppingAction(const G4Step*);

  void WriteResults(const G4String& filename);

private:
  G4int GetDoseIndex(const G4ThreeVector& pos) const;
  std::map<G4int, G4double> fDoseMap;
  G4double fDoseVoxel;
  G4double fTargetHalfZ;
};

#endif
