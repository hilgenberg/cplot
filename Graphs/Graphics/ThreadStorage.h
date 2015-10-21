#pragma once

template<typename FACE> struct ThreadStorage
{
	size_t vertex_count; // number of vertexes in our vertex pool
	size_t face_count;   // number of faces in our face pool
	size_t vertex_offset; // for transfer: where do the vertices for this thread start
	size_t face_offset;   // same for faces
	
	MemoryPool<FACE> faces; // must be separate so we can iterate them
	RawMemoryPool pool;
	
	char padding[128-4*8-sizeof(RawMemoryPool)-sizeof(MemoryPool<FACE>)];
	
	ThreadStorage() : face_count(0), vertex_count(0), faces(128*64), pool(512*1024, 64)
	{
	}
	
	ThreadStorage(const ThreadStorage &) = delete;
	ThreadStorage &operator=(const ThreadStorage &) = delete;
	
	/*void *face_block(size_t n)
	{
		face_count += n;
		return faces.alloc(n*sizeof(Face));
	}*/

	/*/ perform the actual splitting
	void subdivide_finish()
	{
		PoolIterator fi(faces, sizeof(Face));
		size_t n = face_count;
		active_faces = 0;
		// if this stays 0, does not mean that this group is finished! neighbour groups can divide our faces too
		// divide_start could then be skipped though
		for (size_t i = 0; i < n; ++i, ++fi)
		{
			assert(fi);
			Face *f = (Face*)*fi;
			if (!f->done)
			{
				f->divide_finish(*this);
				if (!f->done) ++active_faces;
			}
		}
	}*/
};
