#include "CarbonPETScoring.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4GenericMessenger.hh"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <mutex>

namespace {

std::mutex& ScoringMutex() {
  static std::mutex mutex;
  return mutex;
}

struct SharedScore {
  std::map<G4int, G4double> dose;
  std::map<G4int, G4double> activity;
  G4String file{"scoring_results.txt"};
  G4int epoch = 0;
  G4int runId = 0;
  G4int mergeRun = -1;
  G4GenericMessenger* messenger = nullptr;
};

SharedScore& Shared() {
  static SharedScore shared;
  return shared;
}

void WriteMaps(const G4String& filename,
               const std::map<G4int, G4double>& dose,
               const std::map<G4int, G4double>& activity) {
  std::ofstream out(filename);
  out << "# dose_index dose_MeV\n";
  for (const auto& entry : dose) {
    out << entry.first << " " << entry.second / MeV << "\n";
  }
  out << "# activity_index counts\n";
  for (const auto& entry : activity) {
    out << entry.first << " " << entry.second << "\n";
  }
  G4double totalDose = 0.;
  for (const auto& entry : dose) totalDose += entry.second;
  G4cout << "Wrote " << dose.size() << " dose voxels ("
         << totalDose / MeV << " MeV total) and " << activity.size()
         << " activity voxels to " << filename << G4endl;
}

}

CarbonPETScoring::CarbonPETScoring(const G4String& name)
  : G4VSensitiveDetector(name),
    fDoseVoxel(1.0*mm),
    fActVoxel(1.5*mm),
    fTargetHalfZ(150*mm),
    fMessenger(nullptr),
    fAppliedEpoch(0) {
  // Master and every worker construct a detector. The command is registered once.
  std::lock_guard<std::mutex> lock(ScoringMutex());
  auto& shared = Shared();
  if (shared.messenger == nullptr) {
    fMessenger = new G4GenericMessenger(this, "/carbon/scoring/", "Scoring output");
    fMessenger->DeclareProperty(
        "file", shared.file,
        "File written at the end of each run. Use a name with no spaces.");
    fMessenger->DeclareMethod(
        "reset", &CarbonPETScoring::Reset,
        "Clear dose and activity accumulated so far");
    shared.messenger = fMessenger;
  }
}

CarbonPETScoring::~CarbonPETScoring() {
  delete fMessenger;
}

const G4String& CarbonPETScoring::GetOutputFile() const {
  return Shared().file;
}

void CarbonPETScoring::Reset() {
  fDoseMap.clear();
  fActivityMap.clear();
  fCountedThisEvent.clear();
  std::lock_guard<std::mutex> lock(ScoringMutex());
  auto& shared = Shared();
  shared.dose.clear();
  shared.activity.clear();
  ++shared.epoch;
  fAppliedEpoch = shared.epoch;
  G4cout << "Scoring maps reset. Next run will write " << shared.file << G4endl;
}

void CarbonPETScoring::ApplyResetEpoch() {
  G4int epoch = 0;
  {
    std::lock_guard<std::mutex> lock(ScoringMutex());
    epoch = Shared().epoch;
  }
  if (fAppliedEpoch == epoch) return;
  fDoseMap.clear();
  fActivityMap.clear();
  fCountedThisEvent.clear();
  fAppliedEpoch = epoch;
}

void CarbonPETScoring::BeginRun() {
  std::lock_guard<std::mutex> lock(ScoringMutex());
  auto& shared = Shared();
  ++shared.runId;
  shared.mergeRun = -1;
}

void CarbonPETScoring::MergeToShared() {
  std::lock_guard<std::mutex> lock(ScoringMutex());
  auto& shared = Shared();
  if (shared.mergeRun != shared.runId) {
    shared.dose.clear();
    shared.activity.clear();
    shared.mergeRun = shared.runId;
  }
  for (const auto& entry : fDoseMap) shared.dose[entry.first] += entry.second;
  for (const auto& entry : fActivityMap) shared.activity[entry.first] += entry.second;
}

void CarbonPETScoring::WriteMerged() {
  std::map<G4int, G4double> dose;
  std::map<G4int, G4double> activity;
  G4String filename;
  {
    std::lock_guard<std::mutex> lock(ScoringMutex());
    auto& shared = Shared();
    dose = shared.dose;
    activity = shared.activity;
    filename = shared.file;
  }
  WriteMaps(filename, dose, activity);
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
  WriteMaps(filename, fDoseMap, fActivityMap);
}
