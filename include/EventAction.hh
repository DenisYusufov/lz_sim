#ifndef B1EventAction_h
#define B1EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"
#include <vector>
#include <map>

class G4Event;

namespace B1
{

class RunAction;

struct TrackInfo   { G4int parentID; G4String particle; G4String creator; G4ThreeVector vtx; };
struct DepositStep { G4int trackID; G4ThreeVector pos; G4double edep; G4String process; };
struct Owner       { G4int gammaID; G4ThreeVector pos; G4String process; };

class EventAction : public G4UserEventAction
{
  public:
    EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void AddEdep(G4double edep) { fEdep += edep; }

    // track genealogy, filled from SteppingAction on each track's first step
    bool IsRegistered(G4int id) const { return fTracks.count(id) > 0; }
    void RegisterTrack(G4int id, const TrackInfo& t) { fTracks[id] = t; }

    // energy deposits, filled from SteppingAction
    void AddStep(G4int id, const G4ThreeVector& pos, G4double edep, const G4String& proc)
      { fSteps.push_back({id, pos, edep, proc}); fEdep += edep; }

  private:
    Owner FindOwner(const DepositStep& s) const;

    RunAction* fRunAction = nullptr;
    G4double fEdep = 0.;
    std::map<G4int, TrackInfo> fTracks;
    std::vector<DepositStep> fSteps;
};

}  // namespace B1

#endif