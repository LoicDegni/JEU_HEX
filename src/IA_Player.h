#pragma once

#include <tuple>
#include <vector>
#include <memory>
#include <cassert>
#include <limits>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <cstdio>  
#include <random>
#include <unordered_set>

#include "Hex_Environement.h"

class UnionFind {
private:
    std::vector<int> parent;
    std::vector<int> rank;

    std::vector<bool> occupied;
    std::vector<char> ownership;

    std::vector<std::vector<char>> board;


    int top_virtual;
    int bottom_virtual;
    int left_virtual;
    int right_virtual;
    int coup_joue = 0;



    int N;

public:
    UnionFind(int n) : N(n) {
        int size = N * N + 4; // +4 virtual nodes

        parent.resize(size);
        occupied.resize(size);
        ownership.resize(size,'-');
        rank.resize(size, 0);
        board.resize(N, std::vector<char>(N, '-'));

        for (int i = 0; i < size; i++) {
            parent[i] = i;
            occupied[i] = false;
        }

        top_virtual = N * N;
        bottom_virtual = N * N + 1;
        left_virtual = N * N + 2;
        right_virtual = N * N + 3;
    }

    int id(int r, int c) const {
        return r * N + c;
    }

    int find(int x) {
        if (parent[x] != x)
            parent[x] = find(parent[x]);
        return parent[x];
    }

    void unite(int a, int b) {
        int ra = find(a);
        int rb = find(b);

        if (ra == rb) return;

        if (rank[ra] < rank[rb]) {
            parent[ra] = rb;
        }
        else if (rank[ra] > rank[rb]) {
            parent[rb] = ra;
        }
        else {
            parent[rb] = ra;
            rank[ra]++;
        }
    }

    bool connected(int a, int b) {
        return find(a) == find(b);
    }

    void reset() {
        int size = parent.size();
        for (int i = 0; i < size; i++) {
            parent[i] = i;
            rank[i] = 0;
        }
    }

    void applyMoveUF(int r, int c, char player) {
        int node = id(r, c);
        occupied[node] = true;
        ownership[node] = player;
        board[r][c] = player;
        coup_joue++;

        // directions Hex (6 voisins)
        int dr[6] = {-1, -1, 0, 0, 1, 1};
        int dc[6] = {0, 1, -1, 1, -1, 0};

        for (int i = 0; i < 6; i++) {
            int nr = r + dr[i];
            int nc = c + dc[i];

            if (nr >= 0 && nr < N && nc >= 0 && nc < N) {
                if (occupied[id(nr, nc)] && ownership[id(nr, nc)] == player) {
                    unite(node, id(nr, nc));
                }
            }
        }

        // connexions bords
        if (player == 'O') {
            if (r == 0) unite(node, top_virtual);
            if (r == N - 1) unite(node, bottom_virtual);
        }

        if (player == 'X') {
            if (c == 0) unite(node, left_virtual);
            if (c == N - 1) unite(node, right_virtual);
        }
    }

    bool isValidMoveUF(int r, int c) {
        /**
         * Fonction qui verifie si un move recu est valide.
         * 
        */
       return !occupied[id(r,c)];
    }

    bool hasWinner(char player) {
        if (player == 'O') {
            return connected(top_virtual, bottom_virtual);
        }
        else {
            return connected(left_virtual, right_virtual);
        }
    }
   
    void resetNbCoupJoue(){
        coup_joue = 0;
    }
    
    int getNbCoupJoue(){
        return coup_joue;
    }

    void printBoardUF() {
        int N = board.size();

        for (int i = 0; i < N; i++) {
            // décalage (indentation)
            for (int k = 0; k < i; k++) {
                std::cerr << " ";
            }

            // affichage des cellules
            for (int j = 0; j < N; j++) {
                std::cerr << board[i][j];

                if (j < N - 1) {
                    std::cerr << " - ";
                }
            }

            std::cerr << std::endl;

            // lignes de liaison (diagonales)
            if (i < N - 1) {
                for (int k = 0; k < i; k++) {
                    std::cerr << " ";
                }

                std::cerr << " ";

                for (int j = 0; j < N - 1; j++) {
                    std::cerr << "\\ / ";
                }

                std::cerr << "\\" << std::endl;
            }
        }
    }
};

class IA_Player : public Player_Interface {
private:
    char _player;
    unsigned int _taille;
    std::vector< std::tuple<unsigned int, unsigned int, char> > _historique_coups;
    std::mt19937 _random_number_generator;

    struct Node {
        Node* parent= nullptr;
        std::vector<Node*> children;
        int moveRow, moveCol;
        char playerJustMoved;

        int visits = 0;
        double wins = 0;

