#include "CarbonPETDetectorConstruction.hh"
#include "CarbonPETScoring.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4SDManager.hh"

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
  G4Material* pmma = nist->FindOrBuildMaterial("G4_PLEXIGLASS");

  // Target upstream face is at z = 0 and the block extends to z = fTargetZ.
  // The beam starts at z = -50 mm. The world must contain both; a box only
  // 1.2 times the target length leaves the downstream end outside the mother.
  const G4double margin = 50. * mm;
  const G4double halfX = 0.5 * fTargetX + margin;
  const G4double halfY = 0.5 * fTargetY + margin;
  const G4double halfZ = std::max(100. * mm, fTargetZ + margin);
  G4Box* solidWorld = new G4Box("World", halfX, halfY, halfZ);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, nist->FindOrBuildMaterial("G4_AIR"), "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0);

  G4Box* solidTarget = new G4Box("Target", fTargetX/2, fTargetY/2, fTargetZ/2);
  G4LogicalVolume* logicTarget = new G4LogicalVolume(solidTarget, pmma, "Target");
  new G4PVPlacement(0, G4ThreeVector(0, 0, fTargetZ/2), logicTarget, "Target", logicWorld, false, 0);

  logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
  return physWorld;
}

void CarbonPETDetectorConstruction::ConstructSDandField() {
  auto* scoring = new CarbonPETScoring("TargetScoring");
  scoring->SetVoxelSize(fDoseVoxelSize, fActivityVoxelSize);
  G4SDManager::GetSDMpointer()->AddNewDetector(scoring);
  SetSensitiveDetector("Target", scoring);
}
