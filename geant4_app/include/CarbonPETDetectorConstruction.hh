#ifndef CarbonPETDetectorConstruction_h
#define CarbonPETDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

class CarbonPETDetectorConstruction : public G4VUserDetectorConstruction {
public:
  CarbonPETDetectorConstruction();
  virtual ~CarbonPETDetectorConstruction();
  virtual G4VPhysicalVolume* Construct();
  virtual void ConstructSDandField();

  void SetTargetSize(G4double x, G4double y, G4double z);
  void SetVoxelSize(G4double doseVoxel, G4double activityVoxel);

private:
  G4double fTargetX, fTargetY, fTargetZ;
  G4double fDoseVoxelSize;
  G4double fActivityVoxelSize;
};

#endif