        std::vector<int> toVisit;
        std::vector<int> untriedMoves;
    };   
    Node* _root = nullptr;
    //-------------------MCTS-------------------//
    Node* select(Node* node) {
        double C = 1.41; //(2)^1/2 = 1.1414...
        int child_number = 0;

        Node* best = nullptr;
        double bestValue = -1e9;

        for(auto child: node->children) {
            std::cerr << "\nCHILD NUMBER : " << child_number << std::endl;
            std::cerr << "\nNODE STATS\nCoup joué sur cette node : (" << child->moveRow << "," << child->moveCol << ")\nNb wins : " << child->wins << "\nNb simulation passe par ce noeud : " << child->visits << std::endl; 
          
            double uct = (child->wins / (child->visits + 1e-6)) + C * sqrt(log(node->visits + 1) / (child->visits + 1e-6));   //log(1) = 0

            if (uct > bestValue) 
            {
                bestValue = uct;
                best = child;
            }
            child_number++;
        }
        std::cerr << "\nBEST NODE STATS\nCoup joué sur cette node : (" << best->moveRow << "," << best->moveCol << ")\nNb wins : " << best->wins << "\nNb simulation passe par ce noeud : " << best->visits << std::endl; 
        return best;
    }
   
    Node* expand(Node* node) {
        /**
         * Fonction qui recoit un noeud courant recupere un mouvement possible
         * du noeud et creer un noeud enfant avec le mouvement recuperé
         * 
         * Return:      Le noeud enfant
        */
        int b = node->untriedMoves.back();
        auto [row, col] = convertIDToCoordonate(b);
        node->untriedMoves.pop_back();

        Node* child = new Node();

        child->parent = node;
        child->moveRow = row;
        child->moveCol = col;

        child->playerJustMoved = (node->playerJustMoved == 'X') ? 'O' : 'X';
        child->toVisit = node->toVisit;

        std::swap(child->toVisit[node->untriedMoves.back()], child->toVisit.back());  
        auto tv_move_out = convertIDToCoordonate(child->toVisit.back());
        std::cerr << "Tovisit move pop update (" << row << "," << col << ") \n"; 
        std::cerr << "Tovisit update (" << tv_move_out.first << "," << tv_move_out.second << ") \n"; 
        std::cerr << "Tovisit ID check (" << b << ") \n";
        
        child->toVisit.pop_back();
        child->untriedMoves = child->toVisit;
        
        node->children.push_back(child);

        return child;
    }

    char simulate(Node* node) {
        if (node->toVisit.empty()) {
            return node->playerJustMoved;
        }
        UnionFind uf(_taille);

        std::vector< std::tuple<unsigned int, unsigned int, char> > moves;
        for (auto &h : _historique_coups) {
            moves.push_back(h);
        }

        std::vector<std::tuple<int,int, char>> path;
        Node* current = node;
        while (current->parent != nullptr) {
            path.push_back({current->moveRow, current->moveCol, current->playerJustMoved});
            current = current->parent;
        }
        std::reverse(path.begin(), path.end());

        moves.insert(moves.end(), path.begin(), path.end());

        for(auto [row,col,pl] : moves){
            uf.applyMoveUF(row,col,pl);
        }
        //std::cerr << "FIN RATRAPAGE HISTORIQUE pour joueur : " << ((node->playerJustMoved == 'X') ? 'O' : 'X') << std::endl;
        //uf.printBoardUF();

        std::vector<std::pair<int,int>> available;
        for (const auto &id : node->toVisit){
            auto move = convertIDToCoordonate(id);
            if(uf.isValidMoveUF(move.first, move.second)){
                available.push_back(move);
            }else {
                uf.printBoardUF();
                std::cerr << "Le coup invalide est [" << move.first << "," << move.second << "]\n" << std::endl;
                std::cerr << "Le UF contient\n" << std::endl;
                for (auto [row,col,pl]: moves){
                    std::cerr << "[" << row << "," << col << "]\n" << std::endl;

                }
            }
        }
        std::cerr << "\n\nCOMPARAISON DES TAILLES AVAILABLES-MOVES\nTaille available moves : " << available.size() << "\nTaille moves applied : " << moves.size() << "\nTaille path courant : " << path.size() << "\nTaille _historique_coups : " << _historique_coups.size() << "\nFIN COMPARAISON\n\n" << std::endl;
        //std::cerr << "\nLa taille initiale du available list est : " << available.size() << std::endl;

        char pl = node->playerJustMoved;

        // Debut de la simulation
        //std::cerr << "\n\nDEBUT SIMULATION pour joueur : " << ((pl == 'X') ? 'O' : 'X') << std::endl;
        do {
            //std::cerr <<"Avant le coup : " << std::endl;
            //uf.printBoardUF();
            pl = (pl == 'X') ? 'O' : 'X';
            std::uniform_int_distribution<int> uniform_moves_distribution(0, available.size() -1);
            int random_index = uniform_moves_distribution(_random_number_generator);
            auto move = available[random_index];
            //std::cerr << "Le coup joue est [" << move.first << "," << move.second << "] joueur : " << pl << std::endl;
            uf.applyMoveUF(move.first, move.second, pl);
            //std::cerr <<"Apres le coup : " << std::endl;
            //uf.printBoardUF();
            std::swap(available[random_index], available.back());
            auto move_out = available.back();
            available.pop_back();
            //std::cerr << "Le coup enlevé est [" << move_out.first << "," << move_out.second << "] joueur : " << pl << std::endl;
            //std::cerr << "\nLa taille mise à jour du available list est : " << available.size() << std::endl;
            //std::cerr <<"\nApres le coup : " << "[" << move.first << "," << move.second << "] joueur : " << pl << std::endl;
            //uf.printBoardUF();
        }while (!uf.hasWinner(pl) && !available.empty());

        if(!uf.hasWinner('X') && !uf.hasWinner('O')){
            uf.printBoardUF();
            std::cerr << "Erreur: available list est vide\n";
            std::exit(EXIT_FAILURE);
        }
        //std::cerr << "FIN SIMULATION Le gagnant est : \n" << pl << std::endl;
        //uf.printBoardUF();
        return pl;
    }

