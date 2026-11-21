#ifndef CarbonPETActionInitialization_h
#define CarbonPETActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

class CarbonPETActionInitialization : public G4VUserActionInitialization {
public:
  CarbonPETActionInitialization() = default;
  virtual ~CarbonPETActionInitialization() = default;
  virtual void BuildForMaster() const override;
  virtual void Build() const override;
};

#endif
