#include "CarbonPETDetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

CarbonPETDetectorConstruction::CarbonPETDetectorConstruction()
  : fTargetX(100*mm), fTargetY(100*mm), fTargetZ(300*mm),
    fDoseVoxelSize(1.0*mm), fActivityVoxelSize(1.5*mm) {}

CarbonPETDetectorConstruction::~CarbonPETDetectorConstruction() {}

void CarbonPETDetectorConstruction::SetTargetSize(G4double x, G4double y, G4double z) {
  fTargetX = x; fTargetY = y; fTargetZ = z;
}

void CarbonPETDetectorConstruction::SetVoxelSize(G4double doseVoxel, G4double activityVoxel) {
  fDoseVoxelSize = doseVoxel;
  fActivityVoxelSize = activityVoxel;
}

G4VPhysicalVolume* CarbonPETDetectorConstruction::Construct() {
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* pmma = nist->FindOrBuildMaterial("G4_PLEXIGLASS");  // PMMA

  // World
  G4double worldSize = 1.2 * std::max({fTargetX, fTargetY, fTargetZ});
  G4Box* solidWorld = new G4Box("World", worldSize/2, worldSize/2, worldSize/2);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, nist->FindOrBuildMaterial("G4_AIR"), "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0);

  // Target (PMMA block)
  G4Box* solidTarget = new G4Box("Target", fTargetX/2, fTargetY/2, fTargetZ/2);
  G4LogicalVolume* logicTarget = new G4LogicalVolume(solidTarget, pmma, "Target");
  new G4PVPlacement(0, G4ThreeVector(0, 0, fTargetZ/2), logicTarget, "Target", logicWorld, false, 0);

  // Scoring volumes would be attached here via G4MultiFunctionalDetector
  // (implemented in full version with dose and isotope-specific scorers)

  logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
  return physWorld;
}