    void backpropagate(Node* node, char winner) {
        /**
         * La fonction remonte l'arbre MCTS et mets à jours les noeuds.
        */
       while (node != nullptr) {
        node->visits++;

        if (node->playerJustMoved == winner){
            node->wins++;
        }
        node = node->parent;
       }
    }
    //-------------------MCTS-------------------//
    Node* FindBestChild(Node* node) {
        Node* best = nullptr;
        int maxVisits = -1;
        int child_number = 0;

        for(auto child: node->children) {
            std::cerr << "\nCHILD NUMBER : " << child_number << std::endl;
            std::cerr << "\nNODE STATS\nCoup joué sur cette node : (" << child->moveRow << "," << child->moveCol << ")\nNb wins : " << child->wins << "\nNb simulation passe par ce noeud : " << child->visits << std::endl; 
            if(child->visits > maxVisits) {
                maxVisits = child->visits;
                best = child;
            }
            child_number++;
        }
        std::cerr << "BEST NODE STATS\nCoup joué sur cette node : (" << best->moveRow << "," << best->moveCol << ")\nNb wins : " << best->wins << "\nNb simulation passe par ce noeud : " << best->visits << std::endl; 
        return best;
    }

    void getAllMoves(Hex_Environement& hex) {
        std::vector<std::pair<int,int>> moves;
        // O(n^2)
        for(unsigned int i=0; i < _taille; i++) {
            for(unsigned int j = 0; j< _taille; j++) {
                if(hex.isValidMove(i,j)) {
                    _root->untriedMoves.push_back(convertCoordonateToID(i,j));
                    _root->toVisit.push_back(convertCoordonateToID(i,j));
                }
            }
        }
    }

    int convertCoordonateToID(int r, int c) {
        /**
         * Fonction qui convertit une coordonne(r,c)
         * en un identifiant unique
        */
        return r * _taille + c;
    }

    std::pair<int, int> convertIDToCoordonate(int id) {
        /**
         * Fonction qui convertit un identifiant 
         * en sa coordonnée(r,c) d'origine
        */
       return {id / _taille, id % _taille};
    }

public:
    IA_Player(char player, unsigned int taille=10) : _player(player), _taille(taille), _random_number_generator(std::random_device{}()) {
        assert(player == 'X' || player == 'O');
    }

    void printState() {
        UnionFind uf(_taille);

        for(auto [row,col,pl] : _historique_coups){
            uf.applyMoveUF(row,col,pl);
        }
        std::cerr << "TABLE DE JEU_HEX APRES LE COUP DU JOUEUR : " << ((_player == 'X') ? 'O' : 'X') << std::endl;
        uf.printBoardUF();

    }

    void otherPlayerMove(int row, int col) override {
        // l'autre joueur à joué (row, col).
        _historique_coups.push_back({row, col, (_player == 'X') ? 'O' : 'X'});
        //printState();

        if(_root != nullptr) {
            for(auto child : _root->children) {
                if(child->moveRow == row && child->moveCol == col) {
                    _root = child;
                    _root->parent = nullptr;
                    return;
                }
            }
            std::cerr << "RIen trouve\n";
            _root = nullptr; //Dans quel cas on ne trouve rien??
        }
    }

    std::tuple<int, int> getMove(Hex_Environement& hex) override {
        auto start = std::chrono::steady_clock::now();

        if(_root == nullptr) {
            _root = new Node();
            _root->playerJustMoved = (_player == 'X') ? 'O' : 'X';
            getAllMoves(hex);
        }
        int nb_selection = 0;
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1900)) {
            Node* node = _root;
            // 1. SELECTION
            while(node->untriedMoves.empty() && !node->children.empty()) {
                node = select(node);
                nb_selection++;
                std::cerr << "Il y a eu : " << nb_selection <<  "selections\nLast move  est : (" << node->moveRow << "," << node->moveCol << ")\nJoue par : " << node->playerJustMoved << std::endl;
            }
            // 2. EXPANSION
            if(!node->untriedMoves.empty()) {
                node = expand(node);
            }
            // 3. SIMULATION
            char winner = simulate(node);
            // 4. BACKDROP
            backpropagate(node,winner);
        }
        Node* best = FindBestChild(_root);
        _historique_coups.push_back({best->moveRow,  best->moveCol, _player});
        _root = best;
        _root->parent = nullptr;
        std::cerr << "Le meilleur move pour " << _player << " est : (" << best->moveRow << "," << best->moveCol << ")\n" << std::endl;
        return {best->moveRow, best->moveCol};
    }
};


