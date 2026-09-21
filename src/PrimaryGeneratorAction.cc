#include "PrimaryGeneratorAction.hh"

#include "G4Tubs.hh"               // cylinder shape
#include "G4LogicalVolume.hh"       
#include "G4LogicalVolumeStore.hh" 
#include "G4ParticleGun.hh"        // particle source object
#include "G4ParticleTable.hh"      // lookup table of all known particle types
#include "G4SystemOfUnits.hh"      // gives MeV, cm, etc. as usable unit constants
#include "Randomize.hh"            // random number generation

namespace B1 // group all the code under the name "B1"
{

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  G4int n_particle = 1;
  fParticleGun = new G4ParticleGun(n_particle);

  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4String particleName;
  G4ParticleDefinition* particle = particleTable->FindParticle(particleName = "gamma");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.)); // for now gammas fired in z direction
  fParticleGun->SetParticleEnergy(2.45 * MeV);  // adjust to whatever energy you want to study
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  G4double lxeRadius = 0;
  G4double lxeHalfHeight = 0;

  if (!fLXeTubs) {
    G4LogicalVolume* lxeLV = G4LogicalVolumeStore::GetInstance()->GetVolume("LXeVolume");
    if (lxeLV) fLXeTubs = dynamic_cast<G4Tubs*>(lxeLV->GetSolid());
  }

  if (fLXeTubs) {
    lxeRadius = fLXeTubs->GetOuterRadius();
    lxeHalfHeight = fLXeTubs->GetZHalfLength();
  }
  else {
    G4ExceptionDescription msg;
    msg << "LXeVolume of Tubs shape not found.\n";
    msg << "Perhaps you have changed geometry.\n";
    msg << "The gun will be placed at the center.";
    G4Exception("PrimaryGeneratorAction::GeneratePrimaries()", "MyCode0002", JustWarning, msg);
  }

  G4double x0, y0, z0;
  G4ThreeVector direction;

  // Randomly choose entry surface, weighted roughly by relative surface area
  G4double sideArea = 2. * CLHEP::pi * lxeRadius * (2. * lxeHalfHeight);
  G4double capArea  = CLHEP::pi * lxeRadius * lxeRadius;
  G4double totalArea = sideArea + 2. * capArea;

  G4double roll = G4UniformRand() * totalArea;

  if (roll < sideArea) {
    // --- Enter from the curved side ---
    // Information on where the particles are initiated
    G4double phi = 2. * CLHEP::pi * G4UniformRand();  // random entry angle around the circle
    x0 = (lxeRadius + 1.*cm) * std::cos(phi);  // start just outside the radius
    y0 = (lxeRadius + 1.*cm) * std::sin(phi);
    z0 = (2. *  G4UniformRand() - 1.) * lxeHalfHeight;  //0.;  // mid-height
    direction = G4ThreeVector(-std::cos(phi), -std::sin(phi), 0.); // Direction: pointing inward, toward the central axis

  }
  else if (roll < sideArea + capArea) {
    // --- Enter from the top face ---
    G4double r = lxeRadius * std::sqrt(G4UniformRand());
    G4double phi = 2. * CLHEP::pi * G4UniformRand();
    x0 = r * std::cos(phi);
    y0 = r * std::sin(phi);
    z0 = lxeHalfHeight + 1.*cm;
    direction = G4ThreeVector(0., 0., -1.);  // pointing downward, into the volume
  }
  else {
    // --- Enter from the bottom face ---
    G4double r = lxeRadius * std::sqrt(G4UniformRand());
    G4double phi = 2. * CLHEP::pi * G4UniformRand();
    x0 = r * std::cos(phi);
    y0 = r * std::sin(phi);
    z0 = -lxeHalfHeight - 1.*cm;
    direction = G4ThreeVector(0., 0., 1.);  // pointing upward, into the volume
  }

  fParticleGun->SetParticleMomentumDirection(direction);
  fParticleGun->SetParticlePosition(G4ThreeVector(x0, y0, z0));
  fParticleGun->GeneratePrimaryVertex(event);
}

}  // namespace B1