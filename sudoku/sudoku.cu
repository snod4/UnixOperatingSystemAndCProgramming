#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

// The width and height of a sudoku board
#define BOARD_DIM 9

// The width and heigh of a square group in a sudoku board
#define GROUP_DIM 3

// The number of boards to pass to the solver at one time
//Best size for our machine is 4096. It seems to level off after here at around 500ms for the huge.csv
//Increasing the batch size aids performance because it decreases the number of batches that need to be run, and since each batch requires some constant amount of overhead to run
//increasing the batch size decreases the number of times we incurr the overhead.
#define BATCH_SIZE 128

/**
 * A board is an array of 81 cells. Each cell is encoded as a 16-bit integer.
 * Read about this encoding in the documentation for the digit_to_cell and
 * cell_to_digit functions' documentation.
 *
 * Boards are stored as a one-dimensional array. It doesn't matter if you use
 * row-major or column-major form (that just corresponds to a rotation of the
 * sudoku board) but you will need to convert column and row to a single index
 * when accessing the board to propagate constraints.
 */
typedef struct board {
  uint16_t cells[BOARD_DIM * BOARD_DIM];
} board_t;

// Declare a few functions. Documentation is with the function definition.
void print_board(board_t* board);
__host__ __device__ uint16_t digit_to_cell(int digit);
__host__ __device__ int cell_to_digit(uint16_t cell);



/**
 * Runs each cell in its own thread, pulling the contraints found in other cells in its region, column,
 * and row until no threads are able to make a change to their possible values
 */
__global__ void kernel(board_t * boards, int num_boards){
  int boardIndex = blockIdx.x; //gets board index

  //ensures that we do not read from nonexistant boards
  if(boardIndex < num_boards){

    //initialize shared memory
    __shared__ board_t board;;
    board.cells[threadIdx.y * BOARD_DIM + threadIdx.x] = boards[boardIndex].cells[threadIdx.y * BOARD_DIM + threadIdx.x];
    __syncthreads();
    int changed = 1;

    //initialize start of the region of the cell
    int startRegionX;
    int startRegionY;

    //Loop that pulls constraints from other cells in the row, column, and region of the current cell and applies them to the current cell
    //until no other changes are possible
    while(__syncthreads_count(changed) != 0){
      changed = 1;
      startRegionX = (threadIdx.x/3)*3;
      startRegionY = (threadIdx.y/3)*3;
      uint16_t possibilities;
      uint16_t previous = board.cells[threadIdx.y * BOARD_DIM + threadIdx.x];

      //pull constraints from region
      for(int r = startRegionY; r < startRegionY+3; r++){
        for(int c = startRegionX; c < startRegionX+3; c++){
          if(r != threadIdx.y && c != threadIdx.x){
            possibilities =  1<<cell_to_digit(board.cells[BOARD_DIM*r + c]);
            board.cells[threadIdx.y * BOARD_DIM + threadIdx.x] = board.cells[threadIdx.y * BOARD_DIM + threadIdx.x]&~possibilities;
          }
        }
      }

      //pull constraints from column
      
      int startColumn = 0;
      for(int c = startColumn; c <9; c++) {
        if(c != threadIdx.x){
          possibilities =  1<<cell_to_digit(board.cells[BOARD_DIM*threadIdx.y + c]);
          board.cells[threadIdx.y * BOARD_DIM + threadIdx.x] = board.cells[threadIdx.y * BOARD_DIM + threadIdx.x]&~possibilities;
        }
      }

      //pull constraints from row
   
      int startRow = 0;
      for(int r = startRow; r <9; r++) {
        if(r != threadIdx.y){
          possibilities =  1<<cell_to_digit(board.cells[BOARD_DIM*r + threadIdx.x]);
          board.cells[threadIdx.y * BOARD_DIM + threadIdx.x] = board.cells[threadIdx.y * BOARD_DIM + threadIdx.x]&~possibilities;
        }
      }

      //check to see if a change was made to the current cell
      if(previous == board.cells[threadIdx.y * BOARD_DIM + threadIdx.x]) {
        changed = 0;
      }
    }

    //updates global gpu_board
    boards[boardIndex].cells[threadIdx.y * BOARD_DIM + threadIdx.x] =  board.cells[threadIdx.y * BOARD_DIM + threadIdx.x];
  }
}

/**
 * Take an array of boards and solve them all. The number of boards will be no
 * more than BATCH_SIZE, but may be less if the total number of input boards
 * is not evenly-divisible by BATCH_SIZE.
 *
 * TODO: Implement this function! You will need to add a GPU kernel, and you
 *       will almost certainly want to write helper functions; that is fine.
 *       However, you should not modify any other functions in this file.
 *
 * \param boards      An array of boards that should be solved.
 * \param num_boards  The numebr of boards in the boards array
 */
