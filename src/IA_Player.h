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

#include "Hex_Environement.h"



class IA_Player : public Player_Interface {
    char _player;
    unsigned int _taille;

    struct Node {
        Hex_Environement& state;
        Node* parent;
        std::vector<Node*> children;

        int visits = 0;
        double wins = 0;

        std::vector<std::pair<int,int>> untriedMoves;

        int moveRow, moveCol;
        char player;
    };

    //-------------------DÉCLARATION DES FONCTIONS PRIVÉES-------------------//
    Node* select(Node* node);
    Node* expand(Node* node);
    //char simulate(Hex_Environement& state);
    void backpropagate(Node* node, char winner);
    Node* bestChild(Node* node);
    std::vector<std::pair<int,int>> getAllMoves(Hex_Environement& hex);


    //-------------------DÉFINITION DES FONCTIONS PRIVÉES-------------------//
    Node* select(Node* node) {
        double C = 1.41; //(2)^1/2 = 1.1414...

        Node* best = nullptr;
        double bestValue = -1e9;

        for(auto child: node->children) {
            double uct = (child->wins / (child->visits + 1e-6)) +
                         C * sqrt(log(node->visits + 1) / (child->visits + 1e-6));
            if(uct > bestValue) {
                bestValue = uct;
                best = child;
            }
        }
        return best;
    }
   
    Node* expand(Node* node) {
        auto move = node->untriedMoves.back();
        node->untriedMoves.pop_back();

        Hex_Environement newState = node->state;
        newState.playMove(move.first, move.second);

        Node* child = new Node{newState, node};
        child->moveRow = move.first;
        child->moveCol = move.second;
        child->player = node->state.getPlayer();

        child->untriedMoves = getAllMoves(newState);
        node->children.push_back(child);

        return child;
    }

    char simulate(Hex_Environement state) {
        while(!state.isGameOver()) {
            auto moves = getAllMoves(state);
            auto move = moves[rand()];
            state.playMove(move.first, move.second);
        }
        return state.getWinner();
    }

    void backpropagate(Node* node, char winner) {
        while(node != nullptr) {
            node->visits++;

            if(node->player == winner) {
                node->wins++;
            }
            node = node->parent;
        }
    }

    Node* bestchild(Node* node) {
        Node* best = nullptr;
        int maxVisits = -1;

        for(auto child: node->children) {
            if(child->visits > maxVisits) {
                maxVisits = child->visits;
                best = child;
            }
        }
        return best;
    }

    std::vector<std::pair<int,int>> getAllMoves(Hex_Environement& hex) {
        std::vector<std::pair<int,int>> moves;

        for(int i=0; i < _taille; i++) {
            for(int j = 0; j< _taille; j++) {
                if(hex.isValidMove(i,j)) {
                    moves.push_back({i,j});
                }
            }
        }
        return moves;
    }
     

public:
    IA_Player(char player, unsigned int taille=10) : _player(player), _taille(taille) {
        assert(player == 'X' || player == 'O');
    }

    void otherPlayerMove(int row, int col) override {
        // l'autre joueur à joué (row, col)
    }

    std::tuple<int, int> getMove(Hex_Environement& hex) override {
        Node* root = new Node{hex, nullptr};
        root->untriedMoves = getAllMoves(root->state);

        auto start = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1900)) {

            Node* node = root;

            // 1. SELECTION
            while(node->untriedMoves.empty() && !node->children.empty()) {
                node = select(node);
            }

            // 2. EXPANSION
            if(!node->untriedMoves.empty()) {
                node = expand(node);
            }

            // 3. SIMULATION
            char winner = simulate(node->state);

            // 4. BACKDROP
            backpropagate(node,winner);
        }
        Node* best = bestChild(root);

        return {best->moveRow, best->moveCol};
    }
};


