#ifndef CarbonPETPrimaryGeneratorAction_h
#define CarbonPETPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4GenericMessenger;

class CarbonPETPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  CarbonPETPrimaryGeneratorAction();
  virtual ~CarbonPETPrimaryGeneratorAction();
  virtual void GeneratePrimaries(G4Event*);

  void SetEnergy(G4double energy);
  void SetBeamSigma(G4double sigmaXY);
  void SetEnergySpread(G4double sigmaE);

private:
  G4ParticleGun* fParticleGun;
  G4GenericMessenger* fMessenger;
  // Kinetic energy per nucleon. 250 MeV/u is 3000 MeV for 12C.
  G4double fEnergyPerNucleon;
  G4double fSigmaXY;
  G4double fEnergySpread;
};

#endif
