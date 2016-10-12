#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SYMBOL_ESCAPE 'R'
#define SYMBOL_EMPTY '.'
#define ORIENTATION_ROW 1
#define ORIENTATION_COLUMN 2
#define ORIENTATION_BOTH 3
#define HASH_FACTOR 31

typedef struct {
	int symbol;
	unsigned long start;
	int orientation;
	unsigned long cells_n;
}
vehicle_t;

typedef struct {
	unsigned long starts_idx;
	unsigned long last_idx;
	unsigned long next_idx;
}
grid_t;

typedef struct {
	unsigned long from_idx;
	int symbol;
	int direction;
	unsigned long cells_n;
}
move_t;

int update_or_set_vehicle(int, unsigned long, unsigned long);
int update_vehicle(vehicle_t *, unsigned long);
void set_vehicle(vehicle_t *, int, unsigned long);
unsigned long next_prime(unsigned long);
void set_grid(grid_t *, unsigned long, unsigned long, unsigned long);
void set_vehicle_cells(vehicle_t *, int);
int add_vehicle_moves(vehicle_t *, unsigned long);
int add_move(unsigned long, int, int, unsigned long);
int same_grid(grid_t *);
void set_move(move_t *, unsigned long, int, int, unsigned long);
void print_move(move_t *);

int *cells;
unsigned long grids_tank, rows_n, columns_n, grid_size, vehicles_n, *starts, hash_grids_n, grids_max, grids_n;
vehicle_t *vehicles, *vehicle_escape;
grid_t *grids;
move_t *moves;

int main(int argc, char *argv[]) {
char *end;
int symbol;
unsigned long i, j;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <grids tank>\n", argv[0]);
		return EXIT_FAILURE;
	}
	grids_tank = strtoul(argv[1], &end, 10);
	if (*end || !grids_tank) {
		fprintf(stderr, "Invalid grids tank\n");
		return EXIT_FAILURE;
	}
	if (scanf("%lu%lu", &rows_n, &columns_n) != 2 || !rows_n || !columns_n) {
		fprintf(stderr, "Invalid grid size\n");
		return EXIT_FAILURE;
	}
	if (fgetc(stdin) != '\n') {
		fprintf(stderr, "Unexpected character after grid size\n");
		return EXIT_FAILURE;
	}
	grid_size = rows_n*columns_n;
	vehicles_n = 0;
	vehicle_escape = NULL;
	for (i = 0; i < rows_n; i++) {
		for (j = 0; j < columns_n; j++) {
			symbol = fgetc(stdin);
			if (isdigit(symbol) || isupper(symbol) || islower(symbol)) {
				if (!update_or_set_vehicle(symbol, i, j)) {
					if (vehicles_n) {
						free(vehicles);
					}
					return EXIT_FAILURE;
				}
			}
			else if (symbol != SYMBOL_EMPTY) {
				fprintf(stderr, "Invalid symbol '%c' (line %lu column %lu)\n", symbol, i+1, j+1);
				if (vehicles_n) {
					free(vehicles);
				}
				return EXIT_FAILURE;
			}
		}
		if (fgetc(stdin) != '\n') {
			fprintf(stderr, "Unexpected character (line %lu)\n", i+1);
			if (vehicles_n) {
				free(vehicles);
			}
			return EXIT_FAILURE;
		}
	}
	if (!vehicle_escape) {
		fprintf(stderr, "No escape vehicle set\n");
		if (vehicles_n) {
			free(vehicles);
		}
		return EXIT_FAILURE;
	}
	starts = malloc(sizeof(unsigned long)*vehicles_n*grids_tank);
	if (!starts) {
		fprintf(stderr, "Could not allocate memory for starts\n");
		free(vehicles);
		return EXIT_FAILURE;
	}
	hash_grids_n = next_prime(grids_tank);
	grids = malloc(sizeof(grid_t)*(hash_grids_n+grids_tank));
	if (!grids) {
		fprintf(stderr, "Could not allocate memory for grids\n");
		free(starts);
		free(vehicles);
		return EXIT_FAILURE;
	}
	for (i = 0; i < hash_grids_n; i++) {
		set_grid(grids+i, 0UL, i, i);
	}
	moves = malloc(sizeof(move_t)*grids_tank);
	if (!moves) {
		fprintf(stderr, "Could not allocate memory for moves\n");
		free(grids);
		free(starts);
		free(vehicles);
		return EXIT_FAILURE;
	}
	cells = calloc(grid_size, sizeof(int));
	if (!cells) {
		fprintf(stderr, "Could not allocate memory for cells\n");
		free(moves);
		free(grids);
		free(starts);
		free(vehicles);
		return EXIT_FAILURE;
	}
	grids_max = grids_tank;
	grids_n = 0;
	if (!add_move(0UL, '.', ' ', 0UL)) {
		free(cells);
		free(moves);
		free(grids);
		free(starts);
		free(vehicles);
		return EXIT_FAILURE;
	}
	for (i = 0; i < grids_n; i++) {
		for (j = 0; j < vehicles_n; j++) {
			vehicles[j].start = starts[vehicles_n*i+j];
		}
		if (((vehicle_escape->orientation & ORIENTATION_ROW) && (vehicle_escape->start+vehicle_escape->cells_n)%columns_n == 0) || ((vehicle_escape->orientation & ORIENTATION_COLUMN) && vehicle_escape->start+columns_n*vehicle_escape->cells_n >= grid_size)) {
			break;
		}
		for (j = 0; j < vehicles_n; j++) {
			set_vehicle_cells(vehicles+j, 1);
		}
		for (j = 0; j < vehicles_n; j++) {
			if (!add_vehicle_moves(vehicles+j, i)) {
				free(cells);
				free(moves);
				free(grids);
				free(starts);
				free(vehicles);
				return EXIT_FAILURE;
			}
		}
		for (j = 0; j < vehicles_n; j++) {
			set_vehicle_cells(vehicles+j, 0);
		}
	}
	if (i < grids_n) {
		printf("Solution found at grid %lu\n", i);
		print_move(moves+i);
	}
	printf("Number of grids checked %lu\n", grids_n);
	free(cells);
	free(moves);
	free(grids);
	free(starts);
	free(vehicles);
	return EXIT_SUCCESS;
}

