#include "CarbonPETPrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"
#include "Randomize.hh"
#include "G4Event.hh"
#include "G4ios.hh"

CarbonPETPrimaryGeneratorAction::CarbonPETPrimaryGeneratorAction()
  : fParticleGun(nullptr),
    fMessenger(nullptr),
    fEnergyPerNucleon(250.),
    fSigmaXY(3.4 * mm),
    fEnergySpread(0.002) {
  fParticleGun = new G4ParticleGun(1);
  fMessenger = new G4GenericMessenger(this, "/carbon/beam/", "Carbon beam");
  fMessenger->DeclareProperty(
      "energyPerNucleon", fEnergyPerNucleon,
      "Kinetic energy per nucleon in MeV/u (250 MeV/u = 3000 MeV for C12)");
  fMessenger->DeclareProperty(
      "energySpread", fEnergySpread,
      "Relative Gaussian sigma of the kinetic energy (0.002 = 0.2%)");
}

CarbonPETPrimaryGeneratorAction::~CarbonPETPrimaryGeneratorAction() {
  delete fMessenger;
  delete fParticleGun;
}

void CarbonPETPrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  // G4ParticleGun is constructed as a geantino. A geantino has no processes,
  // so GetParticleDefinition() is never null and the ion must be set explicitly.
  auto* ionTable = G4ParticleTable::GetParticleTable()->GetIonTable();
  auto* particle = ionTable->GetIon(6, 12, 0.0);
  if (!particle) {
    particle = G4ParticleTable::GetParticleTable()->FindParticle("C12");
  }
  fParticleGun->SetParticleDefinition(particle);
  // Relative spread is applied per primary. Floor the scale so a rare
  // negative Gaussian draw cannot reverse the beam.
  G4double scale = 1.;
  if (fEnergySpread > 0.) {
    scale = G4RandGauss::shoot(1., fEnergySpread);
    if (scale < 0.1) scale = 0.1;
  }
  const G4double totalMeV = fEnergyPerNucleon * 12. * scale;
  if (event->GetEventID() == 0) {
    G4cout << "Primary particle set to: "
           << (particle ? particle->GetParticleName() : "NULL")
           << "  " << fEnergyPerNucleon << " MeV/u"
           << "  (" << fEnergyPerNucleon * 12. << " MeV total before spread)"
           << "  sigma_E = " << fEnergySpread * 100. << " %" << G4endl;
  }

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  fParticleGun->SetParticleEnergy(totalMeV * MeV);

  // Start clearly upstream of the target (target occupies z = 0 to 300 mm)
  G4double x = G4RandGauss::shoot(0., fSigmaXY);
  G4double y = G4RandGauss::shoot(0., fSigmaXY);
  fParticleGun->SetParticlePosition(G4ThreeVector(x, y, -50. * mm));

  fParticleGun->GeneratePrimaryVertex(event);
}

void CarbonPETPrimaryGeneratorAction::SetEnergy(G4double totalMeV) {
  fEnergyPerNucleon = totalMeV / 12.;
}
void CarbonPETPrimaryGeneratorAction::SetBeamSigma(G4double sigmaXY) { fSigmaXY = sigmaXY; }
void CarbonPETPrimaryGeneratorAction::SetEnergySpread(G4double sigmaE) { fEnergySpread = sigmaE; }