void solve_boards(board_t* boards, size_t num_boards) {
  // TODO: Implement me

  //mallocs space for an array of boards on the gpu
  board_t * gpu_boards;
  if(cudaMalloc(&gpu_boards, sizeof(board_t)*num_boards) != cudaSuccess){
    fprintf(stderr, "Failed to allocate gpu_board\n");
    exit(2);
  }

  //copys boards from cpu to gpu
  if(cudaMemcpy(gpu_boards, boards, sizeof(board_t) * num_boards, cudaMemcpyHostToDevice) != cudaSuccess){
    fprintf(stderr, "Failed to copy to gpu_boards\n");
  }

  //runs solver on gpu
  dim3 block_size(BOARD_DIM, BOARD_DIM); //declares the dimension of a board
  kernel<<<num_boards, block_size>>>(gpu_boards, num_boards);

  //waits for all threads to be done on the gpu
   if(cudaDeviceSynchronize() != cudaSuccess) {
    fprintf(stderr, "CUDA Error: %s\n", cudaGetErrorString(cudaPeekAtLastError()));
  }

   //copys boards from gpu to cpu, solved or not
   if(cudaMemcpy(boards, gpu_boards, sizeof(board_t) * num_boards, cudaMemcpyDeviceToHost) != cudaSuccess){
     fprintf(stderr, "Failed to copy to gpu_boards\n");
   }
  
}

/**
 * Take as input an integer value 0-9 (inclusive) and convert it to the encoded
 * cell form used for solving the sudoku. This encoding uses bits 1-9 to 
 * indicate which values may appear in this cell.
 *
 * For example, if bit 3 is set to 1, then the cell may hold a three. Cells that
 * have multiple possible values will have multiple bits set.
 *
 * The input digit 0 is treated specially. This value indicates a blank cell,
 * where any value from one to nine is possible.
 *
 * \param digit   An integer value 0-9 inclusive
 * \returns       The encoded form of digit using bits to indicate which values
 *                may appear in this cell.
 */
__host__ __device__ uint16_t digit_to_cell(int digit) {
  if(digit == 0) {
    // A zero indicates a blank cell. Numbers 1-9 are possible, so set bits 1-9.
    return 0x3FE;
  } else {
    // Otherwise we have a fixed value. Set the corresponding bit in the board.
    return 1<<digit;
  }
}

/*
 * Convert an encoded cell back to its digit form. A cell with two or more
 * possible values will be encoded as a zero. Cells with one possible value
 * will be converted to that value.
 *
 * For example, if the provided cell has only bit three set, this function will
 * return the value 3.
 *
 * \param cell  An encoded cell that uses bits to indicate which values could
 *              appear at this point in the board.
 * \returns     The value that must appear in the cell if there is only one
 *              possibility, or zero otherwise.
 */
__host__ __device__ int cell_to_digit(uint16_t cell) {
  // Get the index of the least-significant bit in this cell's value
#if defined(__CUDA_ARCH__)
  int msb = __clz(cell);
  int lsb = sizeof(unsigned int)*8 - msb - 1;
#else
  int lsb = __builtin_ctz(cell);
#endif

  // Is there only one possible value for this cell? If so, return it.
  // Otherwise return zero.
  if(cell == 1<<lsb) return lsb;
  else return 0;
}

/**
 * Read in a sudoku board from a string. Boards are represented as an array of
 * 81 16-bit integers. Each integer corresponds to a cell in the board. Bits
 * 1-9 of the integer indicate whether the values 1, 2, ..., 8, or 9 could
 * appear in the given cell. A zero in the input indicates a blank cell, where
 * any value could appear.
 *
 * \param output  The location where the board will be written
 * \param str     The input string that encodes the board
 * \returns       true if parsing succeeds, false otherwise
 */
bool read_board(board_t* output, const char* str) {
  for(int index=0; index<BOARD_DIM*BOARD_DIM; index++) {
    if(str[index] < '0' || str[index] > '9') return false;

    // Convert the character value to an equivalent integer
    int value = str[index] - '0';

    // Set the value in the board
    output->cells[index] = digit_to_cell(value);
  }

  return true;
}

/**
 * Print a sudoku board. Any cell with a single possible value is printed. All
 * cells with two or more possible values are printed as blanks.
 *
 * \param board   The sudoku board to print
 */