int update_or_set_vehicle(int symbol, unsigned long row, unsigned long column) {
unsigned long i;
vehicle_t *vehicles_tmp;
	for (i = 0; i < vehicles_n && vehicles[i].symbol != symbol; i++);
	if (i < vehicles_n) {
		return update_vehicle(vehicles+i, row*columns_n+column);
	}
	if (vehicles_n) {
		vehicles_tmp = realloc(vehicles, sizeof(vehicle_t)*(vehicles_n+1));
		if (!vehicles_tmp) {
			fprintf(stderr, "Could not reallocate memory for vehicles\n");
			return 0;
		}
		vehicles = vehicles_tmp;
	}
	else {
		vehicles = malloc(sizeof(vehicle_t));
		if (!vehicles) {
			fprintf(stderr, "Could not allocate memory for vehicles\n");
			return 0;
		}
	}
	set_vehicle(vehicles+vehicles_n, symbol, row*columns_n+column);
	if (symbol == SYMBOL_ESCAPE) {
		vehicle_escape = vehicles+vehicles_n;
	}
	vehicles_n++;
	return 1;
}

int update_vehicle(vehicle_t *vehicle, unsigned long end) {
	if (end == vehicle->start+vehicle->cells_n) {
		if (vehicle->orientation == ORIENTATION_BOTH) {
			vehicle->orientation = ORIENTATION_ROW;
		}
	}
	else if (end == vehicle->start+columns_n*vehicle->cells_n) {
		if (vehicle->orientation == ORIENTATION_BOTH) {
			vehicle->orientation = ORIENTATION_COLUMN;
		}
	}
	else {
		fprintf(stderr, "Invalid vehicle %c\n", vehicle->symbol);
		return 0;
	}
	vehicle->cells_n++;
	return 1;
}

void set_vehicle(vehicle_t *vehicle, int symbol, unsigned long start) {
	vehicle->symbol = symbol;
	vehicle->start = start;
	vehicle->orientation = ORIENTATION_BOTH;
	vehicle->cells_n = 1;
}

unsigned long next_prime(unsigned long value) {
unsigned long prime = value | 1, i;
	do {
		for (i = 3; i*i <= prime && prime%i; i += 2);
		if (i*i <= prime) {
			prime += 2;
		}
	}
	while (i*i <= prime);
	return prime;
}

void set_grid(grid_t *grid, unsigned long starts_idx, unsigned long last_idx, unsigned long next_idx) {
	grid->starts_idx = starts_idx;
	grid->last_idx = last_idx;
	grid->next_idx = next_idx;
}

