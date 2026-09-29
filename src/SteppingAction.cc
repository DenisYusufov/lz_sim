#include "SteppingAction.hh"

#include "DetectorConstruction.hh"
#include "EventAction.hh"

#include "G4Event.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4Gamma.hh"

namespace B1
{

SteppingAction::SteppingAction(EventAction* eventAction) : fEventAction(eventAction) {}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  if (!fScoringVolume) {
    const auto detConstruction = static_cast<const DetectorConstruction*>(
      G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    fScoringVolume = detConstruction->GetScoringVolume();
  }

  G4Track* track = step->GetTrack();

  // Register every track on its first step, whatever volume it is in
  if (!fEventAction->IsRegistered(track->GetTrackID())) {
    const G4VProcess* cp = track->GetCreatorProcess();   // null for the primary
    fEventAction->RegisterTrack(track->GetTrackID(),
        { track->GetParentID(),
          track->GetDefinition()->GetParticleName(),
          cp ? cp->GetProcessName() : G4String("primary"),
          track->GetVertexPosition() });
  }

  G4LogicalVolume* volume =
    step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
  if (volume != fScoringVolume) return;

  G4double edepStep = step->GetTotalEnergyDeposit();
  if (edepStep <= 0.) return;

  // Gammas deposit at the interaction point (post-step); charged particles from where the step starts
  const bool isGamma = (track->GetDefinition() == G4Gamma::Definition());
  G4ThreeVector pos = isGamma ? step->GetPostStepPoint()->GetPosition()
                              : step->GetPreStepPoint()->GetPosition();
  const G4VProcess* proc = step->GetPostStepPoint()->GetProcessDefinedStep();

  fEventAction->AddStep(track->GetTrackID(), pos, edepStep,
                        proc ? proc->GetProcessName() : G4String("unknown"));
}

}  // namespace B1