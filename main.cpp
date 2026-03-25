#include <iostream>
#include <cstdio>
#include <csignal>
#include <chrono>
#include <thread>

#include "lib/CLI11.hpp"


#include "src/Hex_Environement.h"

#include "src/IA_Player.h"
#include "src/ExternalProgram_Player.h"


bool noGUI=false;
void signal_handler(int signum) {
    if(!noGUI) {
        endwin();
    }
    exit(signum);
}

int main(int argc, char *argv[]) {
    std::signal(SIGINT, signal_handler);

    CLI::App app("Environnement pour le jeu Hex");

    std::string joueurX="";
    app.add_option("-X", joueurX, "Si -X \"IA\" alors le joueur X (rouge) sera joué par l'IA. Sinon, il s'agit d'un path vers un programme externe.");

    std::string joueurO="";
    app.add_option("-O", joueurO, "Si -O \"IA\" alors le joueur O (bleu) sera joué par l'IA. Sinon, il s'agit d'un path vers un programme externe.");

    bool slow=false;
    app.add_flag("--slow", slow, "Ajoutez un délai entre chaque coup de l'IA");

    app.add_flag("--noGUI", noGUI, "Désactiver l'affichage graphique");

    unsigned int taille=10;
    app.add_option("--size", taille, "Taille du plateau de jeu (default: 10)");

    unsigned int seed = 0;
    app.add_option("--seed", seed, "Seed pour l'aléatoire (par défaut: 0 pour seed aléatoire)");


    CLI11_PARSE(app, argc, argv);

    if(seed==0) {
        seed = time(NULL);
    }
    srand(seed);

    Hex_Environement hex(!noGUI, taille);

    if(joueurX.size()) {
        if(joueurX == "IA") {
            hex.setPlayerX(std::make_unique<IA_Player>('X', taille));
        } else {
            hex.setPlayerX(std::make_unique<ExternalProgram>(joueurX, 'X', taille));
        }
    }

    if(joueurO.size()) {
        if(joueurO == "IA") {
            hex.setPlayerO(std::make_unique<IA_Player>('O', taille));
        } else {
            hex.setPlayerO(std::make_unique<ExternalProgram>(joueurO, 'O', taille));
        }
    }

    while(hex.isGameOver() == false) {
        hex.printBoard();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        hex.play();
        if(noGUI && ((hex.getPlayer() == 'O' && joueurX == "IA") || (hex.getPlayer() == 'X' && joueurO == "IA"))) {
            std::cout << std::get<0>(hex.getLastMove()) << " " << std::get<1>(hex.getLastMove()) << std::endl;
        }
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
