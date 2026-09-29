#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4Threading.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>
#include <fstream>
#include <sstream>
#include <tuple>

namespace B1
{

namespace {
  constexpr G4double kMergeRadius = 0.0 * mm;   // 0 = no merging (exact physical vertices)

  bool IsPhotonInteraction(const G4String& c) { return c == "compt" || c == "phot" || c == "conv"; }

  using VKey = std::tuple<G4int, long long, long long, long long>;   // position rounded to 1 micron
  VKey MakeKey(G4int gid, const G4ThreeVector& p) {
    auto q = [](G4double v) { return (long long)std::llround(v / (1e-3 * mm)); };
    return VKey(gid, q(p.x()), q(p.y()), q(p.z()));
  }

  struct VertexOut {
    G4ThreeVector pos;
    G4String process, photonCreator;
    G4int gammaID = 0;
    G4double edep = 0.;
    bool init = false;
  };
}

EventAction::EventAction(RunAction* runAction) : fRunAction(runAction) {}

void EventAction::BeginOfEventAction(const G4Event*)
{
  fEdep = 0.;
  fSteps.clear();
  fTracks.clear();
}

Owner EventAction::FindOwner(const DepositStep& s) const
{
  const TrackInfo& t0 = fTracks.at(s.trackID);
  if (t0.particle == "gamma") return { s.trackID, s.pos, s.process };  // deposit at its own interaction point

  G4int id = s.trackID;
  while (true) {
    const TrackInfo& t = fTracks.at(id);
    if (IsPhotonInteraction(t.creator)) {
      const TrackInfo& g = fTracks.at(t.parentID);                     // the photon that interacted
      const bool fold = (g.creator == "eBrem") && ((t.vtx - g.vtx).mag() < kMergeRadius);
      if (!fold) return { t.parentID, t.vtx, t.creator };
      id = t.parentID;                                                  // continue to the emitting electron
      continue;
    }
    if (t.parentID == 0) return { id, s.pos, "unknown" };
    id = t.parentID;
  }
}

void EventAction::EndOfEventAction(const G4Event* event)
{
  fRunAction->AddEdep(fEdep);
  if (fSteps.empty()) return;

  std::map<VKey, VertexOut> verts;
  for (const auto& s : fSteps) {
    Owner o = FindOwner(s);
    VertexOut& v = verts[MakeKey(o.gammaID, o.pos)];
    if (!v.init) {
      v.init = true;
      v.pos = o.pos;
      v.process = o.process;
      v.gammaID = o.gammaID;
      v.photonCreator = fTracks.at(o.gammaID).creator;
    }
    v.edep += s.edep;
  }

  std::ostringstream filename;
  filename << "vertices_thread" << G4Threading::G4GetThreadId() << ".csv";
  std::ofstream outFile(filename.str(), std::ios::app);
  for (const auto& kv : verts) {
    const auto& v = kv.second;
    outFile << event->GetEventID() << "," << v.pos.x() << "," << v.pos.y() << "," << v.pos.z() << ","
            << v.edep << "," << v.process << "," << v.gammaID << "," << v.photonCreator << "\n";
  }
}

}  // namespace B1