#pragma once

#include <string>
#include <vector>
#include <map>
#include <array>
#include <memory>
#include <ostream>
#include "../../Utility/MemoryPool.h"

/**
 * @defgroup RecursiveGrid Recursive Grids
 * @{
 */

/**
 * Sparse boolean grids (2D version).
 * Every grid cell is subdivided into 4 sub-cells, up to a maximum depth.
 * The final cells carry one bool each.
 */

class RecursiveGrid_2D
{
public:
	/**
	 * Constructor for 2D grids.
	 *
	 * @param nx,ny    Total size of the grid
	 * @param depth    Subdivision depth. Higher depth compresses sparse grids better.
	 *
	 * Initializes all cells to false.
	 */
	RecursiveGrid_2D(size_t nx, size_t ny, unsigned char depth);
	
	/**
	 * Set grid cell to true.
	 * @param x,y   Cell index
	 */
	void set(size_t x, size_t y);
	
	/**
	 * Get value of a cell.
	 * @param x,y
	 * @return Value of the cell at (x,y).
	 */
	bool get(size_t x, size_t y) const;
	
	/**
	 * Get value of a cell block.
	 * @param x,y
	 * @param range
	 * @return True if any cell in [x-range, x+range] x [y-range, y+range] is true.
	 */
	bool get_range(size_t x, size_t y, int range) const;
	
	bool valid(int x, int y) const{ return x >= 0 && y >= 0 && (size_t)x < nx && (size_t)y < ny; }
	
private:
	bool get_subblock(size_t x0, size_t y0, size_t x1, size_t y1) const;
	
	size_t  nx,  ny;
	size_t nnx, nny; // size of base
	unsigned char depth;
	
	struct Cells
	{
		POOL_ITEM(Cells);
		
		Cells() : sub{nullptr} {}

		Cells *sub[4];
	};
	
	MemoryPool<Cells> pool;
	std::unique_ptr<Cells*[]> base; // nnx*nny*nnz array (must come after nnx, etc for c'tor)
	
	friend std::ostream &operator<<(std::ostream &out, const RecursiveGrid_2D &grid);
};

std::ostream &operator<<(std::ostream &out, const RecursiveGrid_2D &grid);


/**
 * Sparse boolean grids (3D version).
 * Every grid cell is subdivided into 8 sub-cells, up to a maximum depth.
 * The final cells carry one bool each.
 */

class RecursiveGrid_3D
{
public:
	/**
	 * Constructor for 3D grids.
	 *
	 * @param nx,ny,nz Total size of the grid
	 * @param depth    Subdivision depth. Higher depth compresses sparse grids better.
	 * 
	 * Initializes all cells to false.
	 */
	RecursiveGrid_3D(size_t nx, size_t ny, size_t nz, unsigned char depth);
	
	/**
	 * Set grid cells to true.
	 * @param x,y,z Cell index
	 */
	void set(size_t x, size_t y, size_t z);
	
	/**
	 * Get value of a cell.
	 * @param x,y,z
	 * @return Value of the cell at (x,y,z).
	 */
	bool get(size_t x, size_t y, size_t z) const;
	
	/**
	 * Get value of a cell block.
	 * @param x,y,z
	 * @param range
	 * @return True if any cell in [x-range, x+range] x [y-range, y+range] x [z-range, z+range] is true.
	 */
	bool get_range(size_t x, size_t y, size_t z, int range) const;
	
	bool valid(int x, int y, int z) const
	{
		return x >= 0 && y >= 0 && z >= 0 && (size_t)x < nx && (size_t)y < ny && (size_t)z < nz;
	}

private:
	bool get_subblock(size_t x0, size_t y0, size_t z0, size_t x1, size_t y1, size_t z1) const;

	size_t  nx,  ny,  nz;
	size_t nnx, nny, nnz; // size of base
	unsigned char depth;
	
	struct Cells
	{
		POOL_ITEM(Cells);

		Cells() : sub{nullptr} {}
		union
		{
			Cells   *sub[8];
			size_t final[8];
		};
	};
	
	MemoryPool<Cells> pool;
	std::unique_ptr<Cells*[]> base; // nnx*nny*nnz array (must come after nnx, etc for c'tor)

	friend std::ostream &operator<<(std::ostream &out, const RecursiveGrid_3D &grid);
};

std::ostream &operator<<(std::ostream &out, const RecursiveGrid_3D &grid);

/** @} */
