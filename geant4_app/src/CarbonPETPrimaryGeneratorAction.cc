#include "CarbonPETPrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"
#include "Randomize.hh"
#include "G4Event.hh"
#include "G4ios.hh"
#include "G4AutoLock.hh"

namespace {

G4Mutex beamMutex = G4MUTEX_INITIALIZER;

struct BeamConfig {
  G4double energyPerNucleon = 250.;
  G4double sigmaXY = 3.4 * mm;
  G4double energySpread = 0.002;
  G4GenericMessenger* messenger = nullptr;
};

BeamConfig& Beam() {
  static BeamConfig config;
  return config;
}

}

CarbonPETPrimaryGeneratorAction::CarbonPETPrimaryGeneratorAction()
  : fParticleGun(nullptr),
    fMessenger(nullptr) {
  fParticleGun = new G4ParticleGun(1);
  // Each worker builds a generator. The macro commands must change one shared
  // energy, or only the worker that owns the messenger would move off 250 MeV/u.
  G4AutoLock lock(&beamMutex);
  auto& beam = Beam();
  if (beam.messenger == nullptr) {
    fMessenger = new G4GenericMessenger(this, "/carbon/beam/", "Carbon beam");
    fMessenger->DeclareProperty(
        "energyPerNucleon", beam.energyPerNucleon,
        "Kinetic energy per nucleon in MeV/u (250 MeV/u = 3000 MeV for C12)");
    fMessenger->DeclareProperty(
        "energySpread", beam.energySpread,
        "Relative Gaussian sigma of the kinetic energy (0.002 = 0.2%)");
    beam.messenger = fMessenger;
  }
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
  BeamConfig beam;
  {
    G4AutoLock lock(&beamMutex);
    beam = Beam();
  }
  G4double scale = 1.;
  if (beam.energySpread > 0.) {
    scale = G4RandGauss::shoot(1., beam.energySpread);
    if (scale < 0.1) scale = 0.1;
  }
  const G4double totalMeV = beam.energyPerNucleon * 12. * scale;
  if (event->GetEventID() == 0) {
    G4cout << "Primary particle set to: "
           << (particle ? particle->GetParticleName() : "NULL")
           << "  " << beam.energyPerNucleon << " MeV/u"
           << "  (" << beam.energyPerNucleon * 12. << " MeV total before spread)"
           << "  sigma_E = " << beam.energySpread * 100. << " %" << G4endl;
  }

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  fParticleGun->SetParticleEnergy(totalMeV * MeV);

  // Start clearly upstream of the target (target occupies z = 0 to 300 mm)
  G4double x = G4RandGauss::shoot(0., beam.sigmaXY);
  G4double y = G4RandGauss::shoot(0., beam.sigmaXY);
  fParticleGun->SetParticlePosition(G4ThreeVector(x, y, -50. * mm));

  fParticleGun->GeneratePrimaryVertex(event);
}

void CarbonPETPrimaryGeneratorAction::SetEnergy(G4double totalMeV) {
  G4AutoLock lock(&beamMutex);
  Beam().energyPerNucleon = totalMeV / 12.;
}
void CarbonPETPrimaryGeneratorAction::SetBeamSigma(G4double sigmaXY) {
  G4AutoLock lock(&beamMutex);
  Beam().sigmaXY = sigmaXY;
}
void CarbonPETPrimaryGeneratorAction::SetEnergySpread(G4double sigmaE) {
  G4AutoLock lock(&beamMutex);
  Beam().energySpread = sigmaE;
}
