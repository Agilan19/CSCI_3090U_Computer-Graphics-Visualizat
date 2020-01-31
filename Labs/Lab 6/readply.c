/*****************************************************
 *
 *                   readply.cpp
 *
 *  Read files in the ply format and returns the contents
 *  in a ply_model struct.
 *
 ****************************************************/

#include "readply.h"
#include "ply.h"
#include <stdio.h>

typedef struct vertex {
	float x, y, z;
} Vertex;

typedef struct face {
	unsigned char nverts;
	unsigned int *verts;
} Face;


PlyProperty vert_props[] = { /* list of property information for a vertex */
  {"x", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex, x), 0, 0, 0, 0},
  {"y", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex, y), 0, 0, 0, 0},
  {"z", PLY_FLOAT, PLY_FLOAT, offsetof(Vertex, z), 0, 0, 0, 0}
};

PlyProperty face_props[] = { /* list of property information for a face */
  {"vertex_indices", PLY_INT, PLY_INT, offsetof(Face, verts),
   1, PLY_UCHAR, PLY_UCHAR, 0},
};

struct ply_model* readply(char *filename) {
	FILE *fp;
	PlyFile *ply;
	int nelem;
	char **elem_list;
	int normals = 0;
	PlyProperty **properties;
	int nverts;
	int nprops;
	int i;
	struct ply_vertex *v;
	int nface;
	struct ply_face *f;
	int min = 10;
	int max = 0;
	int type;
	long *mesh;
	int j,k;
	FILE *dbgfile;
	int nmesh;
	struct ply_model *model;

	dbgfile = fopen("debug.txt","w");

	fp = fopen(filename,"rb");
	ply = ply_read(fp,&nelem,&elem_list);
	fprintf(dbgfile,"nelem = %d\n",nelem);
	fflush(dbgfile);

	properties = ply_get_element_description(ply,"vertex",&nverts,&nprops);

	fprintf(dbgfile,"nverts = %d\n",nverts);
	fflush(dbgfile);
	
	model = (struct ply_model*) malloc(sizeof(*model));
	model->nvertex = nverts;

	v = (struct ply_vertex*) malloc(nverts*sizeof(*v));
	model->vertices = v;
	ply_get_property(ply,"vertex",&vert_props[0]);
	ply_get_property(ply,"vertex",&vert_props[1]);
	ply_get_property(ply,"vertex",&vert_props[2]);

	for(i=0; i<nverts; i++) {
		ply_get_element(ply,&(v[i]));
	}

	properties = ply_get_element_description(ply,"face",&nface,&nprops);
	f = (struct ply_face *) malloc(nface*sizeof(*f));
	ply_get_property(ply,"face",&face_props[0]);

	fprintf(dbgfile,"nface = %d\n",nface);
	fflush(dbgfile);
	
	model->nface = nface;
	model->faces = f;

	for(i=0; i<nface; i++) {
		ply_get_element(ply,&f[i]);
	}

	return(model);

}