void print_board(board_t* board) {
  for(int row=0; row<BOARD_DIM; row++) {
    // Print horizontal dividers
    if(row != 0 && row % GROUP_DIM == 0) {
      for(int col=0; col<BOARD_DIM*2+BOARD_DIM/GROUP_DIM; col++) {
        printf("-");
      }
      printf("\n");
    }

    for(int col=0; col<BOARD_DIM; col++) {
      // Print vertical dividers
      if(col != 0 && col % GROUP_DIM == 0) printf("| ");

      // Compute the index of this cell in the board array
      int index = col + row * BOARD_DIM;

      // Get the index of the least-significant bit in this cell's value
      int digit = cell_to_digit(board->cells[index]);

      // Print the digit if it's not a zero. Otherwise print a blank.
      if(digit != 0) printf("%d ", digit);
      else printf("  ");
    }
    printf("\n");
  }
  printf("\n");
}

/**
 * Check through a batch of boards to see how many were solved correctly.
 *
 * \param boards        An array of (hopefully) solved boards
 * \param solutions     An array of solution boards
 * \param num_boards    The number of boards and solutions
 * \param solved_count  Output: A pointer to the count of solved boards.
 * \param error:count   Output: A pointer to the count of incorrect boards.
 */
void check_solutions(board_t* boards, board_t* solutions, size_t num_boards,
    size_t* solved_count, size_t* error_count) {

  // Loop over all the boards in this batch
  for(int i=0; i<num_boards; i++) {
    // Does the board match the solution?
    if(memcmp(&boards[i], &solutions[i], sizeof(board_t)) == 0) {
      // Yes. Record a solved board
      (*solved_count)++;
    } else {
      // No. Make sure the board doesn't have any constraints that rule out
      // values that are supposed to appear in the solution.
      bool valid = true;
      for(int j=0; j<BOARD_DIM * BOARD_DIM; j++) {
        if((boards[i].cells[j] & solutions[i].cells[j]) == 0) {
          valid = false;
        }
      }

      // If the board contains an incorrect constraint, record an error
      if(!valid) (*error_count)++;
    }
  }
}

/**
 * Entry point for the program
 */
int main(int argc, char** argv) {
  // Check arguments
  if(argc != 2) {
    fprintf(stderr, "Usage: %s <input file name>\n", argv[0]);
    exit(1);
  }

  // Try to open the input file
  FILE* input = fopen(argv[1], "r");
  if(input == NULL) {
    fprintf(stderr, "Failed to open input file %s.\n", argv[1]);
    perror(NULL);
    exit(2);
  }

  // Keep track of total boards, boards solved, and incorrect outputs
  size_t board_count = 0;
  size_t solved_count = 0;
  size_t error_count = 0;

  // Keep track of time spent solving
  size_t solving_time = 0;

  // Reserve space for a batch of boards and solutions
  board_t boards[BATCH_SIZE];
  board_t solutions[BATCH_SIZE];

  // Keep track of how many boards we've read in this batch
  size_t batch_count = 0;

  // Read the input file line-by-line
  char* line = NULL;
  size_t line_capacity = 0;
  while(getline(&line, &line_capacity, input) > 0) {
    // Read in the starting board
    if(!read_board(&boards[batch_count], line)) {
      fprintf(stderr, "Skipping invalid board...\n");
      continue;
    }

    // Read in the solution board
    if(!read_board(&solutions[batch_count], line + BOARD_DIM * BOARD_DIM + 1)) {
      fprintf(stderr, "Skipping invalid board...\n");
      continue;
    }

    // Move to the next index in the batch
    batch_count++;

    // Also increment the total count of boards
    board_count++;

    // If we finished a batch, run the solver
    if(batch_count == BATCH_SIZE) {
      size_t start_time = time_ms();
      solve_boards(boards, batch_count);
      solving_time += time_ms() - start_time;

      check_solutions(boards, solutions, batch_count,
          &solved_count, &error_count);

      // Reset the batch count
      batch_count = 0;
    }
  }

  // Check if there's an incomplete batch to solve
  if(batch_count > 0) {
    size_t start_time = time_ms();
    solve_boards(boards, batch_count);
    solving_time += time_ms() - start_time;

    check_solutions(boards, solutions, batch_count, &solved_count,
        &error_count);
  }


  // Print stats
  double seconds = (double)solving_time / 1000;
  double solving_rate = (double)solved_count / seconds;
  
  // Don't print nan when solver is not implemented
  if(seconds < 0.01) solving_rate = 0;

  printf("Boards: %lu\n", board_count);
  printf("Boards Solved: %lu\n", solved_count);
  printf("Errors: %lu\n", error_count);
  printf("Total Solving Time: %lums\n", solving_time);
  printf("Solving Rate: %.2f sudoku/second\n", solving_rate);

  return 0;
}

