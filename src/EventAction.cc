#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4Threading.hh"

#include <fstream>
#include <sstream>

namespace B1
{

EventAction::EventAction(RunAction* runAction) : fRunAction(runAction) {}

void EventAction::BeginOfEventAction(const G4Event*)
{
  fEdep = 0.;
  fVertices.clear();  // clear previous event's vertices
}

void EventAction::AddVertex(G4double x, G4double y, G4double z, G4double edep)
{
  fVertices.push_back({x, y, z, edep});
  fEdep += edep;  // keep the running total too, for compatibility
}

void EventAction::EndOfEventAction(const G4Event* event)
{
  fRunAction->AddEdep(fEdep);

  // Only write out events with at least one recorded interaction
  if (fVertices.empty()) return;

  G4int eventID = event->GetEventID();
  G4int threadID = G4Threading::G4GetThreadId();

  // One output file per worker thread, to avoid concurrent write conflicts
  std::ostringstream filename;
  filename << "vertices_thread" << threadID << ".csv";

  std::ofstream outFile(filename.str(), std::ios::app);
  for (const auto& v : fVertices) {
    outFile << eventID << "," << v.x << "," << v.y << "," << v.z << "," << v.edep << "\n";
  }
  outFile.close();
}

}  // namespace B1