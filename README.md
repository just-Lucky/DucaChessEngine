# Duca Chess Engine

> *Duca* (Italian for *Duke*) is a UCI-compliant chess engine written from scratch in C++17, built around an NNUE evaluation function and a heavily optimized alpha-beta search.

---

## Features

### Search
- Iterative deepening with asymmetric aspiration windows
- Alpha-beta with Principal Variation Search (PVS)
- Null move pruning
- Reverse futility pruning
- Futility pruning
- Late move reductions
- Late move pruning
- Check extensions
- Quiescence search with SEE filtering (static exchange evaluation)
- Lazy SMP multithreading

### Evaluation
- NNUE (Efficiently Updatable Neural Network) — incremental updates via dirty piece tracking
- AVX2-accelerated accumulator and forward pass
- Compatible with Stockfish HalfKP `.nnue` network files

### Move Generation
- Magic bitboard sliding piece attacks
- Pseudo-legal generation with legality check on make
- Dedicated capture-only generator for quiescence

### Move Ordering
- Transposition table move
- MVV-LVA (Most Valuable Victim / Least Valuable Attacker)
- Killer move heuristic (2 slots per ply)
- Countermove heuristic
- History heuristic with aging

### Transposition Table
- 16-byte entries: `hash_key`, `best_move`, `score`, `depth`, packed `age+flag`
- Power-of-two sizing for O(1) bitwise indexing
- Age-based replacement strategy

---

## Building

Duca requires a C++17-capable compiler with AVX2 support. GCC 10+ or Clang 12+ recommended.

```bash
g++ -O3 -march=native -flto -funroll-loops \
    -DNDEBUG -DUSE_AVX2 -DUSE_SSE41 -DUSE_SSE3 -DUSE_SSE2 -DUSE_SSE -DIS_64BIT \
    -ffast-math -fno-exceptions -fno-rtti \
    -std=c++17 -pipe \
    *.cpp -o duca.exe
```

> **Note:** `-march=native` compiles for your exact CPU. The resulting binary may not run on older processors. Replace with `-march=x86-64-v3` for a portable AVX2 build, this was also used to compile the .exe files in the `final` directories.

The NNUE network file (`nn-04cf2b4ed1da.nnue`) must be placed in the same directory as the executable, as well as
a file named `threads` and a file named `tt`.
The `threads` files must contain the number of search threads you intend to use, this number should be between 1 and
your CPU's number of cores.
The `tt` file must contain the number of MegaBytes you want to allocate to the `Transposition Table`, it also must be a
power of 2 between 256 and 2048.

---

## Usage

Duca communicates via the [UCI protocol](https://www.chessprogramming.org/UCI) and is designed to be used with a chess GUI such as [Arena](http://www.playwitharena.de/), [Cute Chess](https://cutechess.com/), or [Banksia](https://banksiagui.com/).

### Loading in a GUI

1. Open your GUI and navigate to the engine management section.
2. Add a new engine and point it to the Duca executable.
3. Duca will appear as a UCI engine and is ready to play.

### Running manually

Duca can also be run directly from the terminal for testing:

```
uci                          # identify engine and list options
isready                      # wait for engine to be ready
position startpos            # set starting position
position fen <fen>           # set a position from FEN
position startpos moves e2e4 # set position after moves
go depth 20                  # search to depth 20
go movetime 5000             # search for 5 seconds
go infinite                  # search until "stop"
stop                         # stop searching
quit                         # exit
```

---

## Architecture

```
duca.exe
├── main.cpp          Entry point, initialization
├── uci.cpp/h         UCI protocol loop
├── board.cpp/h       Board representation (bitboards + mailbox)
├── attacks.cpp/h     Magic bitboard attack tables
├── movegen.cpp/h     Pseudo-legal move generation
├── makemove.cpp/h    make_move / unmake_move, mailbox maintenance
├── eval.cpp/h        NNUE evaluation wrapper, thread-local accumulator state
├── nnue.cpp/h        NNUE forward pass (HalfKP, AVX2)
├── search.cpp/h      Alpha-beta, quiescence, Lazy SMP, ThreadData
├── tt.cpp/h          Transposition table
├── zobrist.cpp/h     Zobrist hashing
├── magic.cpp/h       Memory allocation for Magic tables
├── move.h            Move encoding to 16 bits
├── types.h           Enumerations
└── perft.cpp/h       Performance testing. Not utilized in the compiled .exe files
```

### Board Representation

The board is stored as twelve 64-bit bitboards (one per piece type and color) alongside a 64-entry mailbox array for O(1) piece lookup on a given square. Occupancy bitboards for each side and both combined are kept in sync by `update_occupancies`.

### Search

The main search is a negamax alpha-beta with iterative deepening. At each depth, the root is searched with aspiration windows centered on the previous iteration's score. On repeated failures the window is widened progressively; after three failures a bounded depth reduction warms the transposition table before the full-depth retry.

### NNUE Evaluation

The evaluation uses a HalfKP network loaded from a Stockfish-compatible `.nnue` file. Accumulators are updated incrementally via a dirty-piece mechanism maintained in `make_move`: only the changed features are added or removed from the accumulator rather than recomputing from scratch. Each search thread has its own accumulator stack (`thread_local NNUEdata nnue_thread_state[1024]`), eliminating all synchronization overhead on the evaluation path.

### Multithreading (Lazy SMP)

Multiple threads run independent iterative deepening searches on private copies of the board, sharing only the transposition table. Race conditions on TT reads and writes are benign by design: a torn write at worst causes a missed TT hit, never a crash or incorrect result. The main thread (id 0) is the sole source of UCI output and the final best move; helper threads contribute by populating the TT with evaluations the main thread will find when it reaches the same positions.

---

## Acknowledgements

- [Stockfish](https://github.com/official-stockfish/Stockfish) — NNUE architecture and HalfKP network format
- [Chess Programming Wiki](https://www.chessprogramming.org) — invaluable reference for every technique implemented here
- [Bruce Moreland's NNUE resources](https://github.com/dshawul/nnue-probe) — NNUE probe library that inspired the integration approach

---

## License

This project is released under the MIT License. See `LICENSE` for details.