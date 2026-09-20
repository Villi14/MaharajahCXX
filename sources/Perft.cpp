#include "../headers/Perft.h"
#include "../headers/Notation.h"

#include <format>
#include <iostream>

#include <chrono>

using namespace std;

namespace maharajah {

int Perft::get_time_ms() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return static_cast<int>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

u64 Perft::perft(int depth) {
  return perft_rec(depth);
}

u64 Perft::perft_rec(int depth) {
  if(depth == 0) {
    return 1;
  }

  MoveList moves;
  board_.generate_moves(moves);

  u64 nodes = 0;
  for(size_t i{ }; i < moves.size(); ++i) {
    const int move = moves[i];

    // make_move pushes state; if move is illegal it will pop internally and return false.
    if(!board_.make_move(move, TypeMove::all_moves))
      continue;

    nodes += perft_rec(depth - 1);
    board_.pop_state();
  }

  return nodes;
}

void Perft::perft_divide(int depth) {
  if(depth <= 0) {
    cout << "Nodes: 1\n";
    return;
  }

  MoveList moves_list;
  board_.generate_moves(moves_list);

  for(size_t i{ }; i < moves_list.size(); ++i) {
    const int move = moves_list[i];

    if(!board_.make_move(move, TypeMove::all_moves)) {
      continue;
    }

    const u64 nodes_for_move = perft_rec(depth - 1);
    board_.pop_state();

    cout << Notation::square_to_coordinates[Move::get_move_source(move)] << Notation::square_to_coordinates[Move::get_move_target(move)]
         << (Move::get_move_promoted(move) == no_pieces ? ' ' : Notation::promoted_pieces[Move::get_move_promoted(move)]) << ": " << nodes_for_move << '\n';
  }
}

void Perft::perft_test(int depth) {
  cout << "\n      Performance test\n\n";

  if(depth <= 0) {
    cout << "    Depth: " << depth << "\n    Nodes: 1\n";
    return;
  }

  MoveList moves_list;
  board_.generate_moves(moves_list);

  long start = get_time_ms();

  u64 total_nodes = 0;
  for(size_t i{ }; i < moves_list.size(); ++i) {
    const int move = moves_list[i];

    if(!board_.make_move(move, TypeMove::all_moves)) {
      continue;
    }

    const u64 nodes_for_move = perft_rec(depth - 1);
    total_nodes += nodes_for_move;
    board_.pop_state();

    cout << format("      move: {}{}{}    nodes: {}\n",
                   Notation::square_to_coordinates[Move::get_move_source(move)],
                   Notation::square_to_coordinates[Move::get_move_target(move)],
                   Move::get_move_promoted(move) ? Notation::promoted_pieces[Move::get_move_promoted(move)] : ' ',
                   nodes_for_move);
  }

  cout << "\n    Depth: " << depth;
  cout << "\n    Nodes: " << total_nodes << endl;
  cout << "\n     Time: " << get_time_ms() - start << endl;
}

} // namespace maharajah
