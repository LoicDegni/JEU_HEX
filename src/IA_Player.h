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
        // RAVE 
        int raveVisits = 0;
        double raveWins = 0;

        std::vector<std::pair<int,int>> untriedMoves;

        int moveRow, moveCol;
        char player;
    };
    //-------------------DÉFINITION DES FONCTIONS PRIVÉES-------------------//
    Node* select(Node* node) {
        double C = 1.41; //(2)^1/2 = 1.1414...

        Node* best = nullptr;
        double bestValue = -1e9;

        for(auto child: node->children) {
            double rave = 0.0;
            double uct = (child->wins / (child->visits + 1e-6)) + C * sqrt(log(node->visits + 1) / (child->visits + 1e-6));   //log(1) = 0
            if(child->raveVisits > 0) 
            {
                rave = child->raveWins / child->raveVisits;
            }
            double X = 4 * pow(0.001,0.001) * child->visits * child->raveVisits; // 4 ×b2 ×S[i].nb ×ni
            double beta = child->raveVisits / (child->visits + child->raveVisits + X + 1e-6);
            double value = (1 - beta) * uct + beta * rave;
            if (value > bestValue) 
            {
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

    std::pair<char,std::vector<std::pair<int,int>>> simulate(Hex_Environement state) {
        std::vector<std::pair<int,int>> played_move;
        while(!state.isGameOver()) {
            auto moves = getAllMoves(state);
            auto move = moves[rand()];
            played_move.push_back(move);
            state.playMove(move.first, move.second);
        }
        return {state.getWinner(), played_move};
    }

    void backpropagate(Node* node, char winner, std::vector<std::pair<int,int>> playedMoves) {
        while(node != nullptr) {
            node->visits++;

            if(node->player == winner) {
                node->wins++;
            }
            for(auto child : node->children) {
                for(auto move : playedMoves) {
                    if(child->moveRow == move.first && child->moveCol == move.second) {
                        child->raveVisits++;

                        if(child->player == winner) {
                            child->raveWins++;
                        }
                    }

                }
            }
            node = node->parent;
        }
    }

    Node* FindBestChild(Node* node) {
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
            auto result = simulate(node->state);
            // 4. BACKDROP
            backpropagate(node, result.first, result.second);
        }
        Node* best = FindBestChild(root);
        return {best->moveRow, best->moveCol};
    }
};


