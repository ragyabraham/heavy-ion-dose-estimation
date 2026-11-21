#include "CarbonPETScoring.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4GenericMessenger.hh"
#include <fstream>
#include <cmath>
#include <algorithm>

CarbonPETScoring::CarbonPETScoring(const G4String& name)
  : G4VSensitiveDetector(name),
    fDoseVoxel(1.0*mm),
    fActVoxel(1.5*mm),
    fTargetHalfZ(150*mm),
    fMessenger(nullptr),
    fOutputFile("scoring_results.txt") {
  fMessenger = new G4GenericMessenger(this, "/carbon/scoring/", "Scoring output");
  fMessenger->DeclareProperty(
      "file", fOutputFile,
      "File written at the end of each run. Use a name with no spaces.");
  fMessenger->DeclareMethod(
      "reset", &CarbonPETScoring::Reset,
      "Clear dose and activity accumulated so far");
}

CarbonPETScoring::~CarbonPETScoring() {
  delete fMessenger;
}

void CarbonPETScoring::Reset() {
  fDoseMap.clear();
  fActivityMap.clear();
  fCountedThisEvent.clear();
  G4cout << "Scoring maps reset. Next run will write " << fOutputFile << G4endl;
}

void CarbonPETScoring::SetVoxelSize(G4double doseVox, G4double actVox) {
  fDoseVoxel = doseVox;
  fActVoxel = actVox;
}

namespace {

bool IsPetEmitter(const G4ParticleDefinition* def) {
  // Ground and excited states. Names vary ("C11", "C11[0.0]"), so use Z and A.
  const G4int Z = def->GetAtomicNumber();
  const G4int A = def->GetAtomicMass();
  return (Z == 6 && (A == 11 || A == 10))
      || (Z == 8 && A == 15)
      || (Z == 7 && A == 13);
}

}

void CarbonPETScoring::Initialize(G4HCofThisEvent*) {
  // Dose and activity maps accumulate across events. Track IDs do not.
  fCountedThisEvent.clear();
}

G4int CarbonPETScoring::Index3D(const G4ThreeVector& pos, G4double voxel) const {
  // Target is centred in x and y and starts at z = 0.
  const G4double halfXY = 50. * mm;
  const G4double lengthZ = 2. * fTargetHalfZ;
  const G4int nx = std::max(1, static_cast<G4int>(std::floor((2. * halfXY) / voxel)));
  const G4int nz = std::max(1, static_cast<G4int>(std::floor(lengthZ / voxel)));
  G4int ix = static_cast<G4int>(std::floor((pos.x() + halfXY) / voxel));
  G4int iy = static_cast<G4int>(std::floor((pos.y() + halfXY) / voxel));
  G4int iz = static_cast<G4int>(std::floor(pos.z() / voxel));
  if (ix < 0 || iy < 0 || iz < 0 || ix >= nx || iy >= nx || iz >= nz) return -1;
  return iz * nx * nx + iy * nx + ix;
}

G4int CarbonPETScoring::GetDoseIndex(const G4ThreeVector& pos) {
  return Index3D(pos, fDoseVoxel);
}

G4int CarbonPETScoring::GetActivityIndex(const G4ThreeVector& pos) {
  return Index3D(pos, fActVoxel);
}

G4bool CarbonPETScoring::ProcessHits(G4Step* step, G4TouchableHistory*) {
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep > 0.) {
    G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();
    G4int idx = GetDoseIndex(pos);
    if (idx >= 0) fDoseMap[idx] += edep;
  }

  const G4Track* track = step->GetTrack();
  const auto* def = track->GetParticleDefinition();
  if (IsPetEmitter(def)) {
    // With radioactive decay off, stopped ions are fStopButAlive rather than killed.
    const auto status = track->GetTrackStatus();
    const bool stopped = status == fStopAndKill || status == fStopButAlive
                         || track->GetKineticEnergy() <= 1. * eV;
    if (stopped && fCountedThisEvent.insert(track->GetTrackID()).second) {
      G4int idx = GetActivityIndex(step->GetPostStepPoint()->GetPosition());
      if (idx >= 0) fActivityMap[idx] += 1.0;
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
  G4double totalDose = 0.;
  for (const auto& p : fDoseMap) totalDose += p.second;
  G4cout << "Wrote " << fDoseMap.size() << " dose voxels ("
         << totalDose / MeV << " MeV total) and " << fActivityMap.size()
         << " activity voxels to " << filename << G4endl;
}
