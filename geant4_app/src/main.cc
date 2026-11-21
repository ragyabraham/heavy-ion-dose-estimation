#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "CarbonPETDetectorConstruction.hh"
#include "CarbonPETActionInitialization.hh"

#include "QGSP_BIC_HP.hh"
#include "G4EmStandardPhysics_option4.hh"

int main(int argc, char** argv) {
  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  runManager->SetUserInitialization(new CarbonPETDetectorConstruction());

  auto* physicsList = new QGSP_BIC_HP;
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option4());
  runManager->SetUserInitialization(physicsList);

  runManager->SetUserInitialization(new CarbonPETActionInitialization());

  auto* visManager = new G4VisExecutive;
  visManager->Initialize();

  auto* UImanager = G4UImanager::GetUIpointer();

  if (argc == 1) {
    auto* ui = new G4UIExecutive(argc, argv);
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  } else {
    G4String command = "/control/execute ";
    UImanager->ApplyCommand(command + argv[1]);
  }

  delete visManager;
  delete runManager;
  return 0;
}
