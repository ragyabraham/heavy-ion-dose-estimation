/**
 * CarbonPETSim - Geant4 application for 2D PET-based dose estimation data generation
 * Matches parameters from Rutherford et al., Phys. Med. Biol. 67 (2022) 194001
 *
 * Build: mkdir build && cd build && cmake .. && make -j
 * Run:   ./carbon_pet_sim macro.mac
 */

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "CarbonPETDetectorConstruction.hh"
#include "CarbonPETActionInitialization.hh"

#include "QBBC.hh"          // or FTFP_BERT_HP / QGSP_BIC_HP equivalent
#include "G4EmStandardPhysics_option3.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"

int main(int argc, char** argv) {
  // Detect interactive mode
  G4UIExecutive* ui = nullptr;
  if (argc == 1) {
    ui = new G4UIExecutive(argc, argv);
  }

  // Construct the default run manager (multi-threaded if available)
#ifdef G4MULTITHREADED
  G4MTRunManager* runManager = new G4MTRunManager;
  runManager->SetNumberOfThreads(G4Threading::G4GetNumberOfCores());
#else
  G4RunManager* runManager = new G4RunManager;
#endif

  // Detector construction
  runManager->SetUserInitialization(new CarbonPETDetectorConstruction());

  // Physics list: modern equivalent of paper's G4EmStandardPhysics_option3 + QGSP_BIC_HP
  G4VModularPhysicsList* physicsList = new QBBC;  // placeholder; replace with precise list
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option3());
  // physicsList->ReplacePhysics(new G4HadronPhysicsQGSP_BIC_HP());
  runManager->SetUserInitialization(physicsList);

  // User action initialization
  runManager->SetUserInitialization(new CarbonPETActionInitialization());

  // Initialize visualization
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if (!ui) {
    // Batch mode
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
  } else {
    // Interactive mode
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
