#include "CarbonPETActionInitialization.hh"
#include "CarbonPETPrimaryGeneratorAction.hh"
#include "CarbonPETSteppingAction.hh"
#include "CarbonPETScoring.hh"

#include "G4UserRunAction.hh"
#include "G4SDManager.hh"
#include "G4ProcessTable.hh"
#include "G4VRadioactiveDecay.hh"
#include "G4HadronicParameters.hh"
#include "G4Threading.hh"

namespace {

void DisableRadioactiveDecay() {
  // Geant4 11.3 aborts (PART122) when RadioactiveDecay inserts a nucleus that
  // ion inelastic scattering already created, e.g. stopped Be7 -> Li7.
  // The UI command /process/inactivate only affects the currently selected
  // particle, so ions created mid-event still decay. Turn the process off for
  // every particle, and ignore every at-rest decay on the shared process.
  auto* table = G4ProcessTable::GetProcessTable();
  table->SetProcessActivation("RadioactiveDecay", false);
  G4HadronicParameters::Instance()->SetTimeThresholdForRadioactiveDecay(0.);

  G4int nDisabled = 0;
  G4ProcessVector* procs = table->FindProcesses("RadioactiveDecay");
  if (procs) {
    for (G4int i = 0; i < static_cast<G4int>(procs->size()); ++i) {
      auto* rdm = dynamic_cast<G4VRadioactiveDecay*>((*procs)[i]);
      if (!rdm) continue;
      rdm->SetThresholdForVeryLongDecayTime(0.);
      ++nDisabled;
    }
    delete procs;
  }
  G4cout << "RadioactiveDecay disabled on " << nDisabled
         << " process objects. Beta-plus emitters are scored at rest."
         << G4endl;
}

CarbonPETScoring* FindScoring() {
  auto* sd = G4SDManager::GetSDMpointer()->FindSensitiveDetector("TargetScoring");
  return dynamic_cast<CarbonPETScoring*>(sd);
}

class CarbonPETRunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    DisableRadioactiveDecay();
    if (G4Threading::IsMasterThread()) CarbonPETScoring::BeginRun();
    if (auto* scoring = FindScoring()) scoring->ApplyResetEpoch();
  }

  void EndOfRunAction(const G4Run*) override {
    if (G4Threading::IsMultithreadedApplication()) {
      if (G4Threading::IsMasterThread()) {
        CarbonPETScoring::WriteMerged();
      } else if (auto* scoring = FindScoring()) {
        scoring->MergeToShared();
      }
      return;
    }
    if (auto* scoring = FindScoring()) {
      scoring->WriteResults(scoring->GetOutputFile());
    }
  }
};

}

void CarbonPETActionInitialization::BuildForMaster() const {
  SetUserAction(new CarbonPETRunAction);
}

void CarbonPETActionInitialization::Build() const {
  SetUserAction(new CarbonPETPrimaryGeneratorAction);
  SetUserAction(new CarbonPETSteppingAction);
  SetUserAction(new CarbonPETRunAction);
}
