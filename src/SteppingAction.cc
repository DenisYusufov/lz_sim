#include "SteppingAction.hh"

#include "DetectorConstruction.hh"
#include "EventAction.hh"

#include "G4Event.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"

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

  G4LogicalVolume* volume =
    step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

  if (volume != fScoringVolume) return;

  G4double edepStep = step->GetTotalEnergyDeposit();

  // Only record steps that actually deposited energy (skip pure transportation steps)
  if (edepStep <= 0.) return;

  // Get the position of this interaction (use the pre-step point)
  G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();

  // Record this vertex (position + energy) into the event's vertex list
  fEventAction->AddVertex(pos.x(), pos.y(), pos.z(), edepStep);
}

}  // namespace B1