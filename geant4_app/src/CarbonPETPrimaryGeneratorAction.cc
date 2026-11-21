#include "CarbonPETPrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4Event.hh"
#include "G4ios.hh"

CarbonPETPrimaryGeneratorAction::CarbonPETPrimaryGeneratorAction() {
  fParticleGun = new G4ParticleGun(1);
  fEnergy = 3000;          // MeV total (~250 MeV/u)
  fSigmaXY = 3.4 * mm;
  fEnergySpread = 0.002;
}

CarbonPETPrimaryGeneratorAction::~CarbonPETPrimaryGeneratorAction() {
  delete fParticleGun;
}

void CarbonPETPrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
  if (!fParticleGun->GetParticleDefinition()) {
    auto* ionTable = G4ParticleTable::GetParticleTable()->GetIonTable();
    auto* particle = ionTable->GetIon(6, 12, 0.0);
    if (!particle) {
      particle = G4ParticleTable::GetParticleTable()->FindParticle("C12");
    }
    fParticleGun->SetParticleDefinition(particle);
    G4cout << "Primary particle set to: "
           << (particle ? particle->GetParticleName() : "NULL")
           << "  E = " << fEnergy << " MeV" << G4endl;
  }

  // Always force direction and energy
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  fParticleGun->SetParticleEnergy(fEnergy * MeV);

  // Start clearly upstream of the target (target occupies z = 0 to 300 mm)
  G4double x = G4RandGauss::shoot(0., fSigmaXY);
  G4double y = G4RandGauss::shoot(0., fSigmaXY);
  fParticleGun->SetParticlePosition(G4ThreeVector(x, y, -50. * mm));

  fParticleGun->GeneratePrimaryVertex(event);
}

void CarbonPETPrimaryGeneratorAction::SetEnergy(G4double energy) { fEnergy = energy; }
void CarbonPETPrimaryGeneratorAction::SetBeamSigma(G4double sigmaXY) { fSigmaXY = sigmaXY; }
void CarbonPETPrimaryGeneratorAction::SetEnergySpread(G4double sigmaE) { fEnergySpread = sigmaE; }
