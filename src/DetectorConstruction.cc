#include "DetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

namespace B1
{

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();

    // --- World volume ---
    G4double world_size = 2.0*m;
    G4Material* world_mat = nist->FindOrBuildMaterial("G4_AIR");

    G4Box* solidWorld = new G4Box("World",
        0.5*world_size, 0.5*world_size, 0.5*world_size);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(
        nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0, true);

    // --- Liquid xenon cylinder (simplified LZ active volume) ---
    G4double lxe_radius = 0.73*m;
    G4double lxe_height = 1.46*m;
    G4Material* lxe_mat = nist->FindOrBuildMaterial("G4_lXe");

    G4Tubs* solidLXe = new G4Tubs("LXeVolume",
        0., lxe_radius, 0.5*lxe_height, 0.*deg, 360.*deg);
    G4LogicalVolume* logicLXe = new G4LogicalVolume(solidLXe, lxe_mat, "LXeVolume");

    new G4PVPlacement(
        nullptr, G4ThreeVector(), logicLXe, "LXeVolume", logicWorld, false, 0, true);

    G4VisAttributes* lxeVis = new G4VisAttributes(G4Colour(0.0, 0.5, 1.0, 0.3));
    logicLXe->SetVisAttributes(lxeVis);

    fScoringVolume = logicLXe;

    return physWorld;
}

}  // namespace B1