void set_vehicle_cells(vehicle_t *vehicle, int value) {
unsigned long i, end = vehicle->start;
	cells[end] = value;
	for (i = 1; i < vehicle->cells_n; i++) {
		if (vehicle->orientation == ORIENTATION_ROW) {
			end++;
		}
		else {
			end += columns_n;
		}
		cells[end] = value;
	}
}

int add_vehicle_moves(vehicle_t *vehicle, unsigned long from_idx) {
unsigned long cells_n;
	if (vehicle->orientation & ORIENTATION_ROW) {
		for (cells_n = 1; vehicle->start%columns_n && !cells[vehicle->start-1]; cells_n++) {
			vehicle->start--;
			if (!add_move(from_idx, vehicle->symbol, 'W', cells_n)) {
				return 0;
			}
		}
		vehicle->start += cells_n-1;
		for (cells_n = 1; (vehicle->start+vehicle->cells_n)%columns_n && !cells[vehicle->start+vehicle->cells_n]; cells_n++) {
			vehicle->start++;
			if (!add_move(from_idx, vehicle->symbol, 'E', cells_n)) {
				return 0;
			}
		}
		vehicle->start -= cells_n-1;
	}
	if (vehicle->orientation & ORIENTATION_COLUMN) {
		for (cells_n = 1; vehicle->start >= columns_n && !cells[vehicle->start-columns_n]; cells_n++) {
			vehicle->start -= columns_n;
			if (!add_move(from_idx, vehicle->symbol, 'N', cells_n)) {
				return 0;
			}
		}
		vehicle->start += columns_n*(cells_n-1);
		for (cells_n = 1; vehicle->start+columns_n*vehicle->cells_n < grid_size && !cells[vehicle->start+columns_n*vehicle->cells_n]; cells_n++) {
			vehicle->start += columns_n;
			if (!add_move(from_idx, vehicle->symbol, 'S', cells_n)) {
				return 0;
			}
		}
		vehicle->start -= columns_n*(cells_n-1);
	}
	return 1;
}

int add_move(unsigned long from_idx, int symbol, int direction, unsigned long cells_n) {
unsigned long *starts_tmp, factor, hash, i;
grid_t *grids_tmp, *grid;
move_t *moves_tmp;
	if (grids_n == grids_max) {
		grids_max += grids_tank;
		starts_tmp = realloc(starts, sizeof(unsigned long)*vehicles_n*grids_max);
		if (!starts_tmp) {
			fprintf(stderr, "Could not reallocate memory for starts\n");
			return 0;
		}
		starts = starts_tmp;
		grids_tmp = realloc(grids, sizeof(grid_t)*(hash_grids_n+grids_max));
		if (!grids_tmp) {
			fprintf(stderr, "Could not reallocate memory for grids\n");
			return 0;
		}
		grids = grids_tmp;
		moves_tmp = realloc(moves, sizeof(move_t)*grids_max);
		if (!moves_tmp) {
			fprintf(stderr, "Could not reallocate memory for moves\n");
			return 0;
		}
		moves = moves_tmp;
	}
	factor = 1;
	hash = vehicles[0].start;
	for (i = 1; i < vehicles_n; i++) {
		factor *= HASH_FACTOR;
		hash += vehicles[i].start*factor;
	}
	hash %= hash_grids_n;
	for (grid = grids+grids[hash].next_idx; grid != grids+hash && !same_grid(grid); grid = grids+grid->next_idx);
	if (grid == grids+hash) {
		for (i = 0; i < vehicles_n; i++) {
			starts[vehicles_n*grids_n+i] = vehicles[i].start;
		}
		set_grid(grids+hash_grids_n+grids_n, vehicles_n*grids_n, grids[hash].last_idx, hash);
		grids[grids[hash].last_idx].next_idx = hash_grids_n+grids_n;
		grids[hash].last_idx = hash_grids_n+grids_n;
		set_move(moves+grids_n, from_idx, symbol, direction, cells_n);
		grids_n++;
	}
	return 1;
}

int same_grid(grid_t *grid) {
unsigned long i;
	for (i = 0; i < vehicles_n && starts[grid->starts_idx+i] == vehicles[i].start; i++);
	return i == vehicles_n;
}

void set_move(move_t *move, unsigned long from_idx, int symbol, int direction, unsigned long cells_n) {
	move->from_idx = from_idx;
	move->symbol = symbol;
	move->direction = direction;
	move->cells_n = cells_n;
}

void print_move(move_t *move) {
	if (move->cells_n) {
		print_move(moves+move->from_idx);
		printf("%c %c%lu\n", move->symbol, move->direction, move->cells_n);
	}
}
