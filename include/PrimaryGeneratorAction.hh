#ifndef B1PrimaryGeneratorAction_h
#define B1PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"

class G4ParticleGun;
class G4Event;
class G4Tubs;

namespace B1
{

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event*) override;

    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }

  private:
    G4ParticleGun* fParticleGun = nullptr;
    G4Tubs* fLXeTubs = nullptr;
};

}  // namespace B1

#endif