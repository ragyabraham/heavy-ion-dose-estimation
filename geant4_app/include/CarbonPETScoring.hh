#ifndef CarbonPETScoring_h
#define CarbonPETScoring_h 1

#include "G4VSensitiveDetector.hh"
#include "G4ThreeVector.hh"
#include "G4THitsMap.hh"
#include <map>
#include <unordered_set>
#include <vector>

class G4GenericMessenger;

class CarbonPETScoring : public G4VSensitiveDetector {
public:
  CarbonPETScoring(const G4String& name);
  virtual ~CarbonPETScoring();

  virtual void Initialize(G4HCofThisEvent*);
  virtual G4bool ProcessHits(G4Step*, G4TouchableHistory*);
  virtual void EndOfEvent(G4HCofThisEvent*);

  void SetVoxelSize(G4double doseVox, G4double actVox);
  void WriteResults(const G4String& filename);
  void Reset();
  // Workers drop maps cleared by /carbon/scoring/reset, then add their
  // maps into one store. The master thread writes that store.
  void ApplyResetEpoch();
  void MergeToShared();
  static void BeginRun();
  static void WriteMerged();
  const G4String& GetOutputFile() const;

private:
  G4int GetDoseIndex(const G4ThreeVector& pos);
  G4int GetActivityIndex(const G4ThreeVector& pos);
  G4int Index3D(const G4ThreeVector& pos, G4double voxel) const;

  G4double fDoseVoxel;
  G4double fActVoxel;
  G4double fTargetHalfZ;

  G4GenericMessenger* fMessenger;

  std::map<G4int, G4double> fDoseMap;
  std::map<G4int, G4double> fActivityMap;
  std::unordered_set<G4int> fCountedThisEvent;
  G4int fAppliedEpoch;
};

#endif
