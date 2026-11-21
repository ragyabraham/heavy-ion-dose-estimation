#include "CarbonPETScoring.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include <fstream>
#include <cmath>

CarbonPETScoring::CarbonPETScoring(const G4String& name)
  : G4VSensitiveDetector(name),
    fDoseVoxel(1.0*mm),
    fActVoxel(1.5*mm),
    fTargetHalfZ(150*mm) {}

CarbonPETScoring::~CarbonPETScoring() {}

void CarbonPETScoring::SetVoxelSize(G4double doseVox, G4double actVox) {
  fDoseVoxel = doseVox;
  fActVoxel = actVox;
}

void CarbonPETScoring::Initialize(G4HCofThisEvent*) {
  fDoseMap.clear();
  fActivityMap.clear();
}

G4int CarbonPETScoring::GetDoseIndex(const G4ThreeVector& pos) {
  G4int ix = static_cast<G4int>(std::floor(pos.x() / fDoseVoxel));
  G4int iy = static_cast<G4int>(std::floor(pos.y() / fDoseVoxel));
  G4int iz = static_cast<G4int>(std::floor((pos.z() + fTargetHalfZ) / fDoseVoxel));
  return (iz * 10000 + iy * 100 + ix);
}

G4int CarbonPETScoring::GetActivityIndex(const G4ThreeVector& pos) {
  G4int ix = static_cast<G4int>(std::floor(pos.x() / fActVoxel));
  G4int iy = static_cast<G4int>(std::floor(pos.y() / fActVoxel));
  G4int iz = static_cast<G4int>(std::floor((pos.z() + fTargetHalfZ) / fActVoxel));
  return (iz * 10000 + iy * 100 + ix);
}

G4bool CarbonPETScoring::ProcessHits(G4Step* step, G4TouchableHistory*) {
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep > 0.) {
    G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();
    G4int idx = GetDoseIndex(pos);
    fDoseMap[idx] += edep;
  }

  const G4Track* track = step->GetTrack();
  if (track->GetTrackStatus() == fStopAndKill) {
    G4String name = track->GetParticleDefinition()->GetParticleName();
    if (name == "C11" || name == "C10" || name == "O15" || name == "N13") {
      G4ThreeVector pos = track->GetPosition();
      G4int idx = GetActivityIndex(pos);
      fActivityMap[idx] += 1.0;
    }
  }
  return true;
}

void CarbonPETScoring::EndOfEvent(G4HCofThisEvent*) {}

void CarbonPETScoring::WriteResults(const G4String& filename) {
  std::ofstream out(filename);
  out << "# dose_index dose_MeV\n";
  for (const auto& p : fDoseMap) {
    out << p.first << " " << p.second / MeV << "\n";
  }
  out << "# activity_index counts\n";
  for (const auto& p : fActivityMap) {
    out << p.first << " " << p.second << "\n";
  }
  out.close();
  G4cout << "Wrote scoring results to " << filename << G4endl;
}
