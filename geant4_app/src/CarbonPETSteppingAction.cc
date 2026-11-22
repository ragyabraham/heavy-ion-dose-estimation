#include "CarbonPETSteppingAction.hh"

#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include <fstream>
#include <cmath>
#include <atomic>

CarbonPETSteppingAction::CarbonPETSteppingAction()
  : fDoseVoxel(1.0*mm), fTargetHalfZ(150*mm) {}

CarbonPETSteppingAction::~CarbonPETSteppingAction() {}

G4int CarbonPETSteppingAction::GetDoseIndex(const G4ThreeVector& pos) const {
  G4int ix = static_cast<G4int>(std::floor(pos.x() / fDoseVoxel));
  G4int iy = static_cast<G4int>(std::floor(pos.y() / fDoseVoxel));
  G4int iz = static_cast<G4int>(std::floor((pos.z() + fTargetHalfZ) / fDoseVoxel));
  return iz * 10000 + iy * 100 + ix;
}

void CarbonPETSteppingAction::UserSteppingAction(const G4Step* step) {
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) return;

  auto* preVol = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
  if (!preVol || preVol->GetName() != "Target") return;

  G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();
  G4int idx = GetDoseIndex(pos);
  fDoseMap[idx] += edep;

  static std::atomic<G4int> printed{0};
  if (printed.fetch_add(1) < 3) {
    G4cout << "SteppingAction scored " << edep/MeV << " MeV at z = " << pos.z()/mm << " mm" << G4endl;
  }
}

void CarbonPETSteppingAction::WriteResults(const G4String& filename) {
  std::ofstream out(filename);
  out << "# dose_index dose_MeV\n";
  for (const auto& p : fDoseMap) {
    out << p.first << " " << p.second / MeV << "\n";
  }
  out.close();
  G4cout << "Wrote " << fDoseMap.size() << " dose voxels to " << filename << G4endl;
}
