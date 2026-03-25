#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <cstdio>
#include <unistd.h>

#include "src/Hex_Environement.h"
#include "src/ExternalProgram_Player.h"

#include "lib/CLI11.hpp"


int main(int argc, char *argv[])
{
    CLI::App app("Environnement pour le jeu Hex");

    std::filesystem::path path_X="";
    app.add_option("-X", path_X, "Path vers un programme externe qui jouera au Hex pour les X. Le programme doit prendre en argument la taille du plateau de jeu et doit lire les coups de l'adversaire sur stdin et écrire ses coups sur stdout.")->check(CLI::ExistingFile)->required();

    std::filesystem::path path_O="";
    app.add_option("-O", path_O, "Path vers un programme externe qui jouera au Hex pour les O. Le programme doit prendre en argument la taille du plateau de jeu et doit lire les coups de l'adversaire sur stdin et écrire ses coups sur stdout.")->check(CLI::ExistingFile)->required();

    unsigned int taille=10;
    app.add_option("--size", taille, "Taille du plateau de jeu (default: 10)");

    bool noGUI=false;
    app.add_flag("--noGUI", noGUI, "Désactiver l'affichage graphique");

    bool slow=false;
    app.add_flag("--slow", slow, "Ajoutez un délai entre chaque coup de l'IA");

    CLI11_PARSE(app, argc, argv);

    Hex_Environement hex(!noGUI, taille);

    std::cerr << "Lancement du programme pour le joueur X : " << path_X << std::endl;
    hex.setPlayerX(std::make_unique<ExternalProgram>(std::filesystem::absolute(path_X).string(), 'X', taille));
    std::cerr << "Lancement du programme pour le joueur O : " << path_O << std::endl;
    hex.setPlayerO(std::make_unique<ExternalProgram>(std::filesystem::absolute(path_O).string(), 'O', taille));

    std::cerr << "Début de la partie !" << std::endl;
    while(hex.isGameOver() == false) {
        hex.printBoard();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        hex.play();

        if(slow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

    }
    hex.afficherFin();

    if(!noGUI) {
        while(getch() != 10) {
            hex.afficherFin();
            std::cerr << "Appuyer sur ENTER ou CTRL+C pour quitter" << std::endl;
        }
    } else {
        std::cerr << "Le joueur " << hex.getWinner() << " a gagné" << std::endl;
    }

    return 0;
}