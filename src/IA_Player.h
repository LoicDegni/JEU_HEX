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
            occupied[i] = false;
            ownership[i] = '-';
        }
    }

    void applyMoveUF(int r, int c, char player) {
        int node = id(r, c);
        occupied[node] = true;
        ownership[node] = player;
        board[r][c] = player;

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
    int _id_max;
    UnionFind _uf;

    struct Node {
        Node* parent= nullptr;
        std::vector<Node*> children;
        int moveRow, moveCol;
        char playerJustMoved;
        int depth = 0;
        int visits = 0;
        double wins = 0;


        std::vector<int> toVisit;
        std::vector<int> untriedMoves;
    };   
    Node* _root = nullptr;

//-------------------ALGO MCTS-------------------//
    Node* select(Node* node) {
        double base_C = 1.8; //(2)^1/2 = 1.1414...1.41 1.0 - 1.5
        double C = base_C - ((node->depth/_taille) * 0.6);
        int child_number = 0;

        Node* best = nullptr;
        double bestValue = -1e9;

        for(auto child: node->children) {
            double uct = (child->wins / (child->visits + 1e-6)) + C * sqrt(log(node->visits + 1) / (child->visits + 1e-6));   //log(1) = 0

            if (uct > bestValue) 
            {
                bestValue = uct;
                best = child;
            }
            child_number++;
        }
 
        // On met a jour la carte _uf[O(n)]
        _uf.applyMoveUF(best->moveRow, best->moveCol, best->playerJustMoved);
        return best;
    }
   
    Node* expand(Node* node) {
        /**
         * Fonction qui recoit un noeud courant, recupere un mouvement possible
         * du noeud et creer un noeud enfant avec le mouvement recuperé
         * 
         * Return:      Le noeud enfant
        */
        int moveID;
        Node* child = new Node();
        
        moveID = node->untriedMoves.back();        
        node->untriedMoves.pop_back();

        child->parent = node;
        child->moveRow = convertIDToCoordonate(moveID).first;
        child->moveCol = convertIDToCoordonate(moveID).second;
        child->playerJustMoved = (node->playerJustMoved == 'X') ? 'O' : 'X';
        child->depth = node->depth + 1;
        child->toVisit = node->toVisit;

        //maj de child->tovisit
        auto it = std::find(child->toVisit.begin(), child->toVisit.end(),moveID);
        if (it != child->toVisit.end()) {
            std::swap(*it,child->toVisit.back());
            child->toVisit.pop_back();
        }
        // Ordre de visites aleatoire
        std::shuffle(child->toVisit.begin(),child->toVisit.end(),_random_number_generator);
        child->untriedMoves = child->toVisit;
        node->children.push_back(child);

        // On met a jour la carte _uf[O(n)]
        _uf.applyMoveUF(child->moveRow, child->moveCol, child->playerJustMoved);
        return child;
    }

    char simulate(Node* node) {

        std::vector< std::tuple<unsigned int, unsigned int, char> > all_moves_played;
        std::vector<std::tuple<int,int, char>> moves_played_from_root;
        std::vector<int> available_moves;
        char pl = node->playerJustMoved;

        if (node->toVisit.empty()) {
            return node->playerJustMoved;
        }

        //getAllMovesPlayed(node, all_moves_played, moves_played_from_root);
        //simulateToThePresent(all_moves_played);
        //getAvailableMoves(node, available_moves, all_moves_played);

        simulateToTheEnd(pl,node->toVisit);
        return pl;
    }

    void backpropagate(Node* node, char winner) {
        /**
         * La fonction remonte l'arbre MCTS et mets à jour les noeuds.
        */
       while (node != nullptr) {
        node->visits++;

        if (node->playerJustMoved == winner){
            node->wins++;
        }
        node = node->parent;
       }
    }
//-------------------ALGO MCTS-------------------//
    
public:
    IA_Player(char player, unsigned int taille=10) : _player(player), _taille(taille), _random_number_generator(std::random_device{}()), _uf(taille), _id_max( (taille - 1) * taille + (taille - 1) ) {
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
        _historique_coups.push_back({row, col, (_player == 'X') ? 'O' : 'X'});

        if(_root != nullptr) {
            for(auto child : _root->children) {
                if(child->moveRow == row && child->moveCol == col) {
                    _root = child;
                    _root->parent = nullptr;
                    //std::cerr << "\nla profondeur est : " << child->depth << std::endl;
                    // On met a jour la carte _uf[O(n)]
                    _uf.reset();
                    for(const auto& [r,c,pl]: _historique_coups) {
                        _uf.applyMoveUF(r,c,pl);
                    }
                    return;
                }
            }
            _root = nullptr; 
            // On met a jour la carte _uf[O(n)]
            _uf.reset();
            for(const auto& [r,c,pl]: _historique_coups) {
                _uf.applyMoveUF(r,c,pl);
            }
        }
    }

    std::tuple<int, int> getMove(Hex_Environement& hex) override {
        int simulation = 0;
        auto start = std::chrono::steady_clock::now();

        if(_root == nullptr) {
            _root = new Node();
            _root->playerJustMoved = (_player == 'X') ? 'O' : 'X';
            getAllMoves(hex);
        }

        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1900)) {
            Node* node = _root;

            // 1. SELECTION
            while(node->untriedMoves.empty() && !node->children.empty()) {
                node = select(node);
            }
            // 2. EXPANSION
            if(!node->untriedMoves.empty()) {
                node = expand(node);
            }
            // 3. SIMULATION
            char winner = simulate(node);
            simulation++;
            // 4. BACKDROP
            backpropagate(node,winner);

            // On met a jour la carte _uf[ (m * O(n)) + 1]
            _uf.reset();
            for(const auto& [r,c,pl]: _historique_coups) {
                _uf.applyMoveUF(r,c,pl);
            }
        }

        auto end = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(end - start).count();
        //std::cout << "NPS = " << simulation / seconds << std::endl;

        Node* best = FindBestChild(_root);
        _historique_coups.push_back({best->moveRow,  best->moveCol, _player});
        _root = best;
        _root->parent = nullptr;

        return {best->moveRow, best->moveCol};
    }

private:
    void getAllMovesPlayed(Node* node, std::vector< std::tuple<unsigned int, unsigned int, char> >& all_moves_played , std::vector<std::tuple<int,int, char>>& moves_played_from_root){
        Node* current = node;
        for (auto &h : _historique_coups) {
            all_moves_played.push_back(h);
        }

        while (current->parent != nullptr) {
            moves_played_from_root.push_back({current->moveRow, current->moveCol, current->playerJustMoved});
            current = current->parent;
        }
        std::reverse(moves_played_from_root.begin(), moves_played_from_root.end());

        all_moves_played.insert(all_moves_played.end(), moves_played_from_root.begin(), moves_played_from_root.end());
    }
    
    void simulateToThePresent(std::vector<std::tuple<unsigned int, unsigned int, char>>& all_moves_played) {
        for(auto [row,col,pl] : all_moves_played){
            _uf.applyMoveUF(row,col,pl);
        }
    }

    void simulateToTheEnd(char& pl, std::vector<int>& available_moves){
        do {
            pl = (pl == 'X') ? 'O' : 'X';
            std::uniform_int_distribution<int> uniform_moves_distribution(0, available_moves.size() -1);
            int random_index = uniform_moves_distribution(_random_number_generator);
            auto id = available_moves[random_index];
            auto move = convertIDToCoordonate(id);
            _uf.applyMoveUF(move.first, move.second, pl);
        }while (!_uf.hasWinner(pl));


        if (!_uf.hasWinner('X') && !_uf.hasWinner('O')){
            _uf.printBoardUF();
            std::cerr << "Erreur: available list est vide\n";
            std::exit(EXIT_FAILURE);
        }
    }

    void getAvailableMoves(Node* node, std::vector<std::pair<int, int>>& available_moves, std::vector< std::tuple<unsigned int, unsigned int, char> >& all_moves_played){
        
        for (const auto &id : node->toVisit){
            auto move = convertIDToCoordonate(id);
            if(_uf.isValidMoveUF(move.first, move.second)){
                available_moves.push_back(move);
            }else {
                _uf.printBoardUF();
                std::cerr << "Le coup invalide est [" << move.first << "," << move.second << "]\n" << std::endl;
                std::cerr << "Le UF contient\n" << std::endl;
                for (auto [row,col,pl]: all_moves_played){
                    std::cerr << "[" << row << "," << col << "]\n" << std::endl;
                }
            }
        }
    }

    Node* FindBestChild(Node* node) {
        Node* best = nullptr;
        int maxVisits = -1;
        int child_number = 0;

        for(auto child: node->children) {    
            if(child->visits > maxVisits) {
                maxVisits = child->visits;
                best = child;
            }
            child_number++;
        }
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
        std::shuffle(_root->untriedMoves.begin(),_root->untriedMoves.end(),_random_number_generator);
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

};


