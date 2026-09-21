#ifndef B1EventAction_h
#define B1EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <vector>

class G4Event;

namespace B1
{

class RunAction;

struct Vertex {
    G4double x, y, z, edep;
};

class EventAction : public G4UserEventAction
{
  public:
    EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void AddEdep(G4double edep) { fEdep += edep; }
    void AddVertex(G4double x, G4double y, G4double z, G4double edep);

  private:
    RunAction* fRunAction = nullptr;
    G4double fEdep = 0.;
    std::vector<Vertex> fVertices;
};

}  // namespace B1

#endif