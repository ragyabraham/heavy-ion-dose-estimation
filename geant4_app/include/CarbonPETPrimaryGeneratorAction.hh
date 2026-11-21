#ifndef CarbonPETPrimaryGeneratorAction_h
#define CarbonPETPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class CarbonPETPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  CarbonPETPrimaryGeneratorAction();
  virtual ~CarbonPETPrimaryGeneratorAction();
  virtual void GeneratePrimaries(G4Event*);

  void SetEnergy(G4double energy);          // MeV/u
  void SetBeamSigma(G4double sigmaXY);      // mm (Gaussian)
  void SetEnergySpread(G4double sigmaE);    // relative

private:
  G4ParticleGun* fParticleGun;
  G4double fEnergy;
  G4double fSigmaXY;
  G4double fEnergySpread;
};

#endif